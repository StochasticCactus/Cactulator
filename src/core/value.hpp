// Cactulator - core value types: numbers (complex), matrices/vectors, strings.
#pragma once

#include <complex>
#include <stdexcept>
#include <string>
#include <vector>

namespace cact {

using cplx = std::complex<double>;

// Every user-facing failure (syntax error, domain error, ...) is a CalcError.
struct CalcError : std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct Matrix {
  int rows = 0, cols = 0;
  std::vector<cplx> a;  // row-major

  Matrix() = default;
  Matrix(int r, int c, cplx fill = 0.0) : rows(r), cols(c), a(size_t(r) * size_t(c), fill) {}

  cplx& operator()(int r, int c) { return a[size_t(r) * cols + c]; }
  const cplx& operator()(int r, int c) const { return a[size_t(r) * cols + c]; }

  bool square() const { return rows == cols; }
  bool isReal() const;
  static Matrix identity(int n);
};

struct Value {
  enum class Kind { None, Num, Mat, Str };

  Kind kind = Kind::None;
  cplx num{0.0, 0.0};
  Matrix mat;
  bool vec = false;  // a 1-D list/vector (stored as an n x 1 column)
  std::string str;

  Value() = default;
  Value(double d) : kind(Kind::Num), num(d, 0.0) {}
  Value(cplx c) : kind(Kind::Num), num(c) {}

  static Value number(cplx c) { return Value(c); }
  static Value matrix(Matrix m, bool isVector = false) {
    Value v;
    v.kind = Kind::Mat;
    v.mat = std::move(m);
    v.vec = isVector;
    return v;
  }
  static Value vector(const std::vector<cplx>& xs) {
    Matrix m(int(xs.size()), 1);
    for (size_t i = 0; i < xs.size(); ++i) m.a[i] = xs[i];
    return matrix(std::move(m), true);
  }
  static Value vectorReal(const std::vector<double>& xs) {
    Matrix m(int(xs.size()), 1);
    for (size_t i = 0; i < xs.size(); ++i) m.a[i] = xs[i];
    return matrix(std::move(m), true);
  }
  static Value string(std::string s) {
    Value v;
    v.kind = Kind::Str;
    v.str = std::move(s);
    return v;
  }

  bool isNone() const { return kind == Kind::None; }
  bool isNum() const { return kind == Kind::Num; }
  bool isMat() const { return kind == Kind::Mat; }
  bool isStr() const { return kind == Kind::Str; }
  bool isRealNum() const { return isNum() && num.imag() == 0.0; }

  // Real value of a number; throws a friendly error otherwise.
  double real(const char* what = "a real number") const;
  // Integer value; throws if not an integer.
  long long integer(const char* what = "an integer") const;
  bool truthy() const;

  // Flatten a vector or matrix (or number) into a list of numbers.
  std::vector<cplx> elements() const;
  std::vector<double> realElements(const char* what = "a list of real numbers") const;

  const char* typeName() const;
};

// Treat |x| below this (relative) as zero when cleaning results for display.
double chopTiny(double x, double scale = 1.0);
cplx chopTiny(cplx z);

}  // namespace cact
