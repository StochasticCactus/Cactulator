// Copyright (C) 2026 StochasticCactus
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of Cactulator.
//
// Cactulator is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// Cactulator is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along
// with Cactulator; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

// Cactulator - built-in functions and constants (with their help text).
#include <functional>
#include <memory>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <random>

#include "interp.hpp"
#include "linalg.hpp"
#include "numeric.hpp"
#include "units.hpp"

namespace cact {

namespace {

const double kPi = 3.14159265358979323846;
using Args = std::vector<Value>;

std::mt19937_64& rng() {
  static std::mt19937_64 g((unsigned long long)std::chrono::steady_clock::now().time_since_epoch().count());
  return g;
}

bool isReal(cplx z) { return z.imag() == 0.0; }

double snapAngle(double v) {
  double r = std::round(v);
  return std::abs(v - r) < 1e-10 * std::max(1.0, std::abs(r)) ? r : v;
}

// Exact sine for multiples of 30° and 45°.
bool exactSinDeg(double deg, double& out) {
  double r = std::fmod(deg, 360.0);
  if (r < 0) r += 360.0;
  static const double h2 = std::sqrt(2.0) / 2, h3 = std::sqrt(3.0) / 2;
  static const struct {
    double a, v;
  } tbl[] = {{0, 0},      {30, 0.5},   {45, h2},    {60, h3},     {90, 1},     {120, h3},   {135, h2},
             {150, 0.5},  {180, 0},    {210, -0.5}, {225, -h2},   {240, -h3},  {270, -1},   {300, -h3},
             {315, -h2},  {330, -0.5}, {360, 0}};
  for (const auto& e : tbl)
    if (r == e.a) {
      out = e.v;
      return true;
    }
  return false;
}

double snapTrig(double v, double x) {
  if (std::abs(x) > 1e-3 && std::abs(v) < 1e-15 * std::abs(x) * 4) return 0.0;
  return v;
}

cplx sinA(Interp& in, cplx z) {
  if (isReal(z)) {
    double x = z.real(), v;
    if (!std::isfinite(x)) throw CalcError("sin of infinity is undefined");
    if (in.settings.angle == AngleMode::DEG) {
      if (exactSinDeg(x, v)) return v;
      return std::sin(x * kPi / 180);
    }
    return snapTrig(std::sin(x), x);
  }
  return std::sin(in.toRadians(z));
}

cplx cosA(Interp& in, cplx z) {
  if (isReal(z)) {
    double x = z.real(), v;
    if (!std::isfinite(x)) throw CalcError("cos of infinity is undefined");
    if (in.settings.angle == AngleMode::DEG) {
      if (exactSinDeg(x + 90, v)) return v;
      return std::cos(x * kPi / 180);
    }
    return snapTrig(std::cos(x), x);
  }
  return std::cos(in.toRadians(z));
}

cplx tanA(Interp& in, cplx z) {
  if (isReal(z)) {
    double x = z.real(), s, c;
    if (in.settings.angle == AngleMode::DEG && exactSinDeg(x, s) && exactSinDeg(x + 90, c)) {
      if (c == 0) throw CalcError("tan is undefined at " + std::to_string((long long)x) + "°");
      return s / c;
    }
    double r = in.toRadians(x);
    return snapTrig(std::tan(r), r);
  }
  return std::tan(in.toRadians(z));
}

cplx recip(cplx z, const char* what) {
  if (z == cplx(0)) throw CalcError(std::string(what) + " is undefined here (division by zero)");
  return isReal(z) ? cplx(1.0 / z.real()) : 1.0 / z;
}

cplx inverseTrig(Interp& in, cplx z, double (*rf)(double), cplx (*cf)(const cplx&), bool domainCheck) {
  if (isReal(z) && (!domainCheck || std::abs(z.real()) <= 1.0)) {
    double r = in.fromRadians(rf(z.real()));
    return in.settings.angle == AngleMode::DEG ? snapAngle(r) : r;
  }
  return in.fromRadians(cf(z));
}

cplx csqrt(cplx z) {
  if (isReal(z)) {
    double x = z.real();
    return x >= 0 ? cplx(std::sqrt(x)) : cplx(0.0, std::sqrt(-x));
  }
  return std::sqrt(z);
}

cplx cln(cplx z) {
  if (isReal(z)) {
    double x = z.real();
    if (x > 0) return std::log(x);
    if (x == 0) return -INFINITY;
    return cplx(std::log(-x), kPi);
  }
  return std::log(z);
}

cplx cexp(cplx z) { return isReal(z) ? cplx(std::exp(z.real())) : std::exp(z); }

double r1(const Value& v, const char* what = "a real number") { return v.real(what); }

long long i1(const Value& v, const char* what = "a whole number") { return v.integer(what); }

Value mapR(const Value& v, double (*f)(double)) {
  return mapNumbers(v, [f](cplx z) {
    if (!isReal(z)) throw CalcError("expected a real number");
    return cplx(f(z.real()));
  });
}

std::vector<double> listArg(const Args& a, const char* fname) {
  if (a.size() == 1 && a[0].isMat()) return a[0].realElements();
  std::vector<double> out;
  for (const auto& v : a) {
    if (v.isMat()) {
      auto e = v.realElements();
      out.insert(out.end(), e.begin(), e.end());
    } else {
      out.push_back(v.real());
    }
  }
  if (out.empty()) throw CalcError(std::string(fname) + " needs at least one number");
  return out;
}

const Matrix& matArg(const Value& v, const char* fname) {
  if (!v.isMat()) throw CalcError(std::string(fname) + " needs a matrix, got " + v.typeName());
  return v.mat;
}

Matrix asSquare(const Value& v, const char* fname) {
  const Matrix& m = matArg(v, fname);
  if (v.vec && m.a.size() == 1) return Matrix(1, 1, m.a[0]);
  return m;
}

double gcdD(double a, double b) {
  a = std::abs(a);
  b = std::abs(b);
  while (b > 0.5) {
    double t = std::fmod(a, b);
    a = b;
    b = t;
  }
  return a;
}

bool isPrime(long long n) {
  if (n < 2) return false;
  if (n < 4) return true;
  if (n % 2 == 0 || n % 3 == 0) return false;
  for (long long d = 5; d * d <= n; d += 6)
    if (n % d == 0 || n % (d + 2) == 0) return false;
  return true;
}

std::string baseString(long long v, int base) {
  const char* digits = "0123456789ABCDEF";
  bool neg = v < 0;
  unsigned long long u = neg ? (unsigned long long)(-v) : (unsigned long long)v;
  std::string s;
  if (u == 0) s = "0";
  while (u) {
    s.insert(s.begin(), digits[u % base]);
    u /= base;
  }
  return std::string(neg ? "-" : "") + (base == 16 ? "0x" : base == 2 ? "0b" : "0o") + s;
}

// ---- helpers for lazy (special-form) functions

NodeP differenceNode(const NodeP& eq) {
  if (eq->k != NK::Eq) return eq;
  auto n = std::make_shared<Node>();
  n->k = NK::Binary;
  n->name = "-";
  n->pos = eq->pos;
  n->args = {eq->args[0], eq->args[1]};
  return n;
}

std::string varName(const NodeP& n, const char* fname) {
  if (n->k != NK::Var) throw CalcError(std::string(fname) + ": expected a variable name like x");
  return n->name;
}

std::string guessVariable(Interp& in, const NodeP& expr) {
  std::vector<std::string> vars, free;
  collectVars(expr, vars);
  for (const auto& v : vars) {
    Value tmp;
    if (!in.getVar(v, tmp)) free.push_back(v);
  }
  if (free.size() == 1) return free[0];
  for (const auto& v : vars)
    if (v == "x") return "x";
  if (free.empty() && vars.size() == 1) return vars[0];
  if (free.empty()) throw CalcError("which variable should I solve for? Use solve(equation, x)");
  std::string list;
  for (size_t k = 0; k < free.size(); ++k) list += (k ? ", " : "") + free[k];
  throw CalcError("the equation has several unknowns (" + list + "); give values to all but one, or use the Solve app");
}

Value lazySolve(Interp& in, const std::vector<NodeP>& a) {
  NodeP eq = a[0];
  std::string var;
  double guess = NAN;
  if (a.size() >= 2) {
    if (a[1]->k == NK::Var) {
      Value cur;
      bool has = in.getVar(a[1]->name, cur);
      if (has && cur.isMat() && a.size() == 2) {
        Value A = in.eval(a[0]);
        if (!A.isMat()) throw CalcError("solve(A, b) needs a matrix A");
        return Value::matrix(solveLinear(A.mat, cur.mat), cur.vec);
      }
      var = a[1]->name;
      if (has && cur.isRealNum()) guess = cur.num.real();
    } else {
      Value second = in.eval(a[1]);
      if (second.isMat() && a.size() == 2) {
        Value A = in.eval(a[0]);
        if (!A.isMat()) throw CalcError("solve(A, b) needs a matrix A");
        return Value::matrix(solveLinear(A.mat, second.mat), second.vec);
      }
      var = guessVariable(in, eq);
      guess = second.real("a starting guess");
    }
  } else {
    var = guessVariable(in, eq);
    Value cur;
    if (in.getVar(var, cur) && cur.isRealNum()) guess = cur.num.real();
  }
  if (a.size() == 3) guess = in.eval(a[2]).real("a starting guess");
  if (std::isnan(guess)) guess = 1;
  NodeP expr = differenceNode(eq);
  in.probe(expr, var, guess);
  auto f = in.realFunction(expr, var);
  double r = findRoot(f, guess);
  double rr = std::round(r);
  if (std::abs(r - rr) < 1e-12 * std::max(1.0, std::abs(rr)) && std::abs(f(rr)) <= std::abs(f(r))) r = rr;
  return Value(r);
}

// (expr, var, a, b) or (expr, a, b) with var = x
void exprVarRange(Interp& in, const std::vector<NodeP>& a, size_t nRange, const char* fname, NodeP& expr,
                  std::string& var, std::vector<double>& range) {
  expr = a[0];
  size_t start;
  if (a.size() == nRange + 2) {
    var = varName(a[1], fname);
    start = 2;
  } else if (a.size() == nRange + 1) {
    var = "x";
    start = 1;
  } else {
    throw CalcError(std::string(fname) + ": wrong number of arguments");
  }
  for (size_t k = start; k < a.size(); ++k) range.push_back(in.eval(a[k]).real("a number"));
}

Value lazyDeriv(Interp& in, const std::vector<NodeP>& a) {
  NodeP expr = a[0];
  std::string var = "x";
  double at;
  if (a.size() == 3) {
    var = varName(a[1], "deriv");
    at = in.eval(a[2]).real("a point");
  } else if (a[1]->k == NK::Var) {
    var = a[1]->name;
    Value cur;
    if (!in.getVar(var, cur) || !cur.isRealNum())
      throw CalcError("give the point too: deriv(expression, " + var + ", 2)");
    at = cur.num.real();
  } else {
    at = in.eval(a[1]).real("a point");
  }
  in.probe(expr, var, at);
  return Value(derivative(in.realFunction(expr, var), at));
}

Value lazyInteg(Interp& in, const std::vector<NodeP>& a) {
  NodeP expr;
  std::string var;
  std::vector<double> r;
  exprVarRange(in, a, 2, "integ", expr, var, r);
  double probeAt = std::isfinite(r[0]) && std::isfinite(r[1]) ? 0.5 * (r[0] + r[1]) : (std::isfinite(r[0]) ? r[0] + 1 : (std::isfinite(r[1]) ? r[1] - 1 : 0));
  in.probe(expr, var, probeAt);
  return Value(integrate(in.realFunction(expr, var), r[0], r[1]));
}

Value seriesOp(Interp& in, const std::vector<NodeP>& a, bool product) {
  const char* fname = product ? "prod" : "sum";
  if (a.size() == 1) {
    Value v = in.eval(a[0]);
    cplx acc = product ? 1.0 : 0.0;
    for (const auto& z : v.elements()) acc = product ? acc * z : acc + z;
    return Value(acc);
  }
  if (a.size() != 4) throw CalcError(std::string(fname) + " takes a list, or (expression, k, from, to)");
  std::string var = varName(a[1], fname);
  double from = in.eval(a[2]).real("a start value"), to = in.eval(a[3]).real("an end value");
  if (std::floor(from) != from) throw CalcError(std::string(fname) + ": the start must be a whole number");
  Interp::ScopedBinding bind(in, var, Value(from));
  cplx acc = product ? 1.0 : 0.0;
  int small = 0;
  for (double k = from; k <= to; k += 1) {
    in.tick();
    bind.set(Value(k));
    Value t = in.eval(a[0]);
    if (!t.isNum()) throw CalcError(std::string(fname) + ": each term must be a number");
    cplx prev = acc;
    acc = product ? acc * t.num : acc + t.num;
    if (std::isinf(to)) {
      if (std::abs(acc - prev) <= 1e-16 * std::abs(acc)) {
        if (++small > 20) break;
      } else {
        small = 0;
      }
      if (k - from > 2e6) throw CalcError("the series does not seem to converge");
    } else if (to - from > 5e7) {
      throw CalcError("too many terms");
    }
  }
  return Value(chopTiny(acc));
}

Value lazySeq(Interp& in, const std::vector<NodeP>& a) {
  if (a.size() < 4) throw CalcError("seq(expression, k, from, to, step)");
  std::string var = varName(a[1], "seq");
  double from = in.eval(a[2]).real(), to = in.eval(a[3]).real();
  double step = a.size() > 4 ? in.eval(a[4]).real() : 1.0;
  if (step == 0) throw CalcError("seq: step can't be 0");
  std::vector<cplx> out;
  Interp::ScopedBinding bind(in, var, Value(from));
  double eps = 1e-12 * std::abs(step);
  for (long long k = 0;; ++k) {
    double v = from + k * step;
    if (step > 0 ? v > to + eps : v < to - eps) break;
    if (out.size() > 100000) throw CalcError("seq: too many items");
    bind.set(Value(v));
    Value t = in.eval(a[0]);
    if (!t.isNum()) throw CalcError("seq: each item must be a number");
    out.push_back(t.num);
  }
  return Value::vector(out);
}

Value lazyExtremum(Interp& in, const std::vector<NodeP>& a, bool max) {
  NodeP expr;
  std::string var;
  std::vector<double> r;
  exprVarRange(in, a, 2, max ? "fmax" : "fmin", expr, var, r);
  if (!(r[0] < r[1])) throw CalcError("the interval must have from < to");
  in.probe(expr, var, 0.5 * (r[0] + r[1]));
  auto f = in.realFunction(expr, var);
  RealFn g = [&](double x) {
    double v = f(x);
    return std::isfinite(v) ? (max ? -v : v) : 1e300;
  };
  double x = snapMinimum(g, minimize(g, r[0], r[1]));
  return Value(x);
}

Value lazyPiecewise(Interp& in, const std::vector<NodeP>& a) {
  size_t k = 0;
  for (; k + 1 < a.size(); k += 2)
    if (in.eval(a[k]).truthy()) return in.eval(a[k + 1]);
  if (k < a.size()) return in.eval(a[k]);
  return Value(NAN);
}

Value distCdf2(const Args& a, double (*cdf)(double, double, double), double mu0, double s0) {
  // (x) (a, b) (x, mu, s) (a, b, mu, s)
  switch (a.size()) {
    case 1: return Value(cdf(r1(a[0]), mu0, s0));
    case 2: return Value(cdf(r1(a[1]), mu0, s0) - cdf(r1(a[0]), mu0, s0));
    case 3: return Value(cdf(r1(a[0]), r1(a[1]), r1(a[2])));
    default: return Value(cdf(r1(a[1]), r1(a[2]), r1(a[3])) - cdf(r1(a[0]), r1(a[2]), r1(a[3])));
  }
}

struct Registry {
  std::vector<Builtin> list;
  std::map<std::string, size_t> index;
  std::map<std::string, std::string> aliases;

