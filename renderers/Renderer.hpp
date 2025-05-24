#pragma once

#include "simulators/Simulator.hpp"
#include "systems/System.hpp"



namespace nbody {

template <typename T>
class Renderer {
public:
    Renderer() {}
    virtual ~Renderer() = default;
    
    // Инициализация рендерера
    virtual bool initialize(Simulator<T>* simulator) = 0;
    
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
    
    // Установка и получение угла поворота по оси X
    void set_rotation_x(T rotation_x) { 
        rotation_x_ = fmod(rotation_x, T(2. * M_PI));
        if (rotation_x_ < 0.) rotation_x_ += T(2. * M_PI);
    }
    T rotation_x() const { return rotation_x_; }
    
    // Установка и получение угла поворота по оси Y
    void set_rotation_y(T rotation_y) { 
        rotation_y_ = fmod(rotation_y, T(2. * M_PI));
        if (rotation_y_ < 0.) rotation_y_ += T(2. * M_PI);
    }
    T rotation_y() const { return rotation_y_; }
    
    // Установка и получение угла поворота по оси Z
    void set_rotation_z(T rotation_z) { 
        rotation_z_ = fmod(rotation_z, T(2. * M_PI));
        if (rotation_z_ < 0.) rotation_z_ += T(2. * M_PI);
    }
    T rotation_z() const { return rotation_z_; }

    // Преобразование координат модели в координаты экрана с поворотом
    int to_screen_x(T model_x, T model_y, T model_z) const {
        T x, y, z;
        apply_rotation(model_x, model_y, model_z, x, y, z);
        return static_cast<int>((x + offset_x_) * scale_);
    }
    
    int to_screen_y(T model_x, T model_y, T model_z) const {
        T x, y, z;
        apply_rotation(model_x, model_y, model_z, x, y, z);
        return static_cast<int>((y + offset_y_) * scale_);
    }
    
    int to_screen_x(T model_x, T model_y) const {
        return to_screen_x(model_x, model_y, T{0});
    }
    
    int to_screen_y(T model_x, T model_y) const {
        return to_screen_y(model_x, model_y, T{0});
    }

    T to_model_x(int screen_x, int) const {
        return static_cast<T>(screen_x) / scale_ - offset_x_;
    }
    
    T to_model_y(int, int screen_y) const {
        return static_cast<T>(screen_y) / scale_ - offset_y_;
    }
    
private:
    void apply_rotation(T x, T y, T z, T& out_x, T& out_y, T& out_z) const {
        T cx = cos(-rotation_x_), sx = sin(-rotation_x_);
        T cy = cos(-rotation_y_), sy = sin(-rotation_y_);
        T cz = cos(-rotation_z_), sz = sin(-rotation_z_);
        
        T x1 = x;
        T y1 = y * cx - z * sx;
        T z1 = y * sx + z * cx;
        
        T x2 = x1 * cy + z1 * sy;
        T y2 = y1;
        T z2 = -x1 * sy + z1 * cy;
        
        out_x = x2 * cz - y2 * sz;
        out_y = x2 * sz + y2 * cz;
        out_z = z2;
    }
    
protected:
    T scale_ = T{100};      // Масштаб отображения
    T offset_x_ = T{0};     // Смещение по X
    T offset_y_ = T{0};     // Смещение по Y
    T rotation_x_ = T{0};   // Угол поворота по оси X
    T rotation_y_ = T{0};   // Угол поворота по оси Y
    T rotation_z_ = T{0};   // Угол поворота по оси Z

    Simulator<T>* simulator_ = nullptr;
};

} // namespace nbody 