#pragma once

#include "systems/System.hpp"

namespace nbody {

template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
class Renderer {
public:
    Renderer() = default;
    virtual ~Renderer() = default;
    
    // Инициализация рендерера
    virtual bool initialize() = 0;
    
    // Отрисовка системы
    virtual void render(const System<T>& system) = 0;
    
    // Обработка событий
    virtual bool process_events() = 0;
    
    // Закрытие
    virtual void shutdown() = 0;
    
    // Установка и получение масштаба отображения
    void set_scale(T scale) { scale_ = scale; }
    T scale() const { return scale_; }
    
    // Установка и получение смещения отображения
    void set_offset_x(T offset_x) { offset_x_ = offset_x; }
    T offset_x() const { return offset_x_; }
    
    void set_offset_y(T offset_y) { offset_y_ = offset_y; }
    T offset_y() const { return offset_y_; }
    
    // Преобразование координат модели в координаты экрана
    int to_screen_x(T model_x) const {
        return static_cast<int>((model_x + offset_x_) * scale_);
    }
    
    int to_screen_y(T model_y) const {
        return static_cast<int>((model_y + offset_y_) * scale_);
    }
    
protected:
    T scale_ = T{100};    // Масштаб отображения
    T offset_x_ = T{0};   // Смещение по X
    T offset_y_ = T{0};   // Смещение по Y
};

} // namespace nbody 