#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "core/algos.h"
#include "core/DoubleDouble.h"
using namespace std;



DoubleDouble DoubleDouble::add(double a, double b) {
  double s, e;
  s = two_sum(a, b, e);
  return DoubleDouble(s, e);
}


DoubleDouble operator+(const DoubleDouble &a, double b) {
  double s1, s2;
  s1 = two_sum(a.hi, b, s2);
  s2 += a.lo;
  s1 = two_sum(s1, s2, s2);
  return DoubleDouble(s1, s2);
}


DoubleDouble operator+(double a, const DoubleDouble &b) {
  return (b + a);
}


DoubleDouble operator+(const DoubleDouble &a, const DoubleDouble &b) {
  double s1, s2, t1, t2;

  s1 = two_sum(a.hi, b.hi, s2);
  t1 = two_sum(a.lo, b.lo, t2);
  s2 += t1;
  s1 = two_sum(s1, s2, s2);
  s2 += t2;
  s1 = two_sum(s1, s2, s2);
  return DoubleDouble(s1, s2);
}


DoubleDouble &DoubleDouble::operator+=(double a) {
  double s1, s2;
  s1 = two_sum(this->hi, a, s2);
  s2 += this->lo;
  this->hi = two_sum(s1, s2, this->lo);
  return *this;
}


DoubleDouble &DoubleDouble::operator+=(const DoubleDouble &a) {
  double s1, s2, t1, t2;
  s1 = two_sum(this->hi, a.hi, s2);
  t1 = two_sum(this->lo, a.lo, t2);
  s2 += t1;
  s1 = two_sum(s1, s2, s2);
  s2 += t2;
  this->hi = two_sum(s1, s2, this->lo);
  return *this;
}


DoubleDouble operator-(const DoubleDouble &a, double b) {
  double s1, s2;
  s1 = two_diff(a.hi, b, s2);
  s2 += a.lo;
  s1 = two_sum(s1, s2, s2);
  return DoubleDouble(s1, s2);
}


DoubleDouble operator-(const DoubleDouble &a, const DoubleDouble &b) {
  double s1, s2, t1, t2;
  s1 = two_diff(a.hi, b.hi, s2);
  t1 = two_diff(a.lo, b.lo, t2);
  s2 += t1;
  s1 = two_sum(s1, s2, s2);
  s2 += t2;
  s1 = two_sum(s1, s2, s2);
  return DoubleDouble(s1, s2);
}

DoubleDouble operator-(double a, const DoubleDouble &b) {
  double s1, s2;
  s1 = two_diff(a, b.hi, s2);
  s2 -= b.lo;
  s1 = two_sum(s1, s2, s2);
  return DoubleDouble(s1, s2);
}


DoubleDouble &DoubleDouble::operator-=(double a) {
  double s1, s2;
  s1 = two_diff(this->hi, a, s2);
  s2 += this->lo;
  this->hi = two_sum(s1, s2, this->lo);
  return *this;
}


DoubleDouble &DoubleDouble::operator-=(const DoubleDouble &a) {
  double s1, s2, t1, t2;
  s1 = two_diff(this->hi, a.hi, s2);
  t1 = two_diff(this->lo, a.lo, t2);
  s2 += t1;
  s1 = two_sum(s1, s2, s2);
  s2 += t2;
  this->hi = two_sum(s1, s2, this->lo);
  return *this;
}


DoubleDouble DoubleDouble::operator-() const {
  return DoubleDouble(-this->hi, -this->lo);
}

DoubleDouble operator*(const DoubleDouble &a, double b) {
  double p1, p2;

  p1 = two_prod(a.hi, b, p2);
  p2 += (a.lo * b);
  p1 = two_sum(p1, p2, p2);
  return DoubleDouble(p1, p2);
}


DoubleDouble operator*(double a, const DoubleDouble &b) {
  return (b * a);
}


DoubleDouble operator*(const DoubleDouble &a, const DoubleDouble &b) {
  double p1, p2;

  p1 = two_prod(a.hi, b.hi, p2);
  p2 += a.hi * b.lo;
  p2 += a.lo * b.hi;
  p1 = two_sum(p1, p2, p2);
  return DoubleDouble(p1, p2);
}


