#pragma once

#include "core/Vector.hpp"
#include <string>

namespace nbody {

template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
class Body {
public:
    Body() = default;
    
    Body(T mass, const Vector<T>& position, const Vector<T>& velocity, const std::string& name = "")
        : mass_(mass), position_(position), velocity_(velocity), name_(name) {}
    
    T mass() const { return mass_; }
    void set_mass(T mass) { mass_ = mass; }
    
    const Vector<T>& position() const { return position_; }
    void set_position(const Vector<T>& position) { position_ = position; }
    
    const Vector<T>& velocity() const { return velocity_; }
    void set_velocity(const Vector<T>& velocity) { velocity_ = velocity; }
    
    Vector<T>& position() { return position_; }
    Vector<T>& velocity() { return velocity_; }
    
    const std::string& name() const { return name_; }
    void set_name(const std::string& name) { name_ = name; }
    
private:
    T mass_{1};
    Vector<T> position_{};
    Vector<T> velocity_{};
    std::string name_{};
};

} // namespace nbody 