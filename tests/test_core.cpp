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

// Cactulator core tests: run with `make test` or `ctest`.
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <iostream>
#include <string>

#include "core/session.hpp"

using namespace cact;

static int failures = 0, passed = 0;

static std::string run(Session& s, const std::string& input) {
  size_t before = s.history.size();
  s.submit(input);
  std::string out;
  for (size_t i = before; i < s.history.size(); ++i) {
    const auto& h = s.history[i];
    for (const auto& l : h.lines) {
      if (!out.empty()) out += "\n";
      if (h.kind == HistoryItem::Kind::Error) out += "ERROR: ";
      out += l;
    }
  }
  return out;
}

static void expect(Session& s, const std::string& input, const std::string& want) {
  std::string got = run(s, input);
  if (got == want) {
    ++passed;
    return;
  }
  ++failures;
  std::cout << "FAIL: " << input << "\n  want: " << want << "\n  got:  " << got << "\n";
}

static void expectError(Session& s, const std::string& input, const std::string& fragment = "") {
  std::string got = run(s, input);
  if (got.rfind("ERROR: ", 0) == 0 && got.find(fragment) != std::string::npos) {
    ++passed;
    return;
  }
  ++failures;
  std::cout << "FAIL (expected error containing '" << fragment << "'): " << input << "\n  got: " << got << "\n";
}

static void expectNear(Session& s, const std::string& input, double want, double tol = 1e-9) {
  Value v;
  try {
    v = s.in.evalString(input);
  } catch (const CalcError& e) {
    ++failures;
    std::cout << "FAIL: " << input << " threw " << e.what() << "\n";
    return;
  }
  if (v.isNum() && std::abs(v.num.real() - want) <= tol * std::max(1.0, std::abs(want)) && std::abs(v.num.imag()) < 1e-9) {
    ++passed;
    return;
  }
  ++failures;
  std::cout << "FAIL: " << input << " want " << want << " got " << formatInline(v, DisplaySettings{}) << "\n";
}

