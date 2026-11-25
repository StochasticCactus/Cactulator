#include "value.hpp"

#include <cmath>

namespace cact {

bool Matrix::isReal() const {
  for (const auto& z : a)
    if (z.imag() != 0.0) return false;
  return true;
}

Matrix Matrix::identity(int n) {
  Matrix m(n, n);
  for (int i = 0; i < n; ++i) m(i, i) = 1.0;
  return m;
}

double Value::real(const char* what) const {
  if (!isNum()) throw CalcError(std::string("expected ") + what + ", got " + typeName());
  if (num.imag() != 0.0 && std::abs(num.imag()) > 1e-12 * std::max(1.0, std::abs(num.real())))
    throw CalcError(std::string("expected ") + what + ", got a complex number");
  return num.real();
}

long long Value::integer(const char* what) const {
  double d = real(what);
  if (!std::isfinite(d) || std::abs(d - std::round(d)) > 1e-9 * std::max(1.0, std::abs(d)))
    throw CalcError(std::string("expected ") + what);
  return (long long)std::llround(d);
}

bool Value::truthy() const {
  switch (kind) {
    case Kind::Num: return num != cplx(0.0, 0.0);
    case Kind::Mat: return !mat.a.empty();
    case Kind::Str: return !str.empty();
    default: return false;
  }
}

std::vector<cplx> Value::elements() const {
  if (isNum()) return {num};
  if (isMat()) return mat.a;
  throw CalcError(std::string("expected a list of numbers, got ") + typeName());
}

std::vector<double> Value::realElements(const char* what) const {
  std::vector<double> out;
  for (const auto& z : elements()) {
    if (z.imag() != 0.0 && std::abs(z.imag()) > 1e-12 * std::max(1.0, std::abs(z.real())))
      throw CalcError(std::string("expected ") + what + " (found a complex number)");
    out.push_back(z.real());
  }
  return out;
}

const char* Value::typeName() const {
  switch (kind) {
    case Kind::Num: return num.imag() == 0.0 ? "a number" : "a complex number";
    case Kind::Mat: return vec ? "a list" : "a matrix";
    case Kind::Str: return "text";
    default: return "nothing";
  }
}

double chopTiny(double x, double scale) {
  return std::abs(x) < 1e-14 * std::max(1.0, scale) ? 0.0 : x;
}

cplx chopTiny(cplx z) {
  double s = std::abs(z);
  double re = z.real(), im = z.imag();
  if (std::abs(im) < 1e-12 * s) im = 0.0;
  if (std::abs(re) < 1e-12 * s) re = 0.0;
  return {re, im};
}

}  // namespace cact
