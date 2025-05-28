#pragma once

#include <cmath>

#include "systems/System.hpp"



namespace nbody {

template <typename T>
class ThreeBodySystem : public System<T> {
public:
    ThreeBodySystem() = default;
    
    void generate() override {
        this->clear();

        // Chenciner A., Montgomery R. (2000)
        // "A remarkable periodic solution of the three-body problem in the case of equal masses"
        // https://arxiv.org/abs/math/0011268
        
        const T mass = T{1.0};
        
        Vector<T> pos1(T{-0.97000436}, T{0.24308753}, T{0});
        Vector<T> pos2 = -pos1;
        Vector<T> pos3(T{0}, T{0}, T{0});

        Vector<T> vel3(T{-0.93240737}, T{-0.86473146}, T{0});
        Vector<T> vel1 = -vel3 / T{2};
        Vector<T> vel2 = -vel3 / T{2};

        this->add_body(Body<T>(mass, pos1, vel1, "Body 1"));
        this->add_body(Body<T>(mass, pos2, vel2, "Body 2"));
        this->add_body(Body<T>(mass, pos3, vel3, "Body 3"));
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
        
        const T epsilon = T{1e-1};
        
        // суммарный импульс близок к нулю
        if (total_momentum.magnitude() > epsilon) {
            std::cerr << "Ошибка: импульс отклонился: " << total_momentum << std::endl;
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
};

} // namespace nbody 