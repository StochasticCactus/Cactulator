// Cactulator - numerical methods: roots, calculus, optimization, polynomials,
// statistics and probability distributions.
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "value.hpp"

namespace cact {

using RealFn = std::function<double(double)>;

// Find x with f(x) = 0 near `guess`. Throws CalcError if nothing is found.
double findRoot(const RealFn& f, double guess);
// Root inside [a, b] where f changes sign (Brent's method).
double brent(const RealFn& f, double a, double b, double tol = 1e-15);
double derivative(const RealFn& f, double x);
double secondDerivative(const RealFn& f, double x);
double integrate(const RealFn& f, double a, double b);
// Minimize f on [a, b]; returns x.
double minimize(const RealFn& f, double a, double b);
// Round x to the fewest decimals that keep g(x) (to be minimized) as small.
double snapMinimum(const RealFn& g, double x);
// All complex roots of a polynomial, coefficients highest power first.
std::vector<cplx> polyRoots(std::vector<cplx> coeffs);
cplx polyEval(const std::vector<cplx>& coeffs, cplx x);

// --- statistics
double mean(const std::vector<double>& xs);
double median(std::vector<double> xs);
double variance(const std::vector<double>& xs, bool sample);
double quantile(std::vector<double> xs, double q);  // q in [0, 1]
std::vector<double> modes(const std::vector<double>& xs);

struct Fit {
  std::string model;  // "linear", "quadratic", ...
  std::vector<double> coef;
  double r2 = 0;      // coefficient of determination
  double r = 0;       // correlation (linear fits)
  std::string equation;  // human readable
  std::string expr;      // expression in x usable for graphing
};
Fit fitModel(const std::string& model, const std::vector<double>& xs, const std::vector<double>& ys);
std::vector<double> polyfit(const std::vector<double>& xs, const std::vector<double>& ys, int degree);
double correlation(const std::vector<double>& xs, const std::vector<double>& ys);

// --- distributions
double normpdf(double x, double mu, double sigma);
double normcdf(double x, double mu, double sigma);
double invnorm(double p, double mu, double sigma);
double binompdf(int n, double p, int k);
double binomcdf(int n, double p, int k);
double poisspdf(double lambda, int k);
double poisscdf(double lambda, int k);
double tpdf(double t, double df);
double tcdf(double t, double df);
double invt(double p, double df);
double chi2pdf(double x, double k);
double chi2cdf(double x, double k);
double fcdf(double x, double d1, double d2);
double regIncBeta(double a, double b, double x);
double regIncGammaP(double a, double x);

}  // namespace cact
