#pragma once

#include "systems/System.hpp"
#include <cmath>

namespace nbody {

template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
class ThreeBodySystem : public System<T> {
public:
    ThreeBodySystem() = default;
    
    void generate() override {
        this->clear();

        // Chenciner, A., & Montgomery, R. (2000)
        // "A remarkable periodic solution of the three-body problem in the case of equal masses"
        
        constexpr T mass = T{1.0}; // Равные массы для всех тел
        
        // Начальные позиции
        Vector<T> pos1(T{-0.97000436}, T{0.24308753}, T{0});
        Vector<T> pos2(T{0}, T{0}, T{0});
        Vector<T> pos3(T{0.97000436}, T{-0.24308753}, T{0});
        
        // Начальные скорости
        Vector<T> vel1(T{0.4662036850}, T{0.4323657300}, T{0});
        Vector<T> vel2(T{-0.9324073700}, T{0}, T{0});
        Vector<T> vel3(T{0.4662036850}, T{-0.4323657300}, T{0});

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
        
        constexpr T epsilon = T{0.01};
        
        // суммарный импульс близок к нулю
        if (total_momentum.magnitude() > epsilon) {
            std::cerr << "Ошибка: импульс отклонился: " << total_momentum << std::endl;
            return false;
        }
        
        return true;
    }
};

} // namespace nbody 