DoubleDouble &DoubleDouble::operator*=(double a) {
  double p1, p2;
  p1 = two_prod(this->hi, a, p2);
  p2 += this->lo * a;
  this->hi = two_sum(p1, p2, this->lo);
  return *this;
}

DoubleDouble &DoubleDouble::operator*=(const DoubleDouble &a) {
  double p1, p2;
  p1 = two_prod(this->hi, a.hi, p2);
  p2 += a.lo * this->hi;
  p2 += a.hi * this->lo;
  this->hi = two_sum(p1, p2, this->lo);
  return *this;
}

DoubleDouble operator/(const DoubleDouble &a, double b) {

  double q1, q2;
  double p1, p2;
  double s, e;
  DoubleDouble r;
  
  q1 = a.hi / b;   // approx.

  // (s, e) = a - q1 * b
  p1 = two_prod(q1, b, p2);
  s = two_diff(a.hi, p1, e);
  e += a.lo;
  e -= p2;
  
  // approx.
  q2 = (s + e) / b;

  // renormalize
  r.hi = two_sum(q1, q2, r.lo);

  return r;
}

DoubleDouble operator/(double a, const DoubleDouble &b) {
  return DoubleDouble(a) / b;
}

DoubleDouble operator/(const DoubleDouble &a, const DoubleDouble &b) {
  double q1, q2, q3;
  DoubleDouble r, t;

  q1 = a.hi / b.hi;  // first approx.
  r = a - q1 * b;
  
  q2 = r.hi / b.hi;  // second approx.
  r -= (q2 * b);

  q1 = two_sum(q1, q2, q2);
  q3 = r.hi / b.hi;
  t = DoubleDouble(q1, q2) + q3;
  
  while (q3 > 1e-42) {
    r -= (q3 * b); 
    q3 = r.hi / b.hi;
    t += q3;
  }

  return t;
}


DoubleDouble inv(const DoubleDouble &a) {
  return 1.0 / a;
}


DoubleDouble &DoubleDouble::operator/=(double a) {
  *this = *this / a;
  return *this;
}


DoubleDouble &DoubleDouble::operator/=(const DoubleDouble &a) {
  *this = *this / a;
  return *this;
}


// Частное и остаток от деления
DoubleDouble divrem(const DoubleDouble &a, const DoubleDouble &b, DoubleDouble &r) {
  DoubleDouble n = aint(a / b);
  r = a - n * b;
  return n;
}

DoubleDouble fmod(const DoubleDouble &a, const DoubleDouble &b) {
  DoubleDouble n = aint(a / b);
  return a - n * b;
}


DoubleDouble &DoubleDouble::operator=(double a) {
  hi = a;
  lo = 0.0;
  return *this;
}


DoubleDouble DoubleDouble::square(double a) {
  double p1, p2;
  p1 = two_square(a, p2);
  return DoubleDouble(p1, p2);
}

DoubleDouble square(const DoubleDouble &a) {
  double p1, p2;
  double s1, s2;
  p1 = two_square(a.hi, p2);
  p2 += 2.0 * a.hi * a.lo;
  p2 += a.lo * a.lo;
  s1 = two_sum(p1, p2, s2);
  return DoubleDouble(s1, s2);
}

// Возведение в степень
// 0^0 вызовет ошибку
DoubleDouble pow(const DoubleDouble &a, int n) {
  if (n == 0) {
    if (a.is_zero()) {
      cerr << "ERROR pow: 0^0" << endl;
      std::exit(-1);
    }

    return DoubleDouble(1.0);
  }

  DoubleDouble r = a;
  DoubleDouble s = DoubleDouble(1.0);
  int N = std::abs(n);

  if (N > 1) {
    // Бинарное возведение в степень
    while (N > 0) {
      if (N % 2 == 1) s *= r;
      N /= 2;
      if (N > 0) r = square(r);
    }
  } else s = r;

  if (n < 0) return (1.0 / s);
  return s;
}


DoubleDouble DoubleDouble::operator^(int n) {
  return pow(*this, n);
}


bool operator==(const DoubleDouble &a, double b) {
  return (a.hi == b && a.lo == 0.0);
}

