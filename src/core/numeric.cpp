#include "numeric.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <map>
#include <queue>

#include "linalg.hpp"

namespace cact {

static const double kPi = 3.14159265358979323846;

// ------------------------------------------------------------------ roots

double brent(const RealFn& f, double a, double b, double tol) {
  double fa = f(a), fb = f(b);
  if (fa == 0) return a;
  if (fb == 0) return b;
  if ((fa > 0) == (fb > 0)) throw CalcError("no sign change in the interval");
  double c = a, fc = fa, d = b - a, e = d;
  for (int iter = 0; iter < 200; ++iter) {
    if ((fb > 0) == (fc > 0)) {
      c = a;
      fc = fa;
      d = e = b - a;
    }
    if (std::abs(fc) < std::abs(fb)) {
      a = b; b = c; c = a;
      fa = fb; fb = fc; fc = fa;
    }
    double tol1 = 2 * 2.2e-16 * std::abs(b) + 0.5 * tol;
    double xm = 0.5 * (c - b);
    if (std::abs(xm) <= tol1 || fb == 0) return b;
    if (std::abs(e) >= tol1 && std::abs(fa) > std::abs(fb)) {
      double s = fb / fa, p, q;
      if (a == c) {
        p = 2 * xm * s;
        q = 1 - s;
      } else {
        double qq = fa / fc, r = fb / fc;
        p = s * (2 * xm * qq * (qq - r) - (b - a) * (r - 1));
        q = (qq - 1) * (r - 1) * (s - 1);
      }
      if (p > 0) q = -q;
      p = std::abs(p);
      if (2 * p < std::min(3 * xm * q - std::abs(tol1 * q), std::abs(e * q))) {
        e = d;
        d = p / q;
      } else {
        d = xm;
        e = d;
      }
    } else {
      d = xm;
      e = d;
    }
    a = b;
    fa = fb;
    b += std::abs(d) > tol1 ? d : (xm > 0 ? tol1 : -tol1);
    fb = f(b);
    if (std::isnan(fb)) throw CalcError("the function is undefined near the root");
  }
  return b;
}

static bool isGoodRoot(const RealFn& f, double x, double fa, double fb) {
  double fx = f(x);
  if (!std::isfinite(fx)) return false;
  double scale = std::max({1.0, std::abs(fa), std::abs(fb)});
  return std::abs(fx) <= 1e-6 * scale;
}

double findRoot(const RealFn& f, double guess) {
  if (!std::isfinite(guess)) guess = 0;
  double fg = f(guess);
  if (fg == 0) return guess;

  // 1) Newton's method with a numerical derivative.
  double x = guess;
  for (int it = 0; it < 80; ++it) {
    double fx = f(x);
    if (!std::isfinite(fx)) break;
    if (fx == 0) return x;
    double h = 1e-7 * std::max(1.0, std::abs(x));
    double d = (f(x + h) - f(x - h)) / (2 * h);
    if (!std::isfinite(d) || d == 0) break;
    double step = fx / d;
    double xn = x - step;
    if (!std::isfinite(xn)) break;
    if (std::abs(step) <= 1e-13 * std::max(1.0, std::abs(xn))) {
      double fxn = f(xn);
      if (std::isfinite(fxn) && std::abs(fxn) <= 1e-9 * std::max(1.0, std::abs(fg))) return xn;
      break;
    }
    x = xn;
    if (std::abs(x - guess) > 1e12) break;
  }

  // 2) Scan outwards from the guess for a sign change, then use Brent.
  double base = std::max(1.0, std::abs(guess));
  for (double w : {0.5, 2.0, 10.0, 100.0, 1e3, 1e4, 1e6, 1e9}) {
    const int N = 200;
    double lo = guess - w * base, hi = guess + w * base;
    std::vector<double> xs(N + 1), fs(N + 1);
    for (int i = 0; i <= N; ++i) {
      xs[i] = lo + (hi - lo) * i / N;
      fs[i] = f(xs[i]);
    }
    // Intervals ordered by distance from the guess.
    std::vector<int> order(N);
    for (int i = 0; i < N; ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) {
      return std::abs(0.5 * (xs[a] + xs[a + 1]) - guess) < std::abs(0.5 * (xs[b] + xs[b + 1]) - guess);
    });
    for (int i : order) {
      double fa = fs[i], fb = fs[i + 1];
      if (fa == 0) return xs[i];
      if (!std::isfinite(fa) || !std::isfinite(fb)) continue;
      if ((fa > 0) == (fb > 0)) continue;
      try {
        double r = brent(f, xs[i], xs[i + 1]);
        if (isGoodRoot(f, r, fa, fb)) return r;
      } catch (const CalcError&) {
      }
    }
  }
  // 3) Touching roots (like x^2): minimize |f| near the guess.
  {
    RealFn g = [&](double t) { double v = f(t); return std::isfinite(v) ? std::abs(v) : 1e300; };
    double xm = minimize(g, guess - 10 * base, guess + 10 * base);
    if (std::abs(f(xm)) < 1e-10 * std::max(1.0, std::abs(fg))) return xm;
  }
  throw CalcError("no solution found (try a different starting guess)");
}