  void add(const std::string& name, int mn, int mx, const std::string& cat, const std::string& sig,
           const std::string& help, const std::string& ex, std::function<Value(Interp&, Args&)> fn) {
    Builtin b;
    b.name = name;
    b.minArgs = mn;
    b.maxArgs = mx;
    b.category = cat;
    b.signature = sig;
    b.help = help;
    b.example = ex;
    b.fn = std::move(fn);
    index[name] = list.size();
    list.push_back(std::move(b));
  }
  void lazy(const std::string& name, int mn, int mx, const std::string& cat, const std::string& sig,
            const std::string& help, const std::string& ex,
            std::function<Value(Interp&, const std::vector<NodeP>&)> fn) {
    Builtin b;
    b.name = name;
    b.minArgs = mn;
    b.maxArgs = mx;
    b.category = cat;
    b.signature = sig;
    b.help = help;
    b.example = ex;
    b.lazy = std::move(fn);
    index[name] = list.size();
    list.push_back(std::move(b));
  }
  void alias(const std::string& a, const std::string& target) { aliases[a] = target; }

  // Scalar function applied to every element of lists/matrices.
  void scalar(const std::string& name, const std::string& cat, const std::string& sig, const std::string& help,
              const std::string& ex, std::function<cplx(Interp&, cplx)> f) {
    add(name, 1, 1, cat, sig, help, ex, [f](Interp& in, Args& a) {
      return mapNumbers(a[0], [&](cplx z) { return f(in, z); });
    });
  }
};

Registry buildRegistry() {
  Registry R;
  const std::string BASIC = "Basic", TRIG = "Trigonometry", CPLX = "Complex numbers", NUM = "Number theory",
                    PROB = "Probability", STAT = "Statistics & lists", MAT = "Matrices", CALC = "Calculus & solving",
                    UNIT = "Units", PROG = "Programs", RAND = "Random";

  // ------------------------------------------------------------- basic
  R.scalar("sqrt", BASIC, "sqrt(x)", "Square root. Negative numbers give imaginary results.", "sqrt(16) = 4",
           [](Interp&, cplx z) { return csqrt(z); });
  R.scalar("cbrt", BASIC, "cbrt(x)", "Cube root (real for negative numbers).", "cbrt(-8) = -2", [](Interp&, cplx z) {
    return isReal(z) ? cplx(std::cbrt(z.real())) : powc(z, 1.0 / 3.0);
  });
  R.add("root", 2, 2, BASIC, "root(x, n)", "The n-th root of x.", "root(32, 5) = 2", [](Interp&, Args& a) {
    cplx n = a[1].num;
    return mapNumbers(a[0], [n](cplx z) {
      if (isReal(z) && isReal(n) && z.real() < 0 && std::fmod(std::abs(n.real()), 2.0) == 1.0)
        return cplx(-std::pow(-z.real(), 1.0 / n.real()));
      return powc(z, 1.0 / n);
    });
  });
  R.scalar("sq", BASIC, "sq(x)", "x squared (x²).", "sq(12) = 144", [](Interp&, cplx z) { return powc(z, 2.0); });
  R.scalar("exp", BASIC, "exp(x)", "e raised to the power x.", "exp(1) = 2.71828182846",
           [](Interp&, cplx z) { return cexp(z); });
  R.scalar("ln", BASIC, "ln(x)", "Natural logarithm (base e).", "ln(e^2) = 2", [](Interp&, cplx z) { return cln(z); });
  R.add("log", 1, 2, BASIC, "log(x) or log(x, base)", "Logarithm, base 10 unless you give a base.",
        "log(1000) = 3,  log(8, 2) = 3", [](Interp&, Args& a) {
          if (a.size() == 1)
            return mapNumbers(a[0], [](cplx z) {
              cplx r = cln(z) / std::log(10.0);
              if (isReal(z) && z.real() > 0) {
                double l = std::log10(z.real());
                return cplx(l);
              }
              return r;
            });
          cplx lb = cln(a[1].num);
          if (lb == cplx(0)) throw CalcError("log base can't be 1");
          return mapNumbers(a[0], [lb](cplx z) {
            cplx r = cln(z) / lb;
            double rr = std::round(r.real());
            if (isReal(r) && std::abs(r.real() - rr) < 1e-13 * std::max(1.0, std::abs(rr))) r = rr;
            return r;
          });
        });
  R.scalar("log10", BASIC, "log10(x)", "Base-10 logarithm.", "log10(0.01) = -2",
           [](Interp&, cplx z) { return isReal(z) && z.real() > 0 ? cplx(std::log10(z.real())) : cln(z) / std::log(10.0); });
  R.scalar("log2", BASIC, "log2(x)", "Base-2 logarithm.", "log2(1024) = 10",
           [](Interp&, cplx z) { return isReal(z) && z.real() > 0 ? cplx(std::log2(z.real())) : cln(z) / std::log(2.0); });
  R.scalar("abs", BASIC, "abs(x)", "Absolute value (distance from 0; modulus for complex numbers).", "abs(-7) = 7",
           [](Interp&, cplx z) { return cplx(std::abs(z)); });
  R.scalar("sign", BASIC, "sign(x)", "-1, 0 or 1 depending on the sign of x.", "sign(-3.2) = -1", [](Interp&, cplx z) {
    if (!isReal(z)) return z / std::abs(z);
    return cplx(z.real() > 0 ? 1.0 : z.real() < 0 ? -1.0 : 0.0);
  });
  R.scalar("floor", BASIC, "floor(x)", "Round down to a whole number.", "floor(2.7) = 2",
           [](Interp&, cplx z) { return cplx(std::floor(z.real()), std::floor(z.imag())); });
  R.scalar("ceil", BASIC, "ceil(x)", "Round up to a whole number.", "ceil(2.1) = 3",
           [](Interp&, cplx z) { return cplx(std::ceil(z.real()), std::ceil(z.imag())); });
  R.add("round", 1, 2, BASIC, "round(x) or round(x, digits)", "Round to the nearest whole number or to some decimals.",
        "round(3.14159, 2) = 3.14", [](Interp&, Args& a) {
          double p = a.size() > 1 ? std::pow(10.0, double(i1(a[1], "a number of digits"))) : 1.0;
          return mapNumbers(a[0], [p](cplx z) {
            return cplx(std::round(z.real() * p) / p, std::round(z.imag() * p) / p);
          });
        });
  R.scalar("trunc", BASIC, "trunc(x)", "Drop the decimals (round toward zero). Also: ipart.", "trunc(-2.7) = -2",
           [](Interp&, cplx z) { return cplx(std::trunc(z.real()), std::trunc(z.imag())); });
  R.alias("ipart", "trunc");
  R.scalar("fpart", BASIC, "fpart(x)", "The decimal part of x.", "fpart(3.25) = 0.25",
           [](Interp&, cplx z) { return cplx(z.real() - std::trunc(z.real())); });
  R.add("mod", 2, 2, BASIC, "mod(a, b)", "Remainder of a divided by b (also a % b).", "mod(17, 5) = 2",
        [](Interp&, Args& a) { return binaryOp("%", a[0], a[1]); });
  R.add("inv", 1, 1, BASIC, "inv(x)", "Reciprocal 1/x, or the inverse of a matrix.", "inv(4) = 0.25,  inv([[1,2],[3,4]])",
        [](Interp&, Args& a) {
          if (a[0].isMat() && !a[0].vec) return Value::matrix(inverse(a[0].mat));
          return binaryOp("/", Value(1.0), a[0]);
        });
  R.add("neg", 1, 1, BASIC, "neg(x)", "Change the sign of x.", "neg(5) = -5", [](Interp&, Args& a) { return negate(a[0]); });
  R.add("hypot", 2, 2, BASIC, "hypot(a, b)", "Length of the hypotenuse: √(a² + b²).", "hypot(3, 4) = 5",
        [](Interp&, Args& a) { return Value(std::hypot(r1(a[0]), r1(a[1]))); });
  R.add("min", 1, -1, BASIC, "min(a, b, …) or min(list)", "The smallest value.", "min(4, 9, 2) = 2", [](Interp&, Args& a) {
    auto xs = listArg(a, "min");
    return Value(*std::min_element(xs.begin(), xs.end()));
  });
  R.add("max", 1, -1, BASIC, "max(a, b, …) or max(list)", "The largest value.", "max([4, 9, 2]) = 9", [](Interp&, Args& a) {
    auto xs = listArg(a, "max");
    return Value(*std::max_element(xs.begin(), xs.end()));
  });
  R.add("percent", 2, 2, BASIC, "percent(p, x)", "p percent of x.", "percent(15, 80) = 12",
        [](Interp&, Args& a) { return binaryOp("*", binaryOp("/", a[0], Value(100.0)), a[1]); });

  // -------------------------------------------------------------- trig
  R.scalar("sin", TRIG, "sin(x)", "Sine. Uses degrees or radians depending on the mode (DEG/RAD).",
           "sin(30) = 0.5 in DEG mode", sinA);
  R.scalar("cos", TRIG, "cos(x)", "Cosine (angle in the current mode).", "cos(60) = 0.5 in DEG mode", cosA);
  R.scalar("tan", TRIG, "tan(x)", "Tangent (angle in the current mode).", "tan(45) = 1 in DEG mode", tanA);
  R.scalar("sec", TRIG, "sec(x)", "Secant, 1/cos(x).", "sec(60) = 2 in DEG mode",
           [](Interp& in, cplx z) { return recip(cosA(in, z), "sec"); });
  R.scalar("csc", TRIG, "csc(x)", "Cosecant, 1/sin(x).", "csc(30) = 2 in DEG mode",
           [](Interp& in, cplx z) { return recip(sinA(in, z), "csc"); });
  R.scalar("cot", TRIG, "cot(x)", "Cotangent, 1/tan(x).", "cot(45) = 1 in DEG mode",
           [](Interp& in, cplx z) { return recip(tanA(in, z), "cot"); });
  R.scalar("asin", TRIG, "asin(x)", "Inverse sine (arcsin): the angle whose sine is x.", "asin(0.5) = 30 in DEG mode",
           [](Interp& in, cplx z) {
             return inverseTrig(in, z, [](double v) { return std::asin(v); }, [](const cplx& c) { return std::asin(c); }, true);
           });
  R.scalar("acos", TRIG, "acos(x)", "Inverse cosine (arccos).", "acos(0) = 90 in DEG mode", [](Interp& in, cplx z) {
    return inverseTrig(in, z, [](double v) { return std::acos(v); }, [](const cplx& c) { return std::acos(c); }, true);
  });
  R.scalar("atan", TRIG, "atan(x)", "Inverse tangent (arctan).", "atan(1) = 45 in DEG mode", [](Interp& in, cplx z) {
    return inverseTrig(in, z, [](double v) { return std::atan(v); }, [](const cplx& c) { return std::atan(c); }, false);
  });
  R.alias("arcsin", "asin");
  R.alias("arccos", "acos");
  R.alias("arctan", "atan");
  R.add("atan2", 2, 2, TRIG, "atan2(y, x)", "Angle of the point (x, y) from the positive x axis.",
        "atan2(1, -1) = 135 in DEG mode", [](Interp& in, Args& a) {
          double r = in.fromRadians(std::atan2(r1(a[0]), r1(a[1])));
          return Value(in.settings.angle == AngleMode::DEG ? snapAngle(r) : r);
        });
  R.scalar("sinh", TRIG, "sinh(x)", "Hyperbolic sine.", "sinh(0) = 0",
           [](Interp&, cplx z) { return isReal(z) ? cplx(std::sinh(z.real())) : std::sinh(z); });
  R.scalar("cosh", TRIG, "cosh(x)", "Hyperbolic cosine.", "cosh(0) = 1",
           [](Interp&, cplx z) { return isReal(z) ? cplx(std::cosh(z.real())) : std::cosh(z); });
  R.scalar("tanh", TRIG, "tanh(x)", "Hyperbolic tangent.", "tanh(0) = 0",
           [](Interp&, cplx z) { return isReal(z) ? cplx(std::tanh(z.real())) : std::tanh(z); });
  R.scalar("asinh", TRIG, "asinh(x)", "Inverse hyperbolic sine.", "asinh(0) = 0",
           [](Interp&, cplx z) { return isReal(z) ? cplx(std::asinh(z.real())) : std::asinh(z); });
  R.scalar("acosh", TRIG, "acosh(x)", "Inverse hyperbolic cosine.", "acosh(1) = 0", [](Interp&, cplx z) {
    return isReal(z) && z.real() >= 1 ? cplx(std::acosh(z.real())) : std::acosh(z);
  });
  R.scalar("atanh", TRIG, "atanh(x)", "Inverse hyperbolic tangent.", "atanh(0) = 0", [](Interp&, cplx z) {
    return isReal(z) && std::abs(z.real()) < 1 ? cplx(std::atanh(z.real())) : std::atanh(z);
  });
  R.scalar("todeg", TRIG, "todeg(x)", "Convert radians to degrees.", "todeg(pi) = 180",
           [](Interp&, cplx z) { return z * (180 / kPi); });
  R.scalar("torad", TRIG, "torad(x)", "Convert degrees to radians.", "torad(180) = 3.14159265359",
           [](Interp&, cplx z) { return z * (kPi / 180); });

  // ----------------------------------------------------------- complex
  R.scalar("re", CPLX, "re(z)", "Real part of a complex number.", "re(3+4i) = 3",
           [](Interp&, cplx z) { return cplx(z.real()); });
  R.scalar("im", CPLX, "im(z)", "Imaginary part of a complex number.", "im(3+4i) = 4",
           [](Interp&, cplx z) { return cplx(z.imag()); });
  R.scalar("conj", CPLX, "conj(z)", "Complex conjugate.", "conj(3+4i) = 3-4i", [](Interp&, cplx z) { return std::conj(z); });
  R.scalar("arg", CPLX, "arg(z)", "Angle of a complex number (in the current angle mode).", "arg(1+i) = 45 in DEG mode",
           [](Interp& in, cplx z) { return cplx(in.fromRadians(std::arg(z))); });
  R.add("polar", 1, 1, CPLX, "polar(z)", "Polar form [r, θ] of a complex number.", "polar(3+4i) = [5, 53.13…]",
        [](Interp& in, Args& a) {
          cplx z = a[0].num;
          return Value::vectorReal({std::abs(z), in.fromRadians(std::arg(z))});
        });
  R.add("rect", 2, 2, CPLX, "rect(r, θ)", "Complex number from polar form r·e^(iθ).", "rect(2, 90) = 2i in DEG mode",
        [](Interp& in, Args& a) {
          double r = r1(a[0]), th = r1(a[1]);
          cplx z(r * std::real(cosA(in, th)), r * std::real(sinA(in, th)));
          return Value(chopTiny(z));
        });

  // ------------------------------------------------------ number theory
  R.add("gcd", 2, -1, NUM, "gcd(a, b, …)", "Greatest common divisor.", "gcd(12, 18) = 6", [](Interp&, Args& a) {
    auto xs = listArg(a, "gcd");
    double g = 0;
    for (double x : xs) {
      if (x != std::floor(x)) throw CalcError("gcd needs whole numbers");
      g = gcdD(g, x);
    }
    return Value(g);
  });
  R.add("lcm", 2, -1, NUM, "lcm(a, b, …)", "Least common multiple.", "lcm(4, 6) = 12", [](Interp&, Args& a) {
    auto xs = listArg(a, "lcm");
    double l = 1;
    for (double x : xs) {
      if (x != std::floor(x)) throw CalcError("lcm needs whole numbers");
      if (x == 0) return Value(0.0);
      l = std::abs(l / gcdD(l, x) * x);
    }
    return Value(l);
  });
  R.scalar("fact", NUM, "fact(n)", "Factorial n! = 1·2·3·…·n (also written n!).", "fact(5) = 120", [](Interp&, cplx z) {
    double x = z.real();
    if (x < 0 && x == std::floor(x)) throw CalcError("factorial of a negative integer is undefined");
    if (x == std::floor(x) && x <= 170) {
      double r = 1;
      for (int k = 2; k <= (int)x; ++k) r *= k;
      return cplx(r);
    }
    return cplx(std::tgamma(x + 1));
  });
  R.alias("factorial", "fact");
  R.add("nCr", 2, 2, NUM, "nCr(n, r)", "Combinations: ways to choose r items from n (order doesn't matter).",
        "nCr(5, 2) = 10", [](Interp&, Args& a) {
          long long n = i1(a[0]), r = i1(a[1]);
          if (r < 0 || r > n) return Value(0.0);
          double v = std::exp(std::lgamma(n + 1.0) - std::lgamma(r + 1.0) - std::lgamma(n - r + 1.0));
          return Value(v < 9e15 ? std::round(v) : v);
        });
  R.alias("comb", "nCr");
  R.alias("choose", "nCr");
  R.add("nPr", 2, 2, NUM, "nPr(n, r)", "Permutations: ordered ways to pick r items from n.", "nPr(5, 2) = 20",
        [](Interp&, Args& a) {
          long long n = i1(a[0]), r = i1(a[1]);
          if (r < 0 || r > n) return Value(0.0);
          double v = std::exp(std::lgamma(n + 1.0) - std::lgamma(n - r + 1.0));
          return Value(v < 9e15 ? std::round(v) : v);
        });
  R.alias("perm", "nPr");
  R.add("isprime", 1, 1, NUM, "isprime(n)", "1 if n is a prime number, otherwise 0.", "isprime(97) = 1",
        [](Interp&, Args& a) { return Value(isPrime(i1(a[0])) ? 1.0 : 0.0); });
  R.add("factor", 1, 1, NUM, "factor(n)", "Prime factors of a whole number.", "factor(360) = [2, 2, 2, 3, 3, 5]",
        [](Interp&, Args& a) {
          long long n = i1(a[0]);
          if (n < 2) throw CalcError("factor needs a whole number ≥ 2");
          if (n > 9007199254740991LL) throw CalcError("number too large to factor");
          std::vector<double> fs;
          for (long long d = 2; d * d <= n; ++d)
            while (n % d == 0) {
              fs.push_back(double(d));
              n /= d;
            }
          if (n > 1) fs.push_back(double(n));
          return Value::vectorReal(fs);
        });
  R.add("tofrac", 1, 1, NUM, "tofrac(x)", "Show a decimal as a fraction (text).", "tofrac(0.375) = 3/8", [](Interp&, Args& a) {
    double x = r1(a[0]);
    long long p, q;
    if (x == std::floor(x)) return Value::string(std::to_string((long long)x));
    if (toFraction(x, p, q, 1000000, 1e-10)) return Value::string(std::to_string(p) + "/" + std::to_string(q));
    throw CalcError("no simple fraction found");
  });
  R.add("hex", 1, 1, NUM, "hex(n)", "Write a whole number in hexadecimal (base 16).", "hex(255) = 0xFF",
        [](Interp&, Args& a) { return Value::string(baseString(i1(a[0]), 16)); });
  R.add("bin", 1, 1, NUM, "bin(n)", "Write a whole number in binary (base 2).", "bin(10) = 0b1010",
        [](Interp&, Args& a) { return Value::string(baseString(i1(a[0]), 2)); });
  R.add("oct", 1, 1, NUM, "oct(n)", "Write a whole number in octal (base 8).", "oct(64) = 0o100",
        [](Interp&, Args& a) { return Value::string(baseString(i1(a[0]), 8)); });
  R.add("band", 2, 2, NUM, "band(a, b)", "Bitwise AND of two whole numbers.", "band(12, 10) = 8",
        [](Interp&, Args& a) { return Value(double(i1(a[0]) & i1(a[1]))); });
  R.add("bor", 2, 2, NUM, "bor(a, b)", "Bitwise OR.", "bor(12, 10) = 14",
        [](Interp&, Args& a) { return Value(double(i1(a[0]) | i1(a[1]))); });
  R.add("bxor", 2, 2, NUM, "bxor(a, b)", "Bitwise exclusive OR.", "bxor(12, 10) = 6",
        [](Interp&, Args& a) { return Value(double(i1(a[0]) ^ i1(a[1]))); });
  R.add("shl", 2, 2, NUM, "shl(a, n)", "Shift bits left by n (multiply by 2ⁿ).", "shl(1, 4) = 16",
        [](Interp&, Args& a) { return Value(double(i1(a[0]) << i1(a[1]))); });
  R.add("shr", 2, 2, NUM, "shr(a, n)", "Shift bits right by n.", "shr(256, 4) = 16",
        [](Interp&, Args& a) { return Value(double(i1(a[0]) >> i1(a[1]))); });

  // ------------------------------------------------------------ random
  R.add("rand", 0, 1, RAND, "rand() or rand(n)", "Random number between 0 and 1 (or a list of n of them).",
        "rand() = 0.4170…", [](Interp&, Args& a) {
          std::uniform_real_distribution<double> d(0.0, 1.0);
          if (a.empty()) return Value(d(rng()));
          long long n = i1(a[0]);
          std::vector<double> v;
          for (long long k = 0; k < n; ++k) v.push_back(d(rng()));
          return Value::vectorReal(v);
        });
  R.add("randint", 2, 3, RAND, "randint(a, b) or randint(a, b, n)", "Random whole number from a to b (inclusive).",
        "randint(1, 6) = a dice roll", [](Interp&, Args& a) {
          long long lo = i1(a[0]), hi = i1(a[1]);
          if (lo > hi) std::swap(lo, hi);
          std::uniform_int_distribution<long long> d(lo, hi);
          if (a.size() < 3) return Value(double(d(rng())));
          std::vector<double> v;
          for (long long k = 0, n = i1(a[2]); k < n; ++k) v.push_back(double(d(rng())));
          return Value::vectorReal(v);
        });
  R.add("randn", 0, 3, RAND, "randn() or randn(μ, σ) or randn(μ, σ, n)", "Random number from a normal distribution.",
        "randn(100, 15)", [](Interp&, Args& a) {
          double mu = a.size() > 0 ? r1(a[0]) : 0, s = a.size() > 1 ? r1(a[1]) : 1;
          std::normal_distribution<double> d(mu, s);
          if (a.size() < 3) return Value(d(rng()));
          std::vector<double> v;
          for (long long k = 0, n = i1(a[2]); k < n; ++k) v.push_back(d(rng()));
          return Value::vectorReal(v);
        });

  // ------------------------------------------------------ stats & lists
  R.lazy("sum", 1, 4, STAT, "sum(list) or sum(expr, k, from, to)",
         "Add up a list, or a series Σ expr for k = from…to (to can be inf).", "sum([1,2,3]) = 6,  sum(1/k^2, k, 1, inf)",
         [](Interp& in, const std::vector<NodeP>& a) { return seriesOp(in, a, false); });
  R.lazy("prod", 1, 4, STAT, "prod(list) or prod(expr, k, from, to)", "Multiply a list, or a product Π expr for k = from…to.",
         "prod([1,2,3,4]) = 24", [](Interp& in, const std::vector<NodeP>& a) { return seriesOp(in, a, true); });
  R.add("mean", 1, -1, STAT, "mean(list)", "Average of the values. Also: avg.", "mean([2, 4, 9]) = 5",
        [](Interp&, Args& a) { return Value(mean(listArg(a, "mean"))); });
  R.alias("avg", "mean");
  R.alias("average", "mean");
  R.add("median", 1, -1, STAT, "median(list)", "The middle value.", "median([3, 1, 7]) = 3",
        [](Interp&, Args& a) { return Value(median(listArg(a, "median"))); });
  R.add("mode", 1, -1, STAT, "mode(list)", "The most common value(s).", "mode([1, 2, 2, 3]) = 2", [](Interp&, Args& a) {
    auto m = modes(listArg(a, "mode"));
    if (m.empty()) throw CalcError("no value repeats, so there is no mode");
    if (m.size() == 1) return Value(m[0]);
    return Value::vectorReal(m);
  });
  R.add("sd", 1, -1, STAT, "sd(list)", "Sample standard deviation (divides by n−1). Also: stdev.", "sd([2,4,4,4,5,5,7,9]) = 2.138…",
        [](Interp&, Args& a) { return Value(std::sqrt(variance(listArg(a, "sd"), true))); });
  R.alias("stdev", "sd");
  R.add("var", 1, -1, STAT, "var(list)", "Sample variance (divides by n−1).", "var([1, 2, 3, 4]) = 1.666…",
        [](Interp&, Args& a) { return Value(variance(listArg(a, "var"), true)); });
  R.add("psd", 1, -1, STAT, "psd(list)", "Population standard deviation σ (divides by n).", "psd([2,4,4,4,5,5,7,9]) = 2",
        [](Interp&, Args& a) { return Value(std::sqrt(variance(listArg(a, "psd"), false))); });
  R.add("pvar", 1, -1, STAT, "pvar(list)", "Population variance (divides by n).", "pvar([1, 2, 3, 4]) = 1.25",
        [](Interp&, Args& a) { return Value(variance(listArg(a, "pvar"), false)); });
  R.add("percentile", 2, 2, STAT, "percentile(list, p)", "The value below which p percent of the data falls.",
        "percentile([1,2,3,4,5], 50) = 3",
        [](Interp&, Args& a) { return Value(quantile(a[0].realElements(), r1(a[1]) / 100.0)); });
  R.add("sort", 1, 1, STAT, "sort(list)", "Sort a list from smallest to largest.", "sort([3, 1, 2]) = [1, 2, 3]",
        [](Interp&, Args& a) {
          auto xs = a[0].realElements();
          std::sort(xs.begin(), xs.end());
          return Value::vectorReal(xs);
        });
  R.add("rsort", 1, 1, STAT, "rsort(list)", "Sort a list from largest to smallest.", "rsort([3, 1, 2]) = [3, 2, 1]",
        [](Interp&, Args& a) {
          auto xs = a[0].realElements();
          std::sort(xs.rbegin(), xs.rend());
          return Value::vectorReal(xs);
        });
  R.add("rev", 1, 1, STAT, "rev(list)", "Reverse the order of a list.", "rev([1, 2, 3]) = [3, 2, 1]", [](Interp&, Args& a) {
    auto xs = a[0].elements();
    std::reverse(xs.begin(), xs.end());
    return Value::vector(xs);
  });
  R.add("len", 1, 1, STAT, "len(list)", "Number of items in a list (or entries in a matrix). Also: count.",
        "len([5, 6, 7]) = 3", [](Interp&, Args& a) {
          if (a[0].isStr()) return Value(double(a[0].str.size()));
          return Value(double(a[0].elements().size()));
        });
  R.alias("count", "len");
  R.alias("length", "len");
  R.add("cumsum", 1, 1, STAT, "cumsum(list)", "Running totals of a list.", "cumsum([1, 2, 3]) = [1, 3, 6]",
        [](Interp&, Args& a) {
          auto xs = a[0].elements();
          for (size_t k = 1; k < xs.size(); ++k) xs[k] += xs[k - 1];
          return Value::vector(xs);
        });
  R.add("range", 2, 3, STAT, "range(from, to) or range(from, to, step)", "A list of evenly spaced numbers.",
        "range(1, 5) = [1, 2, 3, 4, 5]", [](Interp&, Args& a) {
          double from = r1(a[0]), to = r1(a[1]), step = a.size() > 2 ? r1(a[2]) : (to >= from ? 1 : -1);
          if (step == 0) throw CalcError("step can't be 0");
          std::vector<double> v;
          double eps = 1e-12 * std::abs(step);
          for (long long k = 0;; ++k) {
            double x = from + k * step;
            if (step > 0 ? x > to + eps : x < to - eps) break;
            if (v.size() > 1000000) throw CalcError("list too long");
            v.push_back(x);
          }
          return Value::vectorReal(v);
        });
  R.lazy("seq", 4, 5, STAT, "seq(expr, k, from, to, step)", "Build a list by evaluating expr for each k.",
         "seq(k^2, k, 1, 5) = [1, 4, 9, 16, 25]", lazySeq);
  R.add("concat", 2, -1, STAT, "concat(list1, list2, …)", "Join lists together.", "concat([1, 2], [3]) = [1, 2, 3]",
        [](Interp&, Args& a) {
          std::vector<cplx> out;
          for (const auto& v : a) {
            auto e = v.elements();
            out.insert(out.end(), e.begin(), e.end());
          }
          return Value::vector(out);
        });
  R.add("linreg", 2, 2, STAT, "linreg(xs, ys)", "Best-fit line y = m·x + b. Returns [m, b].",
        "linreg([1,2,3], [2,4,6.1]) = [2.05, -0.0333…]",
        [](Interp&, Args& a) { return Value::vectorReal(polyfit(a[0].realElements(), a[1].realElements(), 1)); });
  R.add("corr", 2, 2, STAT, "corr(xs, ys)", "Correlation coefficient r (−1…1).", "corr([1,2,3], [2,4,7]) = 0.993…",
        [](Interp&, Args& a) { return Value(correlation(a[0].realElements(), a[1].realElements())); });
  R.add("polyfit", 3, 3, STAT, "polyfit(xs, ys, degree)", "Best-fit polynomial; returns coefficients, highest power first.",
        "polyfit([0,1,2], [1,2,5], 2) = [1, 0, 1]", [](Interp&, Args& a) {
          return Value::vectorReal(polyfit(a[0].realElements(), a[1].realElements(), int(i1(a[2]))));
        });
  R.add("polyval", 2, 2, STAT, "polyval(coeffs, x)", "Evaluate a polynomial (coefficients highest power first).",
        "polyval([1, 0, -2], 3) = 7", [](Interp&, Args& a) {
          auto c = a[0].elements();
          return mapNumbers(a[1], [&c](cplx x) { return polyEval(c, x); });
        });
  auto regFn = [&R, STAT](const std::string& name, const std::string& model, const std::string& help,
                          const std::string& ex) {
    R.add(name, 2, 2, STAT, name + "(xs, ys)", help, ex, [model](Interp&, Args& a) {
      Fit f = fitModel(model, a[0].realElements(), a[1].realElements());
      return Value::vectorReal(f.coef);
    });
  };
  regFn("expreg", "exponential", "Exponential fit y = a·e^(b·x). Returns [a, b].", "expreg([0,1,2], [1,2.7,7.4])");
  regFn("powreg", "power", "Power fit y = a·x^b. Returns [a, b].", "powreg([1,2,3], [2,8,18]) = [2, 2]");
  regFn("logreg", "logarithmic", "Logarithmic fit y = a + b·ln(x). Returns [a, b].", "logreg([1,2,4], [0,1,2])");

  // -------------------------------------------------------- probability
  R.add("normpdf", 1, 3, PROB, "normpdf(x) or normpdf(x, μ, σ)", "Normal distribution density (bell curve height).",
        "normpdf(0) = 0.3989…", [](Interp&, Args& a) {
          double mu = a.size() > 1 ? r1(a[1]) : 0, s = a.size() > 2 ? r1(a[2]) : 1;
          return Value(normpdf(r1(a[0]), mu, s));
        });
  R.add("normcdf", 1, 4, PROB, "normcdf(x) · normcdf(a, b) · normcdf(x, μ, σ) · normcdf(a, b, μ, σ)",
        "Normal probability: P(X ≤ x), or P(a ≤ X ≤ b) with two bounds. Default μ=0, σ=1.",
        "normcdf(-1.96, 1.96) = 0.95", [](Interp&, Args& a) { return distCdf2(a, normcdf, 0, 1); });
  R.add("invnorm", 1, 3, PROB, "invnorm(p) or invnorm(p, μ, σ)", "Inverse normal: the x with P(X ≤ x) = p.",
        "invnorm(0.975) = 1.95996…", [](Interp&, Args& a) {
          double mu = a.size() > 1 ? r1(a[1]) : 0, s = a.size() > 2 ? r1(a[2]) : 1;
          return Value(invnorm(r1(a[0]), mu, s));
        });
  R.add("binompdf", 3, 3, PROB, "binompdf(n, p, k)", "Probability of exactly k successes in n tries.",
        "binompdf(10, 0.5, 5) = 0.246…",
        [](Interp&, Args& a) { return Value(binompdf(int(i1(a[0])), r1(a[1]), int(i1(a[2])))); });
  R.add("binomcdf", 3, 3, PROB, "binomcdf(n, p, k)", "Probability of at most k successes in n tries.",
        "binomcdf(10, 0.5, 5) = 0.623…",
        [](Interp&, Args& a) { return Value(binomcdf(int(i1(a[0])), r1(a[1]), int(i1(a[2])))); });
  R.add("poisspdf", 2, 2, PROB, "poisspdf(λ, k)", "Poisson probability of exactly k events.", "poisspdf(3, 2) = 0.224…",
        [](Interp&, Args& a) { return Value(poisspdf(r1(a[0]), int(i1(a[1])))); });
  R.add("poisscdf", 2, 2, PROB, "poisscdf(λ, k)", "Poisson probability of at most k events.", "poisscdf(3, 2) = 0.423…",
        [](Interp&, Args& a) { return Value(poisscdf(r1(a[0]), int(i1(a[1])))); });
  R.add("tpdf", 2, 2, PROB, "tpdf(t, df)", "Student's t density.", "tpdf(0, 5) = 0.3796…",
        [](Interp&, Args& a) { return Value(tpdf(r1(a[0]), r1(a[1]))); });
  R.add("tcdf", 2, 3, PROB, "tcdf(t, df) or tcdf(a, b, df)", "Student's t probability P(T ≤ t), or between a and b.",
        "tcdf(2.571, 5) = 0.975", [](Interp&, Args& a) {
          if (a.size() == 2) return Value(tcdf(r1(a[0]), r1(a[1])));
          return Value(tcdf(r1(a[1]), r1(a[2])) - tcdf(r1(a[0]), r1(a[2])));
        });
  R.add("invt", 2, 2, PROB, "invt(p, df)", "Inverse t: the t with P(T ≤ t) = p.", "invt(0.975, 5) = 2.5706…",
        [](Interp&, Args& a) { return Value(invt(r1(a[0]), r1(a[1]))); });
  R.add("chi2pdf", 2, 2, PROB, "chi2pdf(x, k)", "Chi-square density.", "chi2pdf(2, 3) = 0.2075…",
        [](Interp&, Args& a) { return Value(chi2pdf(r1(a[0]), r1(a[1]))); });
  R.add("chi2cdf", 2, 2, PROB, "chi2cdf(x, k)", "Chi-square probability P(X ≤ x).", "chi2cdf(3.841, 1) = 0.95",
        [](Interp&, Args& a) { return Value(chi2cdf(r1(a[0]), r1(a[1]))); });
  R.add("fcdf", 3, 3, PROB, "fcdf(x, d1, d2)", "F distribution probability P(X ≤ x).", "fcdf(1, 5, 5) = 0.5",
        [](Interp&, Args& a) { return Value(fcdf(r1(a[0]), r1(a[1]), r1(a[2]))); });
  R.add("erf", 1, 1, PROB, "erf(x)", "Error function.", "erf(1) = 0.8427…",
        [](Interp&, Args& a) { return mapR(a[0], [](double x) { return std::erf(x); }); });
  R.add("erfc", 1, 1, PROB, "erfc(x)", "Complementary error function 1 − erf(x).", "erfc(1) = 0.1572…",
        [](Interp&, Args& a) { return mapR(a[0], [](double x) { return std::erfc(x); }); });
  R.add("gamma", 1, 1, PROB, "gamma(x)", "Gamma function Γ(x); gamma(n) = (n−1)!.", "gamma(5) = 24",
        [](Interp&, Args& a) { return mapR(a[0], [](double x) { return std::tgamma(x); }); });
  R.add("lgamma", 1, 1, PROB, "lgamma(x)", "Natural log of |Γ(x)|.", "lgamma(100) = 359.13…",
        [](Interp&, Args& a) { return mapR(a[0], [](double x) { return std::lgamma(x); }); });
  R.add("beta", 2, 2, PROB, "beta(a, b)", "Beta function B(a, b).", "beta(2, 3) = 0.08333…", [](Interp&, Args& a) {
    double x = r1(a[0]), y = r1(a[1]);
    return Value(std::exp(std::lgamma(x) + std::lgamma(y) - std::lgamma(x + y)));
  });

  // ----------------------------------------------------------- matrices
  R.add("det", 1, 1, MAT, "det(A)", "Determinant of a square matrix.", "det([[1,2],[3,4]]) = -2",
        [](Interp&, Args& a) { return Value(determinant(asSquare(a[0], "det"))); });
  R.add("transpose", 1, 1, MAT, "transpose(A)", "Swap rows and columns (also A'). Also: trn.", "transpose([[1,2],[3,4]])",
        [](Interp&, Args& a) { return Value::matrix(transpose(matArg(a[0], "transpose"))); });
  R.alias("trn", "transpose");
  R.add("trace", 1, 1, MAT, "trace(A)", "Sum of the diagonal entries.", "trace([[1,2],[3,4]]) = 5",
        [](Interp&, Args& a) { return Value(trace(asSquare(a[0], "trace"))); });
  R.add("rank", 1, 1, MAT, "rank(A)", "Number of independent rows.", "rank([[1,2],[2,4]]) = 1",
        [](Interp&, Args& a) { return Value(double(rank(matArg(a[0], "rank")))); });
  R.add("rref", 1, 1, MAT, "rref(A)", "Reduced row echelon form (Gauss–Jordan elimination).", "rref([[1,2,3],[4,5,6]])",
        [](Interp&, Args& a) { return Value::matrix(rref(matArg(a[0], "rref"))); });
  R.add("eig", 1, 1, MAT, "eig(A)", "Eigenvalues of a square matrix (largest first).", "eig([[2,0],[0,3]]) = [3, 2]",
        [](Interp&, Args& a) { return Value::vector(eigenvalues(asSquare(a[0], "eig"))); });
  R.add("eigvec", 1, 1, MAT, "eigvec(A)", "Eigenvectors as columns, in the same order as eig(A); each scaled so its largest entry is 1.",
        "eigvec([[2,1],[1,2]])", [](Interp&, Args& a) { return Value::matrix(eigenvectors(asSquare(a[0], "eigvec"))); });
  R.add("identity", 1, 1, MAT, "identity(n)", "The n×n identity matrix. Also: eye.", "identity(3)", [](Interp&, Args& a) {
    long long n = i1(a[0]);
    if (n < 1 || n > 500) throw CalcError("size must be between 1 and 500");
    return Value::matrix(Matrix::identity(int(n)));
  });
  R.alias("eye", "identity");
  auto filled = [](Args& a, double v) {
    long long r = i1(a[0]), c = a.size() > 1 ? i1(a[1]) : r;
    if (r < 1 || c < 1 || r > 1000 || c > 1000) throw CalcError("size must be between 1 and 1000");
    return Value::matrix(Matrix(int(r), int(c), v));
  };
  R.add("zeros", 1, 2, MAT, "zeros(rows, cols)", "A matrix full of zeros.", "zeros(2, 3)",
        [filled](Interp&, Args& a) { return filled(a, 0.0); });
  R.add("ones", 1, 2, MAT, "ones(rows, cols)", "A matrix full of ones.", "ones(2, 2)",
        [filled](Interp&, Args& a) { return filled(a, 1.0); });
  R.add("diag", 1, 1, MAT, "diag(list) or diag(A)", "Diagonal matrix from a list, or the diagonal of a matrix.",
        "diag([1, 2, 3])", [](Interp&, Args& a) {
          if (a[0].isMat() && !a[0].vec) {
            const Matrix& m = a[0].mat;
            std::vector<cplx> d;
            for (int k = 0; k < std::min(m.rows, m.cols); ++k) d.push_back(m(k, k));
            return Value::vector(d);
          }
          auto e = a[0].elements();
          Matrix m(int(e.size()), int(e.size()));
          for (size_t k = 0; k < e.size(); ++k) m(int(k), int(k)) = e[k];
          return Value::matrix(m);
        });
  R.add("norm", 1, 1, MAT, "norm(v)", "Length of a vector (Frobenius norm for matrices).", "norm([3, 4]) = 5",
        [](Interp&, Args& a) {
          if (a[0].isNum()) return Value(std::abs(a[0].num));
          return Value(normFrobenius(a[0].mat));
        });
  R.add("dot", 2, 2, MAT, "dot(u, v)", "Dot product of two vectors.", "dot([1,2,3], [4,5,6]) = 32", [](Interp&, Args& a) {
    auto u = a[0].elements(), v = a[1].elements();
    if (u.size() != v.size()) throw CalcError("vectors must have the same length");
    cplx s = 0;
    for (size_t k = 0; k < u.size(); ++k) s += u[k] * v[k];
    return Value(s);
  });
  R.add("cross", 2, 2, MAT, "cross(u, v)", "Cross product of two 3D vectors.", "cross([1,0,0], [0,1,0]) = [0, 0, 1]",
        [](Interp&, Args& a) {
          auto u = a[0].elements(), v = a[1].elements();
          if (u.size() != 3 || v.size() != 3) throw CalcError("cross product needs two vectors of length 3");
          return Value::vector({u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]});
        });
  R.add("size", 1, 1, MAT, "size(A)", "Dimensions [rows, columns].", "size([[1,2,3],[4,5,6]]) = [2, 3]",
        [](Interp&, Args& a) {
          const Matrix& m = matArg(a[0], "size");
          return Value::vectorReal({double(m.rows), double(m.cols)});
        });
  R.add("rows", 1, 1, MAT, "rows(A)", "Number of rows.", "rows([[1,2],[3,4],[5,6]]) = 3",
        [](Interp&, Args& a) { return Value(double(matArg(a[0], "rows").rows)); });
  R.add("cols", 1, 1, MAT, "cols(A)", "Number of columns.", "cols([[1,2,3]]) = 3",
        [](Interp&, Args& a) { return Value(double(matArg(a[0], "cols").cols)); });
  R.add("augment", 2, 2, MAT, "augment(A, B)", "Put B to the right of A.", "augment([[1],[2]], [[3],[4]])",
        [](Interp&, Args& a) {
          const Matrix &A = matArg(a[0], "augment"), &B = matArg(a[1], "augment");
          if (A.rows != B.rows) throw CalcError("augment needs the same number of rows");
          Matrix m(A.rows, A.cols + B.cols);
          for (int r = 0; r < A.rows; ++r) {
            for (int c = 0; c < A.cols; ++c) m(r, c) = A(r, c);
            for (int c = 0; c < B.cols; ++c) m(r, A.cols + c) = B(r, c);
          }
          return Value::matrix(m);
        });
  R.add("row", 2, 2, MAT, "row(A, i)", "Row i of a matrix, as a list.", "row([[1,2],[3,4]], 2) = [3, 4]",
        [](Interp&, Args& a) {
          const Matrix& m = matArg(a[0], "row");
          long long i = i1(a[1]);
          if (i < 1 || i > m.rows) throw CalcError("row index out of range");
          return Value::vector(std::vector<cplx>(m.a.begin() + (i - 1) * m.cols, m.a.begin() + i * m.cols));
        });
  R.add("col", 2, 2, MAT, "col(A, j)", "Column j of a matrix, as a list.", "col([[1,2],[3,4]], 2) = [2, 4]",
        [](Interp&, Args& a) {
          const Matrix& m = matArg(a[0], "col");
          long long j = i1(a[1]);
          if (j < 1 || j > m.cols) throw CalcError("column index out of range");
          std::vector<cplx> v;
          for (int r = 0; r < m.rows; ++r) v.push_back(m(r, int(j - 1)));
          return Value::vector(v);
        });