int main() {
  Session s;

  // Arithmetic and precedence
  expect(s, "2+3*4", "14");
  expect(s, "(2+3)*4", "20");
  expect(s, "2^3^2", "512");
  expect(s, "-2^2", "-4");
  expect(s, "2^-1", "0.5");
  expect(s, "7/2", "3.5");
  expect(s, "17 % 5", "2");
  expect(s, "-7 % 3", "2");
  expect(s, "5!", "120");
  expect(s, "0.1+0.2", "0.3");
  expect(s, "1/3", "0.333333333333");
  expect(s, "2(3+4)", "14");
  expect(s, "1e3 + 2E-2", "1000.02");
  expect(s, "0xFF + 0b101", "260");
  expect(s, "3²", "9");
  expect(s, "√16", "4");
  expect(s, "2π", "6.28318530718");
  expect(s, "1/0", "ERROR: division by zero");
  expect(s, "123456789*1000000", "123456789000000");
  expect(s, "2^0.5", "1.41421356237");
  expect(s, "(-8)^(1/3)", "-2");
  expect(s, "cbrt(-27)", "-3");
  expect(s, "10*2 =", "ERROR: Syntax error: the expression is incomplete");

  // Answers and continuing from ans
  expect(s, "6*7", "42");
  expect(s, "+8", "50");
  expect(s, "ans/2", "25");

  // Trig in degrees (default) and radians
  expect(s, "sin(30)", "0.5");
  expect(s, "cos(60)", "0.5");
  expect(s, "tan(45)", "1");
  expect(s, "sin 90", "1");
  expect(s, "cos(90)", "0");
  expect(s, "asin(0.5)", "30");
  expect(s, "atan2(1, -1)", "135");
  expectError(s, "tan(90)", "undefined");
  run(s, "rad");
  expect(s, "sin(pi)", "0");
  expect(s, "cos(pi)", "-1");
  expect(s, "sin(30°)", "0.5");
  run(s, "deg");

  // Logs and roots
  expect(s, "ln(e)", "1");
  expect(s, "log(1000)", "3");
  expect(s, "log(8, 2)", "3");
  expect(s, "sqrt(-4)", "2i");
  expect(s, "root(32, 5)", "2");

  // Complex numbers
  expect(s, "(3+4i)*(3-4i)", "25");
  expect(s, "abs(3+4i)", "5");
  expect(s, "i^2", "-1");
  expect(s, "(1+2i)/(3-4i)", "-0.2+0.4i");
  expect(s, "conj(2+5i)", "2-5i");

  // Variables and functions
  expect(s, "a = 5", "5");
  expect(s, "a^2 + 1", "26");
  expect(s, "3a", "15");
  expect(s, "f(x) = x^2 - 2x + 1", "Defined f(x)");
  expect(s, "f(3)", "4");
  expect(s, "g(a, b) = a*b + 1", "Defined g(a, b)");
  expect(s, "g(2, 3)", "7");
  expect(s, "7 -> b", "7");
  expect(s, "b", "7");
  expectError(s, "sin = 3", "built-in");
  expectError(s, "sinn(3)", "did you mean 'sin'");
  expectError(s, "zz + 1", "not defined");

  // Lists and statistics
  expect(s, "[1, 2, 3] + 1", "[2, 3, 4]");
  expect(s, "[1, 2, 3] * [4, 5, 6]", "[4, 10, 18]");
  expect(s, "sum([1, 2, 3, 4])", "10");
  expect(s, "mean([2, 4, 9])", "5");
  expect(s, "median([3, 1, 7, 5])", "4");
  expect(s, "psd([2,4,4,4,5,5,7,9])", "2");
  expect(s, "sort([3, 1, 2])", "[1, 2, 3]");
  expect(s, "sum(1/k^2, k, 1, 100)", "1.63498390018");
  expect(s, "seq(k^2, k, 1, 5)", "[1, 4, 9, 16, 25]");
  expect(s, "range(1, 5)", "[1, 2, 3, 4, 5]");
  expect(s, "linreg([1,2,3], [2,4,6])", "[2, 0]");
  expect(s, "polyfit([0,1,2], [1,2,5], 2)", "[1, 0, 1]");
  expect(s, "v = [10, 20, 30]", "[10, 20, 30]");
  expect(s, "v[2]", "20");
  expect(s, "nCr(5, 2)", "10");
  expect(s, "nPr(5, 2)", "20");
  expect(s, "gcd(12, 18)", "6");
  expect(s, "lcm(4, 6)", "12");
  expect(s, "factor(360)", "[2, 2, 2, 3, 3, 5]");
  expect(s, "isprime(97)", "1");

  // Matrices
  expect(s, "A = [[1, 2], [3, 4]]", "⎡ 1  2 ⎤\n⎣ 3  4 ⎦");
  expect(s, "det(A)", "-2");
  expect(s, "inv(A)", "⎡  -2     1 ⎤\n⎣ 1.5  -0.5 ⎦");
  expect(s, "A*inv(A)", "⎡ 1  0 ⎤\n⎣ 0  1 ⎦");
  expect(s, "A'", "⎡ 1  3 ⎤\n⎣ 2  4 ⎦");
  expect(s, "A^2", "⎡  7  10 ⎤\n⎣ 15  22 ⎦");
  expect(s, "[1, 2; 3, 4] == A", "1");
  expect(s, "A*[1, 1]", "[3, 7]");
  expect(s, "solve(A, [5, 11])", "[1, 2]");
  expect(s, "A\\[5, 11]", "[1, 2]");
  expect(s, "trace(A)", "5");
  expect(s, "rank([[1,2],[2,4]])", "1");
  expect(s, "rref([[1,2,3],[4,5,6]])", "⎡ 1  0  -1 ⎤\n⎣ 0  1   2 ⎦");
  expect(s, "eig([[2,1],[1,2]])", "[3, 1]");
  expect(s, "eigvec([[2,1],[1,2]])", "⎡ 1   1 ⎤\n⎣ 1  -1 ⎦");
  expect(s, "eig([[0,-1],[1,0]])", "[i, -i]");
  expect(s, "det([[2,0,1],[1,3,2],[1,1,2]])", "6");
  expect(s, "inv([[4,7],[2,6]])", "⎡  0.6  -0.7 ⎤\n⎣ -0.2   0.4 ⎦");
  expectError(s, "inv([[1,2],[2,4]])", "singular");
  expect(s, "identity(2)", "⎡ 1  0 ⎤\n⎣ 0  1 ⎦");
  expect(s, "cross([1,0,0], [0,1,0])", "[0, 0, 1]");
  expect(s, "dot([1,2,3], [4,5,6])", "32");
  expect(s, "B = zeros(2, 2)", "⎡ 0  0 ⎤\n⎣ 0  0 ⎦");
  expect(s, "B[1, 2] = 5", "5");
  expect(s, "B", "⎡ 0  5 ⎤\n⎣ 0  0 ⎦");

  // Solving and calculus
  expectNear(s, "solve(x^2 = 2, x)", std::sqrt(2.0));
  expectNear(s, "solve(x^2 - 2, x, -1)", -std::sqrt(2.0));
  expectNear(s, "solve(cos(x) = x, x)", 0.99984774153108, 1e-9);  // degrees mode!
  expectNear(s, "solve(exp(x) = 10)", std::log(10.0));
  expect(s, "deriv(x^3, x, 2)", "12");
  expect(s, "integ(x^2, x, 0, 3)", "9");
  expectNear(s, "integ(exp(-x^2), x, -inf, inf)", std::sqrt(3.14159265358979323846), 1e-8);
  expectNear(s, "integ(1/x^2, x, 1, inf)", 1.0, 1e-8);
  expect(s, "polyroots([1, -3, 2])", "[1, 2]");
  expect(s, "polyroots([1, 0, 1])", "[-i, i]");
  expect(s, "polyroots([1, -6, 11, -6])", "[1, 2, 3]");
  expect(s, "fmin((x-2)^2, x, 0, 5)", "2");
  expect(s, "fmax(sin(x), x, 0, 180)", "90");
  expect(s, "solve x^2 = 9", "3");

  // Probability
  expectNear(s, "normcdf(-1.96, 1.96)", 0.950004209703559, 1e-9);
  expectNear(s, "invnorm(0.975)", 1.959963984540054, 1e-9);
  expectNear(s, "binompdf(10, 0.5, 5)", 0.24609375, 1e-12);
  expectNear(s, "tcdf(2.570581835636314, 5)", 0.975, 1e-8);
  expectNear(s, "chi2cdf(3.841458820694124, 1)", 0.95, 1e-8);

  // Units
  expect(s, "5 km to mi", "3.10685596119 mi");
  expect(s, "100 C to F", "212 F");
  expect(s, "convert(1, \"atm\", \"kPa\")", "101.325");
  expect(s, "2.5kg to lb", "5.51155655462 lb");
  expectError(s, "5 km to kg", "can't convert");

  // Display modes
  run(s, "fix 2");
  expect(s, "pi", "3.14");
  run(s, "sci 3");
  expect(s, "12346", "1.235E4");
  run(s, "eng 2");
  expect(s, "12345", "12.35E3");
  run(s, "std");
  run(s, "frac");
  expect(s, "0.75", "3/4");
  expect(s, "pi/4", "π/4");
  run(s, "frac off");
  run(s, "hex");
  expect(s, "255", "0xFF");
  run(s, "dec");

  // Programs
  run(s, "prog sq2(n)\n  return n*n\nend");
  expect(s, "sq2(9)", "81");
  run(s,
      "prog collatz(n)\n  steps = 0\n  while n != 1 do\n    if n % 2 == 0 then\n      n = n / 2\n    else\n      n = 3*n + 1\n"
      "    end\n    steps = steps + 1\n  end\n  return steps\nend");
  expect(s, "collatz(27)", "111");
  run(s, "prog fact2(n)\n  if n <= 1 then return 1 end\n  return n * fact2(n - 1)\nend");
  expect(s, "fact2(10)", "3628800");
  run(s, "prog total(xs)\n  t = 0\n  for v in xs do t = t + v end\n  return t\nend");
  expect(s, "total([1, 2, 3, 4])", "10");
  run(s, "prog countdown(n)\n  for k = n to 1 step -1 do\n    print k\n  end\nend");
  expect(s, "countdown(3)", "3\n2\n1\nDone");
  run(s, "prog firstbig(n)\n  k = 0\n  repeat\n    k = k + 1\n  until k^2 > n\n  return k\nend");
  expect(s, "firstbig(50)", "8");
  run(s, "prog build(n)\n  out = []\n  for k = 1 to n do out[k] = k^2 end\n  return out\nend");
  expect(s, "build(4)", "[1, 4, 9, 16]");
  run(s, "prog loopbreak()\n  k = 0\n  while 1 do\n    k = k + 1\n    if k == 5 then break end\n  end\n  return k\nend");
  expect(s, "loopbreak()", "5");
  expectError(s, "prog bad(n)\n  return n +\nend", "Syntax");
  expect(s, "if 2 > 1 then 10 else 20 end", "10");
  expect(s, "piecewise(-3 < 0, 1, 2)", "1");

  // Graph functions
  expect(s, "y1 = x^2", "Defined y1(x) for graphing");
  expect(s, "y1(3)", "9");

  // RPN
  Session r;
  r.rpnMode = true;
  r.submitRpn("3");
  r.submitRpn("4 +");
  r.submitRpn("2 *");
  if (r.rpn.stack.size() == 1 && r.rpn.stack.back().num.real() == 14) ++passed;
  else {
    ++failures;
    std::cout << "FAIL: RPN 3 4 + 2 * should be 14\n";
  }
  r.submitRpn("9 sqrt swap drop");
  if (r.rpn.stack.size() == 1 && r.rpn.stack.back().num.real() == 3) ++passed;
  else {
    ++failures;
    std::cout << "FAIL: RPN swap/drop\n";
  }
  r.submitRpn("[[1,2],[3,4]] inv det");
  if (r.rpn.stack.size() == 2 && std::abs(r.rpn.stack.back().num.real() + 0.5) < 1e-12) ++passed;
  else {
    ++failures;
    std::cout << "FAIL: RPN matrix inv det\n";
  }

  // Save / load round trip
  {
    Session a;
    a.submit("q = [[1,2],[3,4.5]]");
    a.submit("h(x) = 3x + 1");
    a.submit("prog twice(n)\n  return 2*n\nend");
    a.submit("rad");
    std::string path = "test_state_roundtrip.txt";
    a.save(path);
    Session b;
    b.load(path);
    std::remove(path.c_str());
    expect(b, "q[2, 2]", "4.5");
    expect(b, "h(2)", "7");
    expect(b, "twice(21)", "42");
    expect(b, "sin(pi/2)", "1");
  }

  std::cout << passed << " passed, " << failures << " failed\n";
  return failures ? 1 : 0;
}
