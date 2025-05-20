#pragma once

#include "simulators/Simulator.hpp"

namespace nbody {

template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
class NewtonianSimulator : public Simulator<T> {
public:
    NewtonianSimulator() = default;

    void set_g(T g) {
        g_ = g;
    }
    
    Vector<T> calculate_gravity_force(const Body<T>& body1, const Body<T>& body2) const {
        Vector<T> r = body2.position() - body1.position();
        T distance_squared = r.magnitude_squared();
        
        // Избегаем деления на ноль
        if (distance_squared < T{1e-15}) {
            return Vector<T>{};
        }
        
        // Сила F = G * m1 * m2 / r^2 * (r_vec / |r|)
        T force_magnitude = g_ * body1.mass() * body2.mass() / distance_squared;
        return r.normalized() * force_magnitude;
    }

    bool step() override {
        if (!this->system_) {
            return false;
        }
        
        auto& bodies = this->system_->bodies();
        std::vector<Vector<T>> accelerations = calculate_accelerations(bodies);

        std::vector<Vector<T>> old_positions;
        old_positions.reserve(bodies.size());
        for (const auto& body : bodies) {
            old_positions.push_back(body.position());
        }
        
        // x(t+dt) = x(t) + v(t)*dt + 0.5*a(t)*dt²
        for (std::size_t i = 0; i < bodies.size(); ++i) {
            Body<T>& body = bodies[i];
            
            Vector<T> half_step_velocity = body.velocity() + accelerations[i] * (this->dt_ * T{0.5});
            body.set_position(body.position() + half_step_velocity * this->dt_);
        }
        
        // v(t+dt) = v(t) + 0.5*[a(t) + a(t+dt)]*dt
        std::vector<Vector<T>> new_accelerations = calculate_accelerations(bodies);
        for (std::size_t i = 0; i < bodies.size(); ++i) {
            Body<T>& body = bodies[i];
            Vector<T> avg_acceleration = (accelerations[i] + new_accelerations[i]) * T{0.5};
            body.set_velocity(body.velocity() + avg_acceleration * this->dt_);
        }
        
        return true;
    }
    
private:
    std::vector<Vector<T>> calculate_accelerations(std::vector<Body<T>>& bodies) const {
        std::vector<Vector<T>> accelerations(bodies.size(), Vector<T>{});
        
        for (std::size_t i = 0; i < bodies.size(); ++i) {
            for (std::size_t j = i + 1; j < bodies.size(); ++j) {
                Vector<T> force = calculate_gravity_force(bodies[i], bodies[j]);

                accelerations[i] += force / bodies[i].mass();
                accelerations[j] -= force / bodies[j].mass();
            }
        }
        
        return accelerations;
    }
    
    T g_ = T{1.0}; // Гравитационная постоянная G
};

} // namespace nbody 