  // ------------------------------------------------- calculus & solving
  R.lazy("solve", 1, 3, CALC, "solve(equation, x, guess) · solve(A, b)",
         "Find x that makes the equation true (numerically). With a matrix A and list b, solves A·x = b.",
         "solve(x^2 = 2, x) = 1.41421356237", lazySolve);
  R.alias("nsolve", "solve");
  R.lazy("deriv", 2, 3, CALC, "deriv(expr, x, at)", "Derivative of expr with respect to x at a point (numerical).",
         "deriv(x^3, x, 2) = 12", lazyDeriv);
  R.alias("derivative", "deriv");
  R.lazy("integ", 3, 4, CALC, "integ(expr, x, a, b)", "Definite integral ∫ expr dx from a to b. Limits can be inf.",
         "integ(x^2, x, 0, 3) = 9", lazyInteg);
  R.alias("integral", "integ");
  R.alias("integrate", "integ");
  R.lazy("fmin", 3, 4, CALC, "fmin(expr, x, a, b)", "Where expr is smallest on [a, b] (returns x).",
         "fmin((x-2)^2, x, 0, 5) = 2",
         [](Interp& in, const std::vector<NodeP>& a) { return lazyExtremum(in, a, false); });
  R.lazy("fmax", 3, 4, CALC, "fmax(expr, x, a, b)", "Where expr is largest on [a, b] (returns x).",
         "fmax(sin(x), x, 0, 180) = 90 in DEG mode",
         [](Interp& in, const std::vector<NodeP>& a) { return lazyExtremum(in, a, true); });
  R.add("polyroots", 1, -1, CALC, "polyroots(coeffs)", "All roots of a polynomial; coefficients highest power first.",
        "polyroots([1, -3, 2]) = [1, 2]   (x² − 3x + 2)", [](Interp&, Args& a) {
          std::vector<cplx> c;
          for (const auto& v : a) {
            auto e = v.elements();
            c.insert(c.end(), e.begin(), e.end());
          }
          return Value::vector(polyRoots(c));
        });
  R.alias("roots", "polyroots");
  R.alias("proots", "polyroots");
  R.lazy("piecewise", 2, -1, CALC, "piecewise(cond1, value1, cond2, value2, …, otherwise)",
         "Pick the value for the first true condition. Great for graphing.", "piecewise(x < 0, -x, x^2)", lazyPiecewise);