// ---------------------------------------------------------------- calculus

double derivative(const RealFn& f, double x) {
  // Ridders' method: Richardson extrapolation of central differences.
  const int NTAB = 10;
  const double CON = 1.4, CON2 = CON * CON, SAFE = 2.0;
  double h = 0.05 * std::max(1.0, std::abs(x));
  double a[NTAB][NTAB];
  a[0][0] = (f(x + h) - f(x - h)) / (2 * h);
  double err = 1e300, ans = a[0][0];
  for (int i = 1; i < NTAB; ++i) {
    h /= CON;
    a[0][i] = (f(x + h) - f(x - h)) / (2 * h);
    double fac = CON2;
    for (int j = 1; j <= i; ++j) {
      a[j][i] = (a[j - 1][i] * fac - a[j - 1][i - 1]) / (fac - 1);
      fac *= CON2;
      double errt = std::max(std::abs(a[j][i] - a[j - 1][i]), std::abs(a[j][i] - a[j - 1][i - 1]));
      if (errt <= err) {
        err = errt;
        ans = a[j][i];
      }
    }
    if (std::abs(a[i][i] - a[i - 1][i - 1]) >= SAFE * err) break;
  }
  if (!std::isfinite(ans)) throw CalcError("the derivative does not exist at this point");
  double r = std::round(ans);
  if (std::abs(ans - r) < 1e-9 * std::max(1.0, std::abs(r))) ans = r;
  return ans;
}

double secondDerivative(const RealFn& f, double x) {
  RealFn d1 = [&](double t) { return derivative(f, t); };
  return derivative(d1, x);
}

