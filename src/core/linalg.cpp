#include "linalg.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace cact {

static std::string dims(const Matrix& m) { return std::to_string(m.rows) + "×" + std::to_string(m.cols); }

static void requireSquare(const Matrix& m, const char* what) {
  if (!m.square())
    throw CalcError(std::string(what) + " needs a square matrix, but this one is " + dims(m));
  if (m.rows == 0) throw CalcError(std::string(what) + " needs a non-empty matrix");
}

static double maxAbs(const Matrix& m) {
  double mx = 0;
  for (const auto& z : m.a) mx = std::max(mx, std::abs(z));
  return mx;
}

static bool allIntegers(const Matrix& m) {
  for (const auto& z : m.a)
    if (z.imag() != 0.0 || z.real() != std::floor(z.real()) || std::abs(z.real()) > 1e9) return false;
  return true;
}

Matrix transpose(const Matrix& m) {
  Matrix t(m.cols, m.rows);
  for (int r = 0; r < m.rows; ++r)
    for (int c = 0; c < m.cols; ++c) t(c, r) = m(r, c);
  return t;
}

Matrix multiply(const Matrix& a, const Matrix& b) {
  if (a.cols != b.rows)
    throw CalcError("can't multiply a " + dims(a) + " matrix by a " + dims(b) +
                    " matrix (columns of the first must equal rows of the second)");
  Matrix c(a.rows, b.cols);
  for (int i = 0; i < a.rows; ++i)
    for (int k = 0; k < a.cols; ++k) {
      cplx aik = a(i, k);
      if (aik == cplx(0)) continue;
      for (int j = 0; j < b.cols; ++j) c(i, j) += aik * b(k, j);
    }
  return c;
}

cplx determinant(const Matrix& m0) {
  requireSquare(m0, "det");
  Matrix m = m0;
  int n = m.rows;
  cplx det = 1.0;
  for (int k = 0; k < n; ++k) {
    int piv = k;
    for (int r = k + 1; r < n; ++r)
      if (std::abs(m(r, k)) > std::abs(m(piv, k))) piv = r;
    if (m(piv, k) == cplx(0)) return 0.0;
    if (piv != k) {
      for (int c = 0; c < n; ++c) std::swap(m(k, c), m(piv, c));
      det = -det;
    }
    det *= m(k, k);
    for (int r = k + 1; r < n; ++r) {
      cplx f = m(r, k) / m(k, k);
      if (f == cplx(0)) continue;
      for (int c = k; c < n; ++c) m(r, c) -= f * m(k, c);
    }
  }
  if (allIntegers(m0)) det = std::round(det.real());
  return det;
}

Matrix inverse(const Matrix& m0) {
  requireSquare(m0, "inv");
  int n = m0.rows;
  Matrix m = m0, inv = Matrix::identity(n);
  double scale = maxAbs(m0);
  double tol = 1e-13 * std::max(scale, 1e-300) * n;
  for (int k = 0; k < n; ++k) {
    int piv = k;
    for (int r = k + 1; r < n; ++r)
      if (std::abs(m(r, k)) > std::abs(m(piv, k))) piv = r;
    if (std::abs(m(piv, k)) <= tol)
      throw CalcError("this matrix is singular (its determinant is 0), so it has no inverse");
    if (piv != k)
      for (int c = 0; c < n; ++c) {
        std::swap(m(k, c), m(piv, c));
        std::swap(inv(k, c), inv(piv, c));
      }
    cplx p = m(k, k);
    for (int c = 0; c < n; ++c) {
      m(k, c) /= p;
      inv(k, c) /= p;
    }
    for (int r = 0; r < n; ++r) {
      if (r == k) continue;
      cplx f = m(r, k);
      if (f == cplx(0)) continue;
      for (int c = 0; c < n; ++c) {
        m(r, c) -= f * m(k, c);
        inv(r, c) -= f * inv(k, c);
      }
    }
  }
  for (auto& z : inv.a) z = chopTiny(z);
  return inv;
}

Matrix matrixPower(const Matrix& m, long long n) {
  requireSquare(m, "matrix power");
  if (n < 0) return matrixPower(inverse(m), -n);
  Matrix result = Matrix::identity(m.rows), base = m;
  while (n > 0) {
    if (n & 1) result = multiply(result, base);
    base = multiply(base, base);
    n >>= 1;
  }
  return result;
}

static Matrix conjTranspose(const Matrix& m) {
  Matrix t = transpose(m);
  for (auto& z : t.a) z = std::conj(z);
  return t;
}

