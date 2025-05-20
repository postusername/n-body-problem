#pragma once

#include "systems/System.hpp"
#include <cmath>
#include <numbers>

namespace nbody {

template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
class RingSystem : public System<T> {
public:
    RingSystem(size_t num_bodies = 5) : num_bodies_(num_bodies) {}
    
    void generate() override {
        this->clear();
        
        constexpr T mass = T{1.0};   // Равные массы для всех тел
        constexpr T radius = T{1.0}; // Радиус кольца
        
        T total_mass = mass * num_bodies_;
        T orbit_velocity = std::sqrt(total_mass / radius);
        
        for (size_t i = 0; i < num_bodies_; ++i) {
            T angle = T{2.0} * std::numbers::pi_v<T> * i / num_bodies_;
            
            T x = radius * std::cos(angle);
            T y = radius * std::sin(angle);
            Vector<T> position(x, y, T{0});

            T vx = -orbit_velocity * std::sin(angle);
            T vy = orbit_velocity * std::cos(angle);
            Vector<T> velocity(vx, vy, T{0});
            
            this->add_body(Body<T>(mass, position, velocity, "Body " + std::to_string(i+1)));
        }
    }
    

    bool is_valid() const override {
        if (!System<T>::is_valid()) {
            return false;
        }

        Vector<T> center_of_mass;
        Vector<T> total_momentum;
        T total_mass = T{0};
        
        for (const auto& body : this->bodies()) {
            center_of_mass += body.position() * body.mass();
            total_momentum += body.velocity() * body.mass();
            total_mass += body.mass();
        }
        
        if (total_mass > T{0}) {
            center_of_mass = center_of_mass / total_mass;
        }
        
        constexpr T epsilon = T{0.01};
        
        // центр масс близок к нулю
        if (center_of_mass.magnitude() > epsilon) {
            std::cerr << "ERROR: центр масс отклонился: " << center_of_mass << std::endl;
            return false;
        }
        
        // суммарный импульс близок к нулю
        if (total_momentum.magnitude() > epsilon) {
            std::cerr << "ERROR: импульс отклонился: " << total_momentum << std::endl;
            return false;
        }
        
        return true;
    }
    
private:
    size_t num_bodies_ = 3;
};

} // namespace nbody 