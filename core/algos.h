#ifndef ALGOS_H
#define ALGOS_H

#include <cmath>
#include <iostream>



inline double two_sum(double a, double b, double &err)
{
    double s = a + b;
    double bb = s - a;
    err = (a - (s - bb)) + (b - bb);
    return s;
}

// Предполагается, что |a| >= |b|
inline double quick_two_sum(double a, double b, double &err)
{
    double s = a + b;
    err = b - (s - a);
    return s;
}


inline double two_diff(double a, double b, double &err)
{
    double s = a - b;
    double bb = s - a;
    err = (a - (s - bb)) - (b + bb);
    return s;
}


inline void split(double a, double &hi, double &lo)
{
    double temp;
    temp = 134217729.0 * a; // splitter = 2^27 (+ 1)
    hi = temp - (temp - a);
    lo = a - hi;
}


inline double two_prod(double a, double b, double &err)
{
    double a_hi, a_lo, b_hi, b_lo;
    double p = a * b;
    split(a, a_hi, a_lo);
    split(b, b_hi, b_lo);
    err = ((a_hi * b_hi - p) + a_hi * b_lo + a_lo * b_hi) + a_lo * b_lo;
    return p;
}


inline double nint(double d)
{
    if (d == std::floor(d))
        return d;
    return std::floor(d + 0.5);
}


inline double two_square(double a, double &err)
{
    double hi, lo;
    double q = a * a;
    split(a, hi, lo);
    err = ((hi * hi - q) + 2.0 * hi * lo) + lo * lo;
    return q;
}

#endif