Matrix solveLinear(const Matrix& a, const Matrix& b) {
  if (a.rows != b.rows)
    throw CalcError("can't solve: the matrix has " + std::to_string(a.rows) + " rows but the right side has " +
                    std::to_string(b.rows));
  if (!a.square()) {
    // Least squares via the normal equations.
    Matrix ah = conjTranspose(a);
    return solveLinear(multiply(ah, a), multiply(ah, b));
  }
  int n = a.rows, m = b.cols;
  Matrix A = a, B = b;
  double tol = 1e-13 * std::max(maxAbs(a), 1e-300) * n;
  for (int k = 0; k < n; ++k) {
    int piv = k;
    for (int r = k + 1; r < n; ++r)
      if (std::abs(A(r, k)) > std::abs(A(piv, k))) piv = r;
    if (std::abs(A(piv, k)) <= tol)
      throw CalcError("the system has no unique solution (the matrix is singular)");
    if (piv != k) {
      for (int c = 0; c < n; ++c) std::swap(A(k, c), A(piv, c));
      for (int c = 0; c < m; ++c) std::swap(B(k, c), B(piv, c));
    }
    for (int r = k + 1; r < n; ++r) {
      cplx f = A(r, k) / A(k, k);
      if (f == cplx(0)) continue;
      for (int c = k; c < n; ++c) A(r, c) -= f * A(k, c);
      for (int c = 0; c < m; ++c) B(r, c) -= f * B(k, c);
    }
  }
  Matrix x(n, m);
  for (int c = 0; c < m; ++c)
    for (int r = n - 1; r >= 0; --r) {
      cplx s = B(r, c);
      for (int k = r + 1; k < n; ++k) s -= A(r, k) * x(k, c);
      x(r, c) = s / A(r, r);
    }
  for (auto& z : x.a) z = chopTiny(z);
  return x;
}

Matrix rref(const Matrix& m0, int* rankOut, double tol) {
  Matrix m = m0;
  if (tol < 0) tol = 1e-12 * std::max(1.0, maxAbs(m0)) * std::max(m.rows, m.cols);
  int lead = 0, rank = 0;
  for (int r = 0; r < m.rows && lead < m.cols; ++r) {
    int piv = -1;
    while (lead < m.cols) {
      double best = tol;
      for (int i = r; i < m.rows; ++i)
        if (std::abs(m(i, lead)) > best) {
          best = std::abs(m(i, lead));
          piv = i;
        }
      if (piv >= 0) break;
      for (int i = r; i < m.rows; ++i) m(i, lead) = 0.0;
      ++lead;
    }
    if (piv < 0) break;
    for (int c = 0; c < m.cols; ++c) std::swap(m(r, c), m(piv, c));
    cplx p = m(r, lead);
    for (int c = 0; c < m.cols; ++c) m(r, c) /= p;
    m(r, lead) = 1.0;
    for (int i = 0; i < m.rows; ++i) {
      if (i == r) continue;
      cplx f = m(i, lead);
      if (f == cplx(0)) continue;
      for (int c = 0; c < m.cols; ++c) m(i, c) -= f * m(r, c);
      m(i, lead) = 0.0;
    }
    ++rank;
    ++lead;
  }
  for (auto& z : m.a) {
    z = chopTiny(z);
    if (std::abs(z) < tol) z = 0.0;
  }
  if (rankOut) *rankOut = rank;
  return m;
}

int rank(const Matrix& m) {
  int r = 0;
  rref(m, &r);
  return r;
}

cplx trace(const Matrix& m) {
  requireSquare(m, "trace");
  cplx t = 0;
  for (int i = 0; i < m.rows; ++i) t += m(i, i);
  return t;
}

double normFrobenius(const Matrix& m) {
  double s = 0;
  for (const auto& z : m.a) s += std::norm(z);
  return std::sqrt(s);
}

// --- eigenvalues: Householder reduction to Hessenberg form, then shifted QR.