namespace {

const double XGK[8] = {0.991455371120812639206854697526329, 0.949107912342758524526189684047851,
                       0.864864423359769072789712788640926, 0.741531185599394439863864773280788,
                       0.586087235467691130294144845693013, 0.405845151377397166906606412076961,
                       0.207784955007898467600689403773245, 0.0};
const double WGK[8] = {0.022935322010529224963732008058970, 0.063092092629978553290700663189204,
                       0.104790010322250183839876322541518, 0.140653259715525918745189590510238,
                       0.169004726639267902826583426598550, 0.190350578064785409913256402421014,
                       0.204432940075298892414161999234649, 0.209482141084727828012999174891714};
const double WG[4] = {0.129484966168869693270611432679082, 0.279705391489276667901467771423780,
                      0.381830050505118944950369775488975, 0.417959183673469387755102040816327};

struct Piece {
  double a, b, val, err;
  bool operator<(const Piece& o) const { return err < o.err; }
};

Piece gk15(const RealFn& f, double a, double b) {
  double c = 0.5 * (a + b), h = 0.5 * (b - a);
  double fc = f(c);
  double resk = fc * WGK[7], resg = fc * WG[3];
  for (int j = 0; j < 7; ++j) {
    double dx = h * XGK[j];
    double f1 = f(c - dx), f2 = f(c + dx);
    resk += WGK[j] * (f1 + f2);
    if (j % 2 == 1) resg += WG[j / 2] * (f1 + f2);
  }
  return {a, b, resk * h, std::abs((resk - resg) * h)};
}

double integrateFinite(const RealFn& f, double a, double b) {
  std::priority_queue<Piece> pq;
  Piece first = gk15(f, a, b);
  double total = first.val, err = first.err;
  pq.push(first);
  int count = 0;
  while (count < 4000 && err > std::max(1e-13, 1e-11 * std::abs(total))) {
    Piece p = pq.top();
    pq.pop();
    double m = 0.5 * (p.a + p.b);
    if (m == p.a || m == p.b) break;
    Piece l = gk15(f, p.a, m), r = gk15(f, m, p.b);
    total += l.val + r.val - p.val;
    err += l.err + r.err - p.err;
    pq.push(l);
    pq.push(r);
    ++count;
  }
  // Recompute the sum to avoid drift.
  double sum = 0, esum = 0;
  while (!pq.empty()) {
    sum += pq.top().val;
    esum += pq.top().err;
    pq.pop();
  }
  if (!std::isfinite(sum)) throw CalcError("the integral diverges, or the function is undefined somewhere in the interval");
  if (esum > 1e-3 * std::max(1.0, std::abs(sum)))
    throw CalcError("the integral does not seem to converge");
  return sum;
}

}  // namespace

double integrate(const RealFn& f, double a, double b) {
  if (std::isnan(a) || std::isnan(b)) throw CalcError("integration limits must be numbers");
  if (a == b) return 0;
  if (a > b) return -integrate(f, b, a);
  if (std::isinf(a) && std::isinf(b)) return integrate(f, -INFINITY, 0) + integrate(f, 0, INFINITY);
  if (std::isinf(b)) {
    RealFn g = [&](double t) {
      double s = 1 - t;
      return f(a + t / s) / (s * s);
    };
    return integrateFinite(g, 0, 1);
  }
  if (std::isinf(a)) {
    RealFn g = [&](double t) {
      double s = 1 - t;
      return f(b - t / s) / (s * s);
    };
    return integrateFinite(g, 0, 1);
  }
  double r = integrateFinite(f, a, b);
  double rr = std::round(r);
  if (std::abs(r - rr) < 1e-11 * std::max(1.0, std::abs(rr))) r = rr;
  return r;
}

double minimize(const RealFn& f, double a, double b) {
  const int N = 400;
  int best = 0;
  double bestv = INFINITY;
  for (int i = 0; i <= N; ++i) {
    double x = a + (b - a) * i / N;
    double v = f(x);
    if (std::isfinite(v) && v < bestv) {
      bestv = v;
      best = i;
    }
  }
  if (!std::isfinite(bestv)) throw CalcError("the function is undefined on this interval");
  double lo = a + (b - a) * std::max(0, best - 1) / N;
  double hi = a + (b - a) * std::min(N, best + 1) / N;
  const double gr = 0.6180339887498949;
  double c = hi - gr * (hi - lo), d = lo + gr * (hi - lo);
  double fc = f(c), fd = f(d);
  for (int it = 0; it < 200 && std::abs(hi - lo) > 1e-15 * std::max(1.0, std::abs(lo)); ++it) {
    if (!(fd < fc)) {
      hi = d;
      d = c;
      fd = fc;
      c = hi - gr * (hi - lo);
      fc = f(c);
    } else {
      lo = c;
      c = d;
      fc = fd;
      d = lo + gr * (hi - lo);
      fd = f(d);
    }
  }
  double x = 0.5 * (lo + hi);
  return x;
}