  // --------------------------------------------------------------- units
  R.add("convert", 3, 3, UNIT, "convert(value, \"from\", \"to\")",
        "Convert between units. You can also type: 5 km to mi", "convert(100, \"C\", \"F\") = 212",
        [](Interp&, Args& a) {
          if (!a[1].isStr() || !a[2].isStr()) throw CalcError("units go in quotes: convert(5, \"km\", \"mi\")");
          std::string f = a[1].str, t = a[2].str;
          return mapNumbers(a[0], [&](cplx z) { return cplx(convertUnits(z.real(), f, t)); });
        });

  // ------------------------------------------------------------ programs
  R.add("input", 0, 1, PROG, "input(\"prompt\")", "Ask the user to type a value while a program runs.",
        "n = input(\"How many? \")", [](Interp& in, Args& a) {
          std::string prompt = a.empty() ? "?" : (a[0].isStr() ? a[0].str : formatInline(a[0], in.settings.display));
          std::string answer;
          if (!in.onInput) throw CalcError("input is not available here");
          if (!in.onInput(prompt, answer)) throw CalcError("Interrupted");
          return in.evalString(answer);
        });
  R.add("str", 1, 1, PROG, "str(x)", "Turn a value into text (for print).", "print \"x = \" + str(5)",
        [](Interp& in, Args& a) { return Value::string(formatInline(a[0], in.settings.display)); });

