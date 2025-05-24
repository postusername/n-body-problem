#ifndef DD_H
#define DD_H

#include <iostream>



class DoubleDouble {
public:
  double hi, lo;

  DoubleDouble (double hi, double lo) : hi(hi), lo(lo) {}
  DoubleDouble () : hi(0.0), lo(0.0) {}
  DoubleDouble (double h) : hi(h), lo(0.0) {}
  DoubleDouble (int h) : hi((double)h), lo(0.0) {}
  explicit DoubleDouble (const char *s);
  explicit DoubleDouble (const double *d) : hi(d[0]), lo(d[1]) {}

  friend DoubleDouble operator+(const DoubleDouble &a, double b);
  friend DoubleDouble operator+(double a, const DoubleDouble &b);
  friend DoubleDouble operator+(const DoubleDouble &a, const DoubleDouble &b);
  static DoubleDouble add(double a, double b);

  DoubleDouble &operator+=(double a);
  DoubleDouble &operator+=(const DoubleDouble &a);

  friend DoubleDouble operator-(const DoubleDouble &a, double b);
  friend DoubleDouble operator-(double a, const DoubleDouble &b);
  friend DoubleDouble operator-(const DoubleDouble &a, const DoubleDouble &b);

  DoubleDouble &operator-=(double a);
  DoubleDouble &operator-=(const DoubleDouble &a);

  DoubleDouble operator-() const;

  friend DoubleDouble operator*(const DoubleDouble &a, double b);
  friend DoubleDouble operator*(double a, const DoubleDouble &b);
  friend DoubleDouble operator*(const DoubleDouble &a, const DoubleDouble &b);

  DoubleDouble &operator*=(double a);
  DoubleDouble &operator*=(const DoubleDouble &a);

  friend DoubleDouble operator/(const DoubleDouble &a, double b);
  friend DoubleDouble operator/(double a, const DoubleDouble &b);
  friend DoubleDouble operator/(const DoubleDouble &a, const DoubleDouble &b);
  friend DoubleDouble inv(const DoubleDouble &a);
  
  DoubleDouble &operator/=(double a);
  DoubleDouble &operator/=(const DoubleDouble &a);

  friend DoubleDouble divrem(const DoubleDouble &a, const DoubleDouble &b, DoubleDouble &r);
  friend DoubleDouble fmod(const DoubleDouble &a, const DoubleDouble &b);

  DoubleDouble &operator=(double a);
  DoubleDouble &operator=(const char *s);

  static DoubleDouble square(double d);
  friend DoubleDouble square(const DoubleDouble &a);
  friend DoubleDouble pow(const DoubleDouble &a, int n);
  DoubleDouble operator^(int n);

  friend bool operator==(const DoubleDouble &a, double b);
  friend bool operator==(double a, const DoubleDouble &b);
  friend bool operator==(const DoubleDouble &a, const DoubleDouble &b);

  friend bool operator<=(const DoubleDouble &a, double b);
  friend bool operator<=(double a, const DoubleDouble &b);
  friend bool operator<=(const DoubleDouble &a, const DoubleDouble &b);

  friend bool operator>=(const DoubleDouble &a, double b);
  friend bool operator>=(double a, const DoubleDouble &b);
  friend bool operator>=(const DoubleDouble &a, const DoubleDouble &b);

  friend bool operator<(const DoubleDouble &a, double b);
  friend bool operator<(double a, const DoubleDouble &b);
  friend bool operator<(const DoubleDouble &a, const DoubleDouble &b);

  friend bool operator>(const DoubleDouble &a, double b);
  friend bool operator>(double a, const DoubleDouble &b);
  friend bool operator>(const DoubleDouble &a, const DoubleDouble &b);

  friend bool operator!=(const DoubleDouble &a, double b);
  friend bool operator!=(double a, const DoubleDouble &b);
  friend bool operator!=(const DoubleDouble &a, const DoubleDouble &b);

  [[nodiscard]] bool is_zero() const;
  [[nodiscard]] bool is_negative() const;
  [[nodiscard]] bool is_positive() const;

  friend DoubleDouble floor(const DoubleDouble &a);
  friend DoubleDouble ceil(const DoubleDouble &a);
  friend DoubleDouble aint(const DoubleDouble &a);

  operator double() const;
  operator int() const;

  friend DoubleDouble sqrt(const DoubleDouble &a);

  static const DoubleDouble _2PI;
  static const DoubleDouble _PI;
  static const DoubleDouble _PI2;
  static const DoubleDouble _PI4;

  friend DoubleDouble cos(const DoubleDouble &a);
  friend DoubleDouble sin(const DoubleDouble &a);
  friend DoubleDouble abs(const DoubleDouble &a);
  friend DoubleDouble atan2(const DoubleDouble &y, const DoubleDouble &x);

  void write(char *s, int d = 32) const;
  int read(const char *s, DoubleDouble &a);

  friend std::ostream& operator<<(std::ostream &s, const DoubleDouble &a);
  friend std::istream& operator>>(std::istream &s, DoubleDouble &a);

  void print_components() const;
};

#endif