static void hessenberg(Matrix& h) {
  int n = h.rows;
  for (int k = 0; k + 2 < n; ++k) {
    int len = n - k - 1;
    std::vector<cplx> v(len);
    double xnorm = 0;
    for (int i = 0; i < len; ++i) {
      v[i] = h(k + 1 + i, k);
      xnorm += std::norm(v[i]);
    }
    xnorm = std::sqrt(xnorm);
    if (xnorm == 0) continue;
    cplx phase = std::abs(v[0]) > 0 ? v[0] / std::abs(v[0]) : cplx(1.0);
    cplx alpha = -phase * xnorm;
    v[0] -= alpha;
    double vnorm = 0;
    for (const auto& z : v) vnorm += std::norm(z);
    vnorm = std::sqrt(vnorm);
    if (vnorm < 1e-300) continue;
    for (auto& z : v) z /= vnorm;
    // Left: H = (I - 2vv*) H on rows k+1..n-1
    for (int j = 0; j < n; ++j) {
      cplx w = 0;
      for (int i = 0; i < len; ++i) w += std::conj(v[i]) * h(k + 1 + i, j);
      for (int i = 0; i < len; ++i) h(k + 1 + i, j) -= 2.0 * v[i] * w;
    }
    // Right: H = H (I - 2vv*) on columns k+1..n-1
    for (int i = 0; i < n; ++i) {
      cplx w = 0;
      for (int j = 0; j < len; ++j) w += h(i, k + 1 + j) * v[j];
      for (int j = 0; j < len; ++j) h(i, k + 1 + j) -= 2.0 * w * std::conj(v[j]);
    }
    for (int i = k + 2; i < n; ++i) h(i, k) = 0.0;
  }
}

std::vector<cplx> eigenvalues(const Matrix& m) {
  requireSquare(m, "eig");
  int n = m.rows;
  Matrix h = m;
  hessenberg(h);
  std::vector<cplx> eig;
  const double eps = 2.220446049250313e-16;
  int hi = n - 1;
  int iter = 0, total = 0;
  while (hi >= 0) {
    if (hi == 0) {
      eig.push_back(h(0, 0));
      break;
    }
    // Find the start of the unreduced block ending at hi.
    int lo = hi;
    while (lo > 0) {
      double s = std::abs(h(lo, lo)) + std::abs(h(lo - 1, lo - 1));
      if (s == 0) s = normFrobenius(m);
      if (std::abs(h(lo, lo - 1)) <= eps * s) {
        h(lo, lo - 1) = 0.0;
        break;
      }
      --lo;
    }
    if (lo == hi) {
      eig.push_back(h(hi, hi));
      --hi;
      iter = 0;
      continue;
    }
    if (++total > 200 * n) throw CalcError("eigenvalue computation did not converge");
    ++iter;
    // Wilkinson shift from the trailing 2x2 block.
    cplx a = h(hi - 1, hi - 1), b = h(hi - 1, hi), c = h(hi, hi - 1), d = h(hi, hi);
    cplx tr = a + d, det = a * d - b * c;
    cplx disc = std::sqrt(tr * tr - 4.0 * det);
    cplx l1 = (tr + disc) / 2.0, l2 = (tr - disc) / 2.0;
    cplx mu = std::abs(l1 - d) < std::abs(l2 - d) ? l1 : l2;
    if (iter % 11 == 10) mu = d + cplx(std::abs(h(hi, hi - 1)) * 0.75, std::abs(h(hi, hi - 1)) * 0.3);

    for (int i = lo; i <= hi; ++i) h(i, i) -= mu;
    std::vector<cplx> cs(hi - lo), ss(hi - lo);
    for (int k = lo; k < hi; ++k) {
      cplx x = h(k, k), y = h(k + 1, k);
      double r = std::sqrt(std::norm(x) + std::norm(y));
      cplx cc = 1.0, sn = 0.0;
      if (r > 0) {
        cc = x / r;
        sn = y / r;
      }
      cs[k - lo] = cc;
      ss[k - lo] = sn;
      for (int j = k; j <= hi; ++j) {
        cplx t1 = h(k, j), t2 = h(k + 1, j);
        h(k, j) = std::conj(cc) * t1 + std::conj(sn) * t2;
        h(k + 1, j) = -sn * t1 + cc * t2;
      }
    }
    for (int k = lo; k < hi; ++k) {
      cplx cc = cs[k - lo], sn = ss[k - lo];
      int rmax = std::min(k + 2, hi);
      for (int i = lo; i <= rmax; ++i) {
        cplx t1 = h(i, k), t2 = h(i, k + 1);
        h(i, k) = t1 * cc + t2 * sn;
        h(i, k + 1) = -t1 * std::conj(sn) + t2 * std::conj(cc);
      }
    }
    for (int i = lo; i <= hi; ++i) h(i, i) += mu;
  }
  // Clean up and sort: real parts descending, then imaginary parts.
  double scale = std::max(1e-300, normFrobenius(m));
  for (auto& z : eig) {
    if (std::abs(z.imag()) < 1e-10 * scale) z = cplx(z.real(), 0.0);
    if (std::abs(z.real()) < 1e-13 * scale) z = cplx(0.0, z.imag());
    // Snap to integers when extremely close (common for textbook matrices).
    double rr = std::round(z.real()), ri = std::round(z.imag());
    if (std::abs(z.real() - rr) < 1e-10 * std::max(1.0, std::abs(rr))) z = cplx(rr, z.imag());
    if (std::abs(z.imag() - ri) < 1e-10 * std::max(1.0, std::abs(ri))) z = cplx(z.real(), ri);
  }
  std::sort(eig.begin(), eig.end(), [](const cplx& x, const cplx& y) {
    if (std::abs(x.real() - y.real()) > 1e-12 * std::max(1.0, std::abs(x.real())))
      return x.real() > y.real();
    return x.imag() > y.imag();
  });
  return eig;
}

Matrix nullspace(const Matrix& m, double tol) {
  int rk = 0;
  Matrix r = rref(m, &rk, tol);
  std::vector<int> pivotCol(r.rows, -1);
  std::vector<bool> isPivot(r.cols, false);
  for (int i = 0; i < r.rows; ++i)
    for (int c = 0; c < r.cols; ++c)
      if (std::abs(r(i, c)) > 0.5) {  // leading ones
        pivotCol[i] = c;
        isPivot[c] = true;
        break;
      }
  std::vector<std::vector<cplx>> basis;
  for (int f = 0; f < r.cols; ++f) {
    if (isPivot[f]) continue;
    std::vector<cplx> v(r.cols, 0.0);
    v[f] = 1.0;
    for (int i = 0; i < r.rows; ++i)
      if (pivotCol[i] >= 0) v[pivotCol[i]] = -r(i, f);
    basis.push_back(v);
  }
  Matrix out(m.cols, int(basis.size()));
  for (size_t j = 0; j < basis.size(); ++j)
    for (int i = 0; i < m.cols; ++i) out(i, int(j)) = basis[j][i];
  return out;
}

static void normalizeLargestToOne(std::vector<cplx>& v) {
  size_t best = 0;
  for (size_t i = 1; i < v.size(); ++i)
    if (std::abs(v[i]) > std::abs(v[best]) * (1 + 1e-9)) best = i;
  cplx p = v[best];
  if (p == cplx(0)) return;
  for (auto& z : v) z = chopTiny(z / p);
  v[best] = 1.0;
  for (auto& z : v) {
    double rr = std::round(z.real());
    if (z.imag() == 0.0 && std::abs(z.real() - rr) < 1e-9) z = rr;
  }
}

Matrix eigenvectors(const Matrix& m, std::vector<cplx>* valuesOut) {
  std::vector<cplx> vals = eigenvalues(m);
  if (valuesOut) *valuesOut = vals;
  int n = m.rows;
  double scale = std::max(1.0, normFrobenius(m));
  Matrix out(n, n);
  size_t col = 0;
  for (size_t i = 0; i < vals.size();) {
    // Group (numerically) repeated eigenvalues.
    size_t j = i + 1;
    while (j < vals.size() && std::abs(vals[j] - vals[i]) < 1e-7 * scale) ++j;
    size_t mult = j - i;
    Matrix shifted = m;
    for (int k = 0; k < n; ++k) shifted(k, k) -= vals[i];
    Matrix ns;
    for (double tol : {1e-10, 1e-8, 1e-6}) {
      ns = nullspace(shifted, tol * scale * n);
      if (ns.cols > 0) break;
    }
    std::vector<std::vector<cplx>> vecs;
    for (int c = 0; c < ns.cols && vecs.size() < mult; ++c) {
      std::vector<cplx> v(n);
      for (int r = 0; r < n; ++r) v[r] = ns(r, c);
      vecs.push_back(v);
    }
    if (vecs.empty()) {
      // Fall back to inverse iteration.
      Matrix b = m;
      cplx shift = vals[i] + cplx(1e-9 * scale, 0);
      for (int k = 0; k < n; ++k) b(k, k) -= shift;
      Matrix x(n, 1, 1.0);
      for (int k = 0; k < n; ++k) x(k, 0) = 1.0 + 0.1 * k;
      try {
        for (int it = 0; it < 4; ++it) {
          x = solveLinear(b, x);
          double nr = normFrobenius(x);
          for (auto& z : x.a) z /= nr;
        }
      } catch (const CalcError&) {
      }
      vecs.push_back(x.a);
    }
    while (vecs.size() < mult) vecs.push_back(vecs.back());  // defective matrix
    for (auto& v : vecs) {
      normalizeLargestToOne(v);
      for (int r = 0; r < n; ++r) out(r, int(col)) = v[r];
      ++col;
    }
    i = j;
  }
  return out;
}

}  // namespace cact