  return R;
}

const Registry& registry() {
  static const Registry r = buildRegistry();
  return r;
}

}  // namespace

const std::vector<Builtin>& builtinList() { return registry().list; }

const Builtin* findBuiltin(const std::string& name) {
  const Registry& r = registry();
  auto it = r.index.find(name);
  if (it != r.index.end()) return &r.list[it->second];
  auto al = r.aliases.find(name);
  if (al != r.aliases.end()) return &r.list[r.index.at(al->second)];
  return nullptr;
}

const std::vector<Constant>& constantList() {
  static const std::vector<Constant> c = {
      {"pi", "Math", "π = 3.14159…, ratio of a circle's circumference to its diameter", 3.14159265358979323846},
      {"e", "Math", "Euler's number e = 2.71828…", 2.71828182845904523536},
      {"i", "Math", "the imaginary unit, i² = −1 (you can also write 3+4i)", 0},
      {"phi", "Math", "golden ratio φ = 1.61803…", 1.61803398874989484820},
      {"inf", "Math", "infinity ∞ (e.g. as an integration limit)", INFINITY},
      {"nan", "Math", "'not a number' (undefined)", NAN},
      {"_c", "Physics", "speed of light in vacuum, 299 792 458 m/s", 299792458.0},
      {"_h", "Physics", "Planck constant, J·s", 6.62607015e-34},
      {"_hbar", "Physics", "reduced Planck constant ħ = h/2π, J·s", 1.054571817e-34},
      {"_e", "Physics", "elementary charge, C", 1.602176634e-19},
      {"_NA", "Chemistry", "Avogadro constant, 1/mol", 6.02214076e23},
      {"_kB", "Physics", "Boltzmann constant, J/K", 1.380649e-23},
      {"_R", "Chemistry", "molar gas constant, J/(mol·K)", 8.314462618},
      {"_F", "Chemistry", "Faraday constant, C/mol", 96485.33212},
      {"_G", "Physics", "gravitational constant, m³/(kg·s²)", 6.67430e-11},
      {"_g", "Physics", "standard gravity, m/s²", 9.80665},
      {"_me", "Physics", "electron mass, kg", 9.1093837015e-31},
      {"_mp", "Physics", "proton mass, kg", 1.67262192369e-27},
      {"_mn", "Physics", "neutron mass, kg", 1.67492749804e-27},
      {"_u", "Chemistry", "atomic mass unit (dalton), kg", 1.66053906660e-27},
      {"_eps0", "Physics", "vacuum permittivity ε₀, F/m", 8.8541878128e-12},
      {"_mu0", "Physics", "vacuum permeability μ₀, N/A²", 1.25663706212e-6},
      {"_sigma", "Physics", "Stefan–Boltzmann constant, W/(m²·K⁴)", 5.670374419e-8},
      {"_a0", "Physics", "Bohr radius, m", 5.29177210903e-11},
      {"_Ry", "Physics", "Rydberg constant, 1/m", 10973731.568160},
      {"_alpha", "Physics", "fine-structure constant", 7.2973525693e-3},
      {"_atm", "Chemistry", "standard atmosphere, Pa", 101325.0},
      {"_Vm", "Chemistry", "molar volume of an ideal gas at 0 °C and 1 atm, m³/mol", 22.41396954e-3},
  };
  return c;
}

const Constant* findConstant(const std::string& name) {
  for (const auto& c : constantList())
    if (c.name == name) return &c;
  return nullptr;
}

}  // namespace cact
