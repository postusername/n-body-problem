#pragma once

#include "core/Body.hpp"
#include <vector>
#include <memory>
#include <stdexcept>

namespace nbody {

template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
class System {
public:
    System() = default;
    virtual ~System() = default;

    const std::vector<Body<T>>& bodies() const { return bodies_; }
    std::vector<Body<T>>& bodies() { return bodies_; }

    void add_body(const Body<T>& body) {
        bodies_.push_back(body);
    }

    void clear() {
        bodies_.clear();
    }

    std::size_t size() const {
        return bodies_.size();
    }
    
    // Проверка корректности состояния системы
    // Можно переопределить в подклассах для дополнительных проверок
    virtual bool is_valid() const {
        for (const auto& body : bodies_) {
            if (body.mass() <= T{0}) {
                return false;
            }
        }
        return true;
    }
    
    // Генерация начального состояния системы
    // Должна быть определена в подклассах
    virtual void generate() = 0;
    
protected:
    std::vector<Body<T>> bodies_;
};

} // namespace nbody 