double snapMinimum(const RealFn& g, double x) {
  double gx = g(x);
  if (!std::isfinite(gx)) return x;
  for (int d = 0; d <= 10; ++d) {
    double p = std::pow(10.0, d);
    double rx = std::round(x * p) / p;
    if (std::abs(rx - x) > 1e-5 * std::max(1.0, std::abs(x))) continue;
    double gr = g(rx);
    if (std::isfinite(gr) && gr <= gx + 1e-13 * std::max(1.0, std::abs(gx))) return rx;
  }
  return x;
}

// -------------------------------------------------------------- polynomials

cplx polyEval(const std::vector<cplx>& c, cplx x) {
  cplx r = 0;
  for (const auto& a : c) r = r * x + a;
  return r;
}

std::vector<cplx> polyRoots(std::vector<cplx> c) {
  while (!c.empty() && c.front() == cplx(0)) c.erase(c.begin());
  if (c.empty()) throw CalcError("all coefficients are zero");
  std::vector<cplx> roots;
  while (c.size() > 1 && c.back() == cplx(0)) {
    roots.push_back(0.0);
    c.pop_back();
  }
  int n = int(c.size()) - 1;
  if (n >= 1) {
    std::vector<cplx> m(c.size());
    for (size_t i = 0; i < c.size(); ++i) m[i] = c[i] / c[0];
    double R = 0;
    for (size_t i = 1; i < m.size(); ++i) R = std::max(R, std::abs(m[i]));
    R = 1 + R;
    std::vector<cplx> z(n);
    for (int k = 0; k < n; ++k) z[k] = std::polar(R * 0.9, 2 * kPi * k / n + 0.4);
    for (int it = 0; it < 1000; ++it) {
      double maxStep = 0;
      for (int k = 0; k < n; ++k) {
        cplx denom = 1.0;
        for (int j = 0; j < n; ++j)
          if (j != k) denom *= (z[k] - z[j]);
        if (denom == cplx(0)) denom = 1e-12;
        cplx w = polyEval(m, z[k]) / denom;
        z[k] -= w;
        maxStep = std::max(maxStep, std::abs(w) / std::max(1.0, std::abs(z[k])));
      }
      if (maxStep < 1e-15) break;
    }
    // Polish with Newton on the original polynomial.
    std::vector<cplx> dc;
    for (int i = 0; i < n; ++i) dc.push_back(c[i] * double(n - i));
    bool realCoeffs = true;
    for (const auto& a : c)
      if (a.imag() != 0.0) realCoeffs = false;
    for (auto& r : z) {
      for (int it = 0; it < 5; ++it) {
        cplx d = polyEval(dc, r);
        if (d == cplx(0)) break;
        cplx step = polyEval(c, r) / d;
        if (!std::isfinite(step.real()) || !std::isfinite(step.imag())) break;
        r -= step;
      }
      if (realCoeffs && std::abs(r.imag()) < 1e-9 * std::max(1.0, std::abs(r))) r = r.real();
      double rr = std::round(r.real()), ri = std::round(r.imag());
      if (std::abs(r.real() - rr) < 1e-10 * std::max(1.0, std::abs(rr))) r = cplx(rr, r.imag());
      if (std::abs(r.imag() - ri) < 1e-10 * std::max(1.0, std::abs(ri))) r = cplx(r.real(), ri);
      roots.push_back(chopTiny(r));
    }
  }
  std::sort(roots.begin(), roots.end(), [](const cplx& a, const cplx& b) {
    if (a.real() != b.real()) return a.real() < b.real();
    return a.imag() < b.imag();
  });
  return roots;
}

// --------------------------------------------------------------- statistics

double mean(const std::vector<double>& xs) {
  if (xs.empty()) throw CalcError("the list is empty");
  double s = 0;
  for (double x : xs) s += x;
  return s / xs.size();
}

