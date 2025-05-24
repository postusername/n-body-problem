#pragma once

#include <cmath>
#include <numbers>

#include "systems/System.hpp"



namespace nbody {

template <typename T>
class CircleSystem : public System<T> {
public:
    CircleSystem() {}
    
    void generate() override {
        this->clear();
        
        const T mass = T{1.0};
        const T G = T{1.0};
        
        T orbit_velocity = sqrt(G * mass * T{num_bodies_} / (3.625 * radius_));
        
        for (int i = 0; i < int(num_bodies_); ++i) {
            T angle = T{2.0} * M_PI * T{i} / num_bodies_;
            
            T x = radius_ * cos(angle);
            T y = radius_ * sin(angle);
            Vector<T> position(x, y, T{0});

            T vx = -orbit_velocity * sin(angle);
            T vy = orbit_velocity * cos(angle);
            Vector<T> velocity(vx, vy, T{0});
            
            this->add_body(Body<T>(mass, position, velocity, "Body " + std::to_string(i+1)));
        }
    }
    

    bool is_valid() const override {
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
        
        const T epsilon = T{5e-1};
        
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

    T graph_value() const override {
        T total_energy = T{0};
        const T G = T{1.0};
        
        for (size_t i = 0; i < this->bodies().size(); ++i) {
            const auto& body_i = this->bodies()[i];
            T kinetic_energy = T{0.5} * body_i.mass() * body_i.velocity().magnitude_squared();
            total_energy += kinetic_energy;
            
            for (size_t j = i + 1; j < this->bodies().size(); ++j) {
                const auto& body_j = this->bodies()[j];
                T distance = (body_i.position() - body_j.position()).magnitude();
                if (distance > T{0}) {
                    T potential_energy = -G * body_i.mass() * body_j.mass() / distance;
                    total_energy += potential_energy;
                }
            }
        }
        
        return total_energy;
    }
    
private:
    T num_bodies_ = 5;  // Количество тел
    T radius_ = T{1.0}; // Радиус кольца
};

} // namespace nbody 