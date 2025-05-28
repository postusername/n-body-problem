#pragma once

#include "simulators/Simulator.hpp"
#include "systems/TwoBodySystem.hpp"



namespace nbody {

template <typename T>
class NewtonianSimulator : public Simulator<T> {
public:
    NewtonianSimulator() = default;

    void set_g(T g) override {
        g_ = g;
    }
    
    Vector<T> calculate_gravity_force(const Body<T>& body1, const Body<T>& body2) const {
        Vector<T> r = body2.position() - body1.position();
        T distance_squared = r.magnitude_squared();
        
        // Избегаем деления на ноль
        if (distance_squared < T{1e-20}) {
            return Vector<T>{};
        }
        
        // F = G * m1 * m2 / r^2
        T force_magnitude = g_ * body1.mass() * body2.mass() / distance_squared;
        return r.normalized() * force_magnitude;
    }

    bool step() override {
        if (!this->system_) {
            return false;
        }
        
        auto& bodies = this->system_->bodies();
        std::vector<Vector<T>> current_accelerations = calculate_accelerations(bodies);
        
        // v(t + dt/2) = v(t) + a(t)*dt/2
        for (std::size_t i = 0; i < bodies.size(); ++i) {
            Body<T>& body = bodies[i];
            body.set_velocity(body.velocity() + current_accelerations[i] * (this->dt_ * T{0.5}));
        }

        // x(t + dt) = x(t) + v(t + dt/2) * dt
        for (std::size_t i = 0; i < bodies.size(); ++i) {
            Body<T>& body = bodies[i];
            body.set_position(body.position() + body.velocity() * this->dt_);
        }
        
        // a(t + dt) на основе новых позиций x(t + dt)
        std::vector<Vector<T>> next_accelerations = calculate_accelerations(bodies);
        
        // v(t + dt) = v(t + dt/2) + a(t + dt)*dt/2
        for (std::size_t i = 0; i < bodies.size(); ++i) {
            Body<T>& body = bodies[i];
            body.set_velocity(body.velocity() + next_accelerations[i] * (this->dt_ * T{0.5}));
        }
        
        auto* two_body_system = dynamic_cast<TwoBodySystem<T>*>(this->system_);
        if (two_body_system) {
            two_body_system->update_time(this->dt_);
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
    
    T g_ = T{1};
};

} // namespace nbody 