double median(std::vector<double> xs) {
  if (xs.empty()) throw CalcError("the list is empty");
  std::sort(xs.begin(), xs.end());
  size_t n = xs.size();
  return n % 2 ? xs[n / 2] : 0.5 * (xs[n / 2 - 1] + xs[n / 2]);
}

double variance(const std::vector<double>& xs, bool sample) {
  if (xs.empty()) throw CalcError("the list is empty");
  if (sample && xs.size() < 2) throw CalcError("need at least 2 values for a sample standard deviation");
  double m = mean(xs), s = 0;
  for (double x : xs) s += (x - m) * (x - m);
  return s / (sample ? xs.size() - 1 : xs.size());
}

double quantile(std::vector<double> xs, double q) {
  if (xs.empty()) throw CalcError("the list is empty");
  if (q < 0 || q > 1) throw CalcError("percentile must be between 0 and 100");
  std::sort(xs.begin(), xs.end());
  double pos = q * (xs.size() - 1);
  size_t i = size_t(std::floor(pos));
  if (i + 1 >= xs.size()) return xs.back();
  return xs[i] + (pos - i) * (xs[i + 1] - xs[i]);
}

std::vector<double> modes(const std::vector<double>& xs) {
  std::map<double, int> cnt;
  int best = 0;
  for (double x : xs) best = std::max(best, ++cnt[x]);
  std::vector<double> out;
  if (best <= 1 && xs.size() > 1) return out;
  for (const auto& [v, c] : cnt)
    if (c == best) out.push_back(v);
  return out;
}

double correlation(const std::vector<double>& xs, const std::vector<double>& ys) {
  if (xs.size() != ys.size()) throw CalcError("the x and y lists must have the same length");
  if (xs.size() < 2) throw CalcError("need at least 2 points");
  double mx = mean(xs), my = mean(ys), sxy = 0, sxx = 0, syy = 0;
  for (size_t i = 0; i < xs.size(); ++i) {
    sxy += (xs[i] - mx) * (ys[i] - my);
    sxx += (xs[i] - mx) * (xs[i] - mx);
    syy += (ys[i] - my) * (ys[i] - my);
  }
  if (sxx == 0 || syy == 0) return 0;
  return sxy / std::sqrt(sxx * syy);
}

std::vector<double> polyfit(const std::vector<double>& xs, const std::vector<double>& ys, int degree) {
  if (xs.size() != ys.size()) throw CalcError("the x and y lists must have the same length");
  if (degree < 0 || degree > 10) throw CalcError("polynomial degree must be between 0 and 10");
  if ((int)xs.size() < degree + 1)
    throw CalcError("need at least " + std::to_string(degree + 1) + " points for degree " + std::to_string(degree));
  int n = int(xs.size()), m = degree + 1;
  Matrix A(n, m), b(n, 1);
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < m; ++j) A(i, j) = std::pow(xs[i], degree - j);
    b(i, 0) = ys[i];
  }
  Matrix x = solveLinear(A, b);
  std::vector<double> out;
  for (int j = 0; j < m; ++j) out.push_back(x(j, 0).real());
  return out;
}

static std::string num6(double v) {
  char buf[64];
  std::snprintf(buf, sizeof buf, "%.6g", v);
  return buf;
}
static std::string numFull(double v) {
  char buf[64];
  std::snprintf(buf, sizeof buf, "%.15g", v);
  return std::string("(") + buf + ")";
}
static std::string signed6(double v) { return (v < 0 ? " - " : " + ") + num6(std::abs(v)); }

