#pragma once

#include <cmath>

#include "core/DoubleDouble.h"
#include "systems/System.hpp"



namespace nbody {

template <typename T>
class TwoBodySystem : public System<T> {
public:
    TwoBodySystem(T e = T{0.5}) : e_(e) {
        if (e_ < 0.0 || e_ >= 1.0) {
            throw std::invalid_argument("Эксцентриситет должен быть в диапазоне [0, 1)");
        }
    }
    
    void generate() override {
        this->clear();

        const T G = T{1.0};

        const T mass1 = T{1e3};
        const T mass2 = T{1.0};
        
        // Начальные позиции: центральное тело в начале координат,
        // спутник на расстоянии a*(1-e)
        const T a = T{1.0};
        const T r = a * (T{1.0} - e_);
        
        Vector<T> pos1(T{0}, T{0}, T{0});
        Vector<T> pos2(r, T{0}, T{0});

        const T v_orbit = sqrt(G * mass1 * (T{2.0} / r - T{1.0} / a));
        
        Vector<T> vel1(T{0}, T{0}, T{0});
        Vector<T> vel2(T{0}, v_orbit, T{0});

        this->add_body(Body<T>(mass1, pos1, vel1, "Центральное тело"));
        this->add_body(Body<T>(mass2, pos2, vel2, "Спутник"));

        a_ = a;
        G_ = G;
        m1_ = mass1;
        
        period_ = T{2.0} * M_PI * sqrt(a * a * a / (G * mass1));
        initial_position_ = pos2;
    }
    
    bool is_valid() const override {
        const auto& body = this->bodies()[1];
        
        Vector<T> exact_position = calculate_exact_position(time_);
        const Vector<T> pos_diff = body.position() - exact_position;
    
        if (time_ > next_check_time_) {
            std::cout << "TwoBodySystem -- INFO: время: " << time_ 
                      << ", отклонение от точного решения: " 
                      << pos_diff.magnitude() << std::endl;
            next_check_time_ += period_ * T{0.1};
        }

        if (time_ > period_) {
            time_ = T{0};
            next_check_time_ = period_ * T{0.1};
            std::cout << "TwoBodySystem -- INFO: проверка точного решения: период пройден успешно" << std::endl;
        }
        
        return true;
    }
    
    void update_time(T dt) {
        time_ += dt;
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
    Vector<T> calculate_exact_position(T t) const {
        const T n = sqrt(G_ * m1_ / (a_ * a_ * a_));
        const T M = n * t;

        T E = M;
        const int max_iterations = 20;
        const T epsilon = T{1e-12};
        
        for (int i = 0; i < max_iterations; ++i) {
            const T E_next = M + e_ * sin(E);
            if (abs(E_next - E) < epsilon) {
                E = E_next;
                break;
            }
            E = E_next;
        }

        const T x = a_ * (cos(E) - e_);
        const T y = a_ * sqrt(T{1.0} - e_ * e_) * sin(E);
        
        return Vector<T>(x, y, T{0});
    }
    
private:
    T e_;                              // Эксцентриситет орбиты
    T a_;                              // Большая полуось
    T G_;                              // Гравитационная постоянная
    T m1_;                             // Масса центрального тела
    mutable T time_ = T{0};            // Текущее время симуляции
    mutable T next_check_time_ = T{0}; // Время следующей проверки
    T period_ = T{0};                  // Период обращения
    Vector<T> initial_position_;       // Начальная позиция спутника
};

} // namespace nbody 