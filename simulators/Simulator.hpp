#pragma once

#include <functional>
#include <stdexcept>

#include "systems/System.hpp"



namespace nbody {

template <typename T>
class Simulator {
public:
    using StepCallback = std::function<void(const System<T>&, T)>;
    
    Simulator() = default;
    virtual ~Simulator() = default;

    void set_system(System<T>* system) {
        system_ = system;
    }

    void set_dt(T dt) {
        if (dt <= T{0}) {
            throw std::invalid_argument("Time step must be positive");
        }
        dt_ = dt;
    }

    T dt() const {
        return dt_;
    }

    int steps_per_frame() const {
        return static_cast<int>(T{1e-2} / dt_);
    }

    void set_step_callback(StepCallback callback) {
        step_callback_ = callback;
    }
    
    // Установка гравитационной постоянной
    virtual void set_g(T g) {

    }
    
    // Выполнение одного шага симуляции
    // Возвращает false, если симуляция не может продолжаться
    virtual bool step() = 0;
    
    // Запуск симуляции на заданное количество шагов
    // Возвращает фактическое количество выполненных шагов
    std::size_t run(std::size_t max_steps) {
        if (!system_) {
            throw std::runtime_error("System not set");
        }
        
        std::size_t step_count = 0;
        for (; step_count < max_steps; ++step_count) {
            if (!step()) {
                break;
            }
            
            current_time_ += dt_;
            
            if (step_callback_) {
                step_callback_(*system_, current_time_);
            }
            
            if (!system_->is_valid()) {
                break;
            }
        }
        
        return step_count;
    }
    
    // Получение текущего времени симуляции
    T current_time() const { return current_time_; }

protected:
    System<T>* system_ = nullptr;
    T dt_ = T{0.01};         // Шаг по времени
    T current_time_ = T{0};  // Текущее время симуляции
    StepCallback step_callback_ = nullptr;
};

} // namespace nbody 