Fit fitModel(const std::string& model, const std::vector<double>& xs, const std::vector<double>& ys) {
  if (xs.size() != ys.size()) throw CalcError("the x and y lists must have the same length");
  if (xs.size() < 2) throw CalcError("need at least 2 data points");
  Fit fit;
  fit.model = model;
  std::function<double(double)> predict;
  if (model == "linear") {
    auto c = polyfit(xs, ys, 1);
    fit.coef = c;
    fit.r = correlation(xs, ys);
    fit.equation = "y = " + num6(c[0]) + "x" + signed6(c[1]);
    fit.expr = numFull(c[0]) + "*x+" + numFull(c[1]);
    predict = [c](double x) { return c[0] * x + c[1]; };
  } else if (model == "quadratic" || model == "cubic") {
    int deg = model == "quadratic" ? 2 : 3;
    auto c = polyfit(xs, ys, deg);
    fit.coef = c;
    fit.equation = "y = " + num6(c[0]) + "x" + (deg == 3 ? "³" : "²");
    fit.expr = numFull(c[0]) + "*x^" + std::to_string(deg);
    for (int i = 1; i <= deg; ++i) {
      int p = deg - i;
      fit.equation += signed6(c[i]) + (p == 2 ? "x²" : p == 1 ? "x" : "");
      fit.expr += "+" + numFull(c[i]) + (p >= 1 ? "*x^" + std::to_string(p) : "");
    }
    predict = [c](double x) {
      double r = 0;
      for (double a : c) r = r * x + a;
      return r;
    };
  } else if (model == "exponential") {
    std::vector<double> ly;
    for (double y : ys) {
      if (y <= 0) throw CalcError("exponential fit needs all y values > 0");
      ly.push_back(std::log(y));
    }
    auto c = polyfit(xs, ly, 1);
    double a = std::exp(c[1]), b = c[0];
    fit.coef = {a, b};
    fit.equation = "y = " + num6(a) + "·e^(" + num6(b) + "x)";
    fit.expr = numFull(a) + "*exp(" + numFull(b) + "*x)";
    predict = [a, b](double x) { return a * std::exp(b * x); };
  } else if (model == "power") {
    std::vector<double> lx, ly;
    for (size_t i = 0; i < xs.size(); ++i) {
      if (xs[i] <= 0 || ys[i] <= 0) throw CalcError("power fit needs all x and y values > 0");
      lx.push_back(std::log(xs[i]));
      ly.push_back(std::log(ys[i]));
    }
    auto c = polyfit(lx, ly, 1);
    double a = std::exp(c[1]), b = c[0];
    fit.coef = {a, b};
    fit.equation = "y = " + num6(a) + "·x^" + num6(b);
    fit.expr = numFull(a) + "*x^" + numFull(b);
    predict = [a, b](double x) { return a * std::pow(x, b); };
  } else if (model == "logarithmic") {
    std::vector<double> lx;
    for (double x : xs) {
      if (x <= 0) throw CalcError("logarithmic fit needs all x values > 0");
      lx.push_back(std::log(x));
    }
    auto c = polyfit(lx, ys, 1);
    double a = c[1], b = c[0];
    fit.coef = {a, b};
    fit.equation = "y = " + num6(a) + signed6(b) + "·ln(x)";
    fit.expr = numFull(a) + "+" + numFull(b) + "*ln(x)";
    predict = [a, b](double x) { return a + b * std::log(x); };
  } else {
    throw CalcError("unknown regression model '" + model + "'");
  }
  double my = mean(ys), ssr = 0, sst = 0;
  for (size_t i = 0; i < xs.size(); ++i) {
    double e = ys[i] - predict(xs[i]);
    ssr += e * e;
    sst += (ys[i] - my) * (ys[i] - my);
  }
  fit.r2 = sst > 0 ? 1 - ssr / sst : 1;
  return fit;
}

// ------------------------------------------------------------ distributions

double normpdf(double x, double mu, double sigma) {
  if (sigma <= 0) throw CalcError("σ must be positive");
  double z = (x - mu) / sigma;
  return std::exp(-0.5 * z * z) / (sigma * std::sqrt(2 * kPi));
}