bool operator==(const DoubleDouble &a, const DoubleDouble &b) {
  return (a.hi == b.hi && a.lo == b.lo);
}

bool operator==(double a, const DoubleDouble &b) {
  return (a == b.hi && b.lo == 0.0);
}

bool operator>(const DoubleDouble &a, double b) {
  return (a.hi > b || (a.hi == b && a.lo > 0.0));
}

bool operator>(const DoubleDouble &a, const DoubleDouble &b) {
  return (a.hi > b.hi || (a.hi == b.hi && a.lo > b.lo));
}

bool operator>(double a, const DoubleDouble &b) {
  return (a > b.hi || (a == b.hi && b.lo < 0.0));
}

bool operator<(const DoubleDouble &a, double b) {
  return (a.hi < b || (a.hi == b && a.lo < 0.0));
}

bool operator<(const DoubleDouble &a, const DoubleDouble &b) {
  return (a.hi < b.hi || (a.hi == b.hi && a.lo < b.lo));
}

bool operator<(double a, const DoubleDouble &b) {
  return (a < b.hi || (a == b.hi && b.lo > 0.0));
}

bool operator>=(const DoubleDouble &a, double b) {
  return (a.hi > b || (a.hi == b && a.lo >= 0.0));
}

bool operator>=(const DoubleDouble &a, const DoubleDouble &b) {
  return (a.hi > b.hi || (a.hi == b.hi && a.lo >= b.lo));
}

bool operator>=(double a, const DoubleDouble &b) {
  return (b <= a);
}

bool operator<=(const DoubleDouble &a, double b) {
  return (a.hi < b || (a.hi == b && a.lo <= 0.0));
}

bool operator<=(const DoubleDouble &a, const DoubleDouble &b) {
  return (a.hi < b.hi || (a.hi == b.hi && a.lo <= b.lo));
}

bool operator<=(double a, const DoubleDouble &b) {
  return (b >= a);
}

bool operator!=(const DoubleDouble &a, double b) {
  return (a.hi != b || a.lo != 0.0);
}

bool operator!=(const DoubleDouble &a, const DoubleDouble &b) {
  return (a.hi != b.hi || a.lo != b.lo);
}

bool operator!=(double a, const DoubleDouble &b) {
  return (a != b.hi || b.lo != 0.0);
}

bool DoubleDouble::is_zero() const {
  return (hi == 0.0);
}

bool DoubleDouble::is_negative() const {
  return (hi < 0.0);
}

bool DoubleDouble::is_positive() const {
  return (hi > 0.0);
}

DoubleDouble floor(const DoubleDouble &a) {
  double hi = floor(a.hi);
  double lo = 0.0;

  if (hi == a.hi) {
    lo = floor(a.lo);
    hi = two_sum(hi, lo, lo);
  }

  return DoubleDouble(hi, lo);
}

DoubleDouble ceil(const DoubleDouble &a) {
  double hi = ceil(a.hi);
  double lo = 0.0;

  if (hi == a.hi) {
    lo = ceil(a.lo);
    hi = two_sum(hi, lo, lo);
  }

  return DoubleDouble(hi, lo);
}

DoubleDouble aint(const DoubleDouble &a) {
  return (a.hi >= 0.0) ? floor(a) : ceil(a);
}

DoubleDouble::operator double() const {
  return hi;
}

DoubleDouble::operator int() const {
  return (int) hi;
}

DoubleDouble::DoubleDouble(const char *s) {
  DoubleDouble::read(s, *this);
}

DoubleDouble &DoubleDouble::operator=(const char *s) {
  DoubleDouble::read(s, *this);
  return *this;
}

std::ostream &operator<<(std::ostream &s, const DoubleDouble &a) {
  char str[43]; // sign + 32 digits + '.' + '\0' + 8 for sprintf

  a.write(str);
  return s << str;
}

std::istream &operator>>(std::istream &s, DoubleDouble &a) {
  char str[43];
  s >> str;
  a = DoubleDouble(str);
  return s;
}


static const char *digits = "0123456789";

void DoubleDouble::write(char *s, int d) const {

  int D = d + 1;
  DoubleDouble r = abs(*this);
  DoubleDouble p;
  int e;
  int i, j;

  if (hi == 0.0) {
    s[0] = digits[0];
    s[1] = '\0';
    return;
  }

  int *a = new int[D];

  e = (int) std::floor(std::log10(std::abs(hi)));
  p = DoubleDouble(10.0) ^ e;
  
  r /= p;
  if (r >= 10.0) {
    r /= 10.0;
    e++;
  } else if (r < 1.0) {
    r *= 10.0;
    e--;
  }

  if (r >= 10.0 || r < 1.0) {
    cerr << "ERROR (DoubleDouble::to_str): can't compute exponent." << endl;
    delete [] a;
    std::exit(-1);
  }

  for (i = 0; i < D; i++) {
    a[i] = (int) r.hi;
    r = r - (double) a[i];
    r *= 10.0;
  }

  for (i = D-1; i > 0; i--) {
    if (a[i] < 0) {
      a[i-1]--;
      a[i] += 10;
    }
  }

  if (a[0] <= 0) {
    cerr << "ERROR (DoubleDouble::to_str): non-positive leading digit." << endl;
    delete [] a;
    std::exit(-1);
  }

  if (a[D-1] >= 5) {
    a[D-2]++;

    i = D-2;
    while (i > 0 && a[i] >= 10) {
      a[i] -= 10;
      a[--i]++;
    }

  }

  i = 0;
  if (hi < 0.0)
    s[i++] = '-';

  if (a[0] >= 10) {
    s[i++] = digits[1];
    s[i++] = '.';
    s[i++] = digits[0];
    e++;
  } else {
    s[i++] = digits[a[0]];
    s[i++] = '.';
  }

  for (j=1; j < D-1; j++, i++)
    s[i] = digits[a[j]];
  
  s[i++] = 'E';
  std::sprintf(&s[i], "%d", e);

  delete [] a;
}

int DoubleDouble::read(const char *s, DoubleDouble &a) {
  const char *p = s;
  char ch;
  int sign = 0;
  int point = -1;
  int nd = 0;
  int e = 0;
  bool done = false;
  DoubleDouble r = DoubleDouble(0.0);
  int nread;
  
  while (*p == ' ')
    p++;

  while (!done && (ch = *p) != '\0') {
    if (ch >= '0' && ch <= '9') {
      int d = ch - '0';
      r *= 10.0;
      r += (double) d;
      nd++;
    } else {

      switch (ch) {

      case '.':
        if (point >= 0)
          return -1;        
        point = nd;
        break;

      case '-':
      case '+':
        if (sign != 0 || nd > 0)
          return -1;
        sign = (ch == '-') ? -1 : 1;
        break;

      case 'E':
      case 'e':
        nread = sscanf(p+1, "%d", &e);
        done = true;
        if (nread != 1)
          return -1;
        break;

      default:
        return -1;
      }
    }
    
    p++;
  }

  if (point >= 0) {
    e -= (nd - point);
  }

  if (e != 0) {
    r *= (DoubleDouble(10.0) ^ e);
  }

  a = (sign == -1) ? -r : r;
  return 0;
}

void DoubleDouble::print_components() const {
  printf("[ %.19e %.19e ]\n", hi, lo);
}

const DoubleDouble DoubleDouble::_2PI = DoubleDouble(6.283185307179586232e+00,
                                                     2.449293598294706414e-16);
const DoubleDouble DoubleDouble::_PI = DoubleDouble(3.141592653589793116e+00,
                                                    1.224646799147353207e-16);
const DoubleDouble DoubleDouble::_PI2 = DoubleDouble(1.570796326794896558e+00,
                                                     6.123233995736766036e-17);
const DoubleDouble DoubleDouble::_PI4 = DoubleDouble(7.853981633974482790e-01,
                                                     3.061616997868383018e-17);                                                   

// Квадратный корень
// Кидает ошибку если a отрицательное
DoubleDouble sqrt(const DoubleDouble &a) {
  // sqrt(a) = a*x + [a - (a*x)^2] * x / 2   (approx)

  if (a.is_zero())
    return DoubleDouble(0.0);

  if (a.is_negative()) {
    cerr << "ERROR (DoubleDouble::sqrt): Negative argument." << endl;
    std::exit(-1);
  }

  double x = 1.0 / std::sqrt(a.hi);
  double ax = a.hi * x;
  return DoubleDouble::add(ax, (a - DoubleDouble::square(ax)).hi * (x * 0.5));
}


void sincos_taylor(const DoubleDouble &a, DoubleDouble &sin_a, DoubleDouble &cos_a) {
  const DoubleDouble thresh = 1.0e-34 * abs(a);
  DoubleDouble term, power, denom, partial_sum, m_sqr;
  double m;

  if (a.is_zero()) {
    sin_a = 0.0;
    cos_a = 1.0;
    return;
  }

  m_sqr = -square(a);
  partial_sum = a;
  power = a;
  m = 1.0;
  denom = 1.0;
  //sqrt(1.0 - square(partial_sum)).print_components();
  do {
    power *= m_sqr;
    m += 2.0;
    denom *= (m*(m-1));
    term = power / denom;
    partial_sum += term;
    //sqrt(1.0 - square(partial_sum)).print_components();
  } while (abs(term) > thresh);

  sin_a = partial_sum;
  cos_a = sqrt(1.0 - square(partial_sum));
}


DoubleDouble cos(const DoubleDouble &a) {
  if (a.is_zero()) {
    return DoubleDouble(1.0);
  }

  // Приведем к интервалу [-2π, 2π]
  DoubleDouble t;
  divrem(a, DoubleDouble::_2PI, t);

  // Приведем к интервалу [-π, π]
  if (t > DoubleDouble::_PI) {
    t -= DoubleDouble::_2PI;
  } else if (t < -DoubleDouble::_PI) {
    t += DoubleDouble::_2PI;
  }

  // Приведем к интервалу [0, π]
  if (t.is_negative()) t = -t;

  // Приведем к интервалу [0, π/2]
  bool negate = false;
  if (t > DoubleDouble::_PI2) {
    t = DoubleDouble::_PI - t;
    negate = !negate;
  }

  // Приведем к интервалу [0, π/4]
  DoubleDouble sin_t, cos_t;
  if (t > DoubleDouble::_PI4) {
    t = DoubleDouble::_PI2 - t;
    sincos_taylor(t, cos_t, sin_t); // Меняем местами sin и cos
  } else {
    sincos_taylor(t, sin_t, cos_t);
  }

  return negate ? -cos_t : cos_t;
}

DoubleDouble sin(const DoubleDouble &a) {
  return cos(a - DoubleDouble::_PI2);
}

DoubleDouble abs(const DoubleDouble &a) {
  return (a.hi < 0.0) ? -a : a;
}

DoubleDouble atan2(const DoubleDouble &y, const DoubleDouble &x) {
  if (x.is_zero()) {
    if (y.is_zero()) {
      cerr << "ERROR (DoubleDouble::atan2): Both arguments zero." << endl;
      std::exit(-1);
      return 0.0;
    }
    return (y.is_positive()) ? DoubleDouble::_PI2 : -DoubleDouble::_PI2;
  } else if (y.is_zero()) {
    return (x.is_positive()) ? DoubleDouble(0.0) : DoubleDouble::_PI;
  }

  if (x == y) {
    return (y.is_positive()) ? DoubleDouble::_PI4 : -DoubleDouble::_PI4 * 3.;
  }

  if (x == -y) {
    return (y.is_positive()) ? DoubleDouble::_PI4 * 3. : -DoubleDouble::_PI4;
  }

  DoubleDouble r = sqrt(square(x) + square(y));
  DoubleDouble xx = x / r;
  DoubleDouble yy = y / r;
  DoubleDouble z = std::atan2((double) y, (double) x);
  DoubleDouble sin_z, cos_z;

  if (xx > yy) {
    /* z' = z + (y - sin(z)) / cos(z)  */
    sincos_taylor(z, sin_z, cos_z);
    z += (yy - sin_z) / cos_z;
  } else {
    /* z' = z - (x - cos(z)) / sin(z)  */
    sincos_taylor(z, sin_z, cos_z);
    z -= (xx - cos_z) / sin_z;
  }

  return z;
}