#pragma once

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <type_traits>

#include "core/DoubleDouble.h"



namespace nbody {

template <typename T>
class Vector {
public:
    static constexpr std::size_t dimensions = 3;
    
    Vector() : data_{} {}
    
    Vector(T x, T y, T z) : data_{x, y, z} {}
    
    T& x() { return data_[0]; }
    T& y() { return data_[1]; }
    T& z() { return data_[2]; }
    
    const T& x() const { return data_[0]; }
    const T& y() const { return data_[1]; }
    const T& z() const { return data_[2]; }
    
    T& operator[](std::size_t index) { return data_[index]; }
    const T& operator[](std::size_t index) const { return data_[index]; }
    
    Vector& operator+=(const Vector& other) {
        for (std::size_t i = 0; i < dimensions; ++i) {
            data_[i] += other.data_[i];
        }
        return *this;
    }
    
    Vector& operator-=(const Vector& other) {
        for (std::size_t i = 0; i < dimensions; ++i) {
            data_[i] -= other.data_[i];
        }
        return *this;
    }
    
    Vector& operator*=(T scalar) {
        for (std::size_t i = 0; i < dimensions; ++i) {
            data_[i] *= scalar;
        }
        return *this;
    }
    
    Vector& operator/=(T scalar) {
        for (std::size_t i = 0; i < dimensions; ++i) {
            data_[i] /= scalar;
        }
        return *this;
    }
    
    T magnitude_squared() const {
        T result{};
        for (const auto& component : data_) {
            result += component * component;
        }
        return result;
    }
    
    T magnitude() const {
        return sqrt(magnitude_squared());
    }
    
    Vector normalized() const {
        T mag = magnitude();
        if (mag < std::numeric_limits<T>::epsilon()) {
            return Vector{};
        }
        return *this / mag;
    }
    
    Vector operator-() const {
        Vector result;
        for (std::size_t i = 0; i < dimensions; ++i) {
            result.data_[i] = -data_[i];
        }
        return result;
    }
private:
    std::array<T, dimensions> data_;
};

template <typename T>
Vector<T> operator+(const Vector<T>& lhs, const Vector<T>& rhs) {
    Vector<T> result = lhs;
    result += rhs;
    return result;
}

template <typename T>
Vector<T> operator-(const Vector<T>& lhs, const Vector<T>& rhs) {
    Vector<T> result = lhs;
    result -= rhs;
    return result;
}

template <typename T>
Vector<T> operator*(const Vector<T>& vec, T scalar) {
    Vector<T> result = vec;
    result *= scalar;
    return result;
}

template <typename T>
Vector<T> operator*(T scalar, const Vector<T>& vec) {
    return vec * scalar;
}

template <typename T>
Vector<T> operator/(const Vector<T>& vec, T scalar) {
    Vector<T> result = vec;
    result /= scalar;
    return result;
}

template <typename T>
T dot(const Vector<T>& lhs, const Vector<T>& rhs) {
    T result{};
    for (std::size_t i = 0; i < Vector<T>::dimensions; ++i) {
        result += lhs[i] * rhs[i];
    }
    return result;
}

template <typename T>
Vector<T> cross(const Vector<T>& lhs, const Vector<T>& rhs) {
    return Vector<T>(
        lhs.y() * rhs.z() - lhs.z() * rhs.y(),
        lhs.z() * rhs.x() - lhs.x() * rhs.z(),
        lhs.x() * rhs.y() - lhs.y() * rhs.x()
    );
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const Vector<T>& vec) {
    os << "(" << vec.x() << ", " << vec.y() << ", " << vec.z() << ")";
    return os;
}

} // namespace nbody 