double normcdf(double x, double mu, double sigma) {
  if (sigma <= 0) throw CalcError("σ must be positive");
  return 0.5 * std::erfc(-(x - mu) / (sigma * std::sqrt(2.0)));
}

double invnorm(double p, double mu, double sigma) {
  if (p <= 0 || p >= 1) throw CalcError("probability must be between 0 and 1 (exclusive)");
  if (sigma <= 0) throw CalcError("σ must be positive");
  // Acklam's rational approximation followed by one Halley step.
  static const double a[] = {-3.969683028665376e+01, 2.209460984245205e+02, -2.759285104469687e+02,
                             1.383577518672690e+02, -3.066479806614716e+01, 2.506628277459239e+00};
  static const double b[] = {-5.447609879822406e+01, 1.615858368580409e+02, -1.556989798598866e+02,
                             6.680131188771972e+01, -1.328068155288572e+01};
  static const double c[] = {-7.784894002430293e-03, -3.223964580411365e-01, -2.400758277161838e+00,
                             -2.549732539343734e+00, 4.374664141464968e+00, 2.938163982698783e+00};
  static const double d[] = {7.784695709041462e-03, 3.224671290700398e-01, 2.445134137142996e+00,
                             3.754408661907416e+00};
  double q, x;
  if (p < 0.02425) {
    q = std::sqrt(-2 * std::log(p));
    x = (((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
        ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1);
  } else if (p > 1 - 0.02425) {
    q = std::sqrt(-2 * std::log(1 - p));
    x = -(((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
        ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1);
  } else {
    q = p - 0.5;
    double r = q * q;
    x = (((((a[0] * r + a[1]) * r + a[2]) * r + a[3]) * r + a[4]) * r + a[5]) * q /
        (((((b[0] * r + b[1]) * r + b[2]) * r + b[3]) * r + b[4]) * r + 1);
  }
  double e = 0.5 * std::erfc(-x / std::sqrt(2.0)) - p;
  double u = e * std::sqrt(2 * kPi) * std::exp(x * x / 2);
  x = x - u / (1 + x * u / 2);
  return mu + sigma * x;
}

static double logChoose(int n, int k) { return std::lgamma(n + 1.0) - std::lgamma(k + 1.0) - std::lgamma(n - k + 1.0); }

double binompdf(int n, double p, int k) {
  if (n < 0) throw CalcError("n must be ≥ 0");
  if (p < 0 || p > 1) throw CalcError("p must be between 0 and 1");
  if (k < 0 || k > n) return 0;
  if (p == 0) return k == 0 ? 1 : 0;
  if (p == 1) return k == n ? 1 : 0;
  return std::exp(logChoose(n, k) + k * std::log(p) + (n - k) * std::log1p(-p));
}

double binomcdf(int n, double p, int k) {
  double s = 0;
  for (int i = 0; i <= std::min(k, n); ++i) s += binompdf(n, p, i);
  return std::min(1.0, s);
}

double poisspdf(double lambda, int k) {
  if (lambda <= 0) throw CalcError("λ must be positive");
  if (k < 0) return 0;
  return std::exp(k * std::log(lambda) - lambda - std::lgamma(k + 1.0));
}

double poisscdf(double lambda, int k) {
  double s = 0;
  for (int i = 0; i <= k; ++i) s += poisspdf(lambda, i);
  return std::min(1.0, s);
}

static double betacf(double a, double b, double x) {
  const int MAXIT = 300;
  const double EPS = 1e-15, FPMIN = 1e-300;
  double qab = a + b, qap = a + 1, qam = a - 1, c = 1, d = 1 - qab * x / qap;
  if (std::abs(d) < FPMIN) d = FPMIN;
  d = 1 / d;
  double h = d;
  for (int m = 1; m <= MAXIT; ++m) {
    int m2 = 2 * m;
    double aa = m * (b - m) * x / ((qam + m2) * (a + m2));
    d = 1 + aa * d;
    if (std::abs(d) < FPMIN) d = FPMIN;
    c = 1 + aa / c;
    if (std::abs(c) < FPMIN) c = FPMIN;
    d = 1 / d;
    h *= d * c;
    aa = -(a + m) * (qab + m) * x / ((a + m2) * (qap + m2));
    d = 1 + aa * d;
    if (std::abs(d) < FPMIN) d = FPMIN;
    c = 1 + aa / c;
    if (std::abs(c) < FPMIN) c = FPMIN;
    d = 1 / d;
    double del = d * c;
    h *= del;
    if (std::abs(del - 1) < EPS) break;
  }
  return h;
}

double regIncBeta(double a, double b, double x) {
  if (x <= 0) return 0;
  if (x >= 1) return 1;
  double bt = std::exp(std::lgamma(a + b) - std::lgamma(a) - std::lgamma(b) + a * std::log(x) + b * std::log1p(-x));
  if (x < (a + 1) / (a + b + 2)) return bt * betacf(a, b, x) / a;
  return 1 - bt * betacf(b, a, 1 - x) / b;
}

double regIncGammaP(double a, double x) {
  if (x <= 0) return 0;
  if (x < a + 1) {
    double ap = a, sum = 1 / a, del = sum;
    for (int n = 0; n < 1000; ++n) {
      ap += 1;
      del *= x / ap;
      sum += del;
      if (std::abs(del) < std::abs(sum) * 1e-16) break;
    }
    return sum * std::exp(-x + a * std::log(x) - std::lgamma(a));
  }
  double b = x + 1 - a, c = 1 / 1e-300, d = 1 / b, h = d;
  for (int i = 1; i < 1000; ++i) {
    double an = -i * (i - a);
    b += 2;
    d = an * d + b;
    if (std::abs(d) < 1e-300) d = 1e-300;
    c = b + an / c;
    if (std::abs(c) < 1e-300) c = 1e-300;
    d = 1 / d;
    double del = d * c;
    h *= del;
    if (std::abs(del - 1) < 1e-16) break;
  }
  return 1 - std::exp(-x + a * std::log(x) - std::lgamma(a)) * h;
}

double tpdf(double t, double df) {
  if (df <= 0) throw CalcError("degrees of freedom must be positive");
  return std::exp(std::lgamma((df + 1) / 2) - std::lgamma(df / 2)) / std::sqrt(df * kPi) *
         std::pow(1 + t * t / df, -(df + 1) / 2);
}

double tcdf(double t, double df) {
  if (df <= 0) throw CalcError("degrees of freedom must be positive");
  double x = df / (df + t * t);
  double tail = 0.5 * regIncBeta(df / 2, 0.5, x);
  return t >= 0 ? 1 - tail : tail;
}

double invt(double p, double df) {
  if (p <= 0 || p >= 1) throw CalcError("probability must be between 0 and 1 (exclusive)");
  RealFn f = [&](double t) { return tcdf(t, df) - p; };
  return brent(f, -1e4, 1e4, 1e-14);
}

double chi2pdf(double x, double k) {
  if (k <= 0) throw CalcError("degrees of freedom must be positive");
  if (x < 0) return 0;
  return std::exp((k / 2 - 1) * std::log(x) - x / 2 - (k / 2) * std::log(2.0) - std::lgamma(k / 2));
}

double chi2cdf(double x, double k) {
  if (k <= 0) throw CalcError("degrees of freedom must be positive");
  return regIncGammaP(k / 2, x / 2);
}

double fcdf(double x, double d1, double d2) {
  if (d1 <= 0 || d2 <= 0) throw CalcError("degrees of freedom must be positive");
  if (x <= 0) return 0;
  return regIncBeta(d1 / 2, d2 / 2, d1 * x / (d1 * x + d2));
}

}  // namespace cact
