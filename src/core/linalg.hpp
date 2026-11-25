// Cactulator - linear algebra on (complex) matrices.
#pragma once

#include <vector>

#include "value.hpp"

namespace cact {

Matrix transpose(const Matrix& m);
Matrix multiply(const Matrix& a, const Matrix& b);
cplx determinant(const Matrix& m);
Matrix inverse(const Matrix& m);
Matrix matrixPower(const Matrix& m, long long n);
// Solve A x = B. Square: Gaussian elimination; otherwise least squares.
Matrix solveLinear(const Matrix& a, const Matrix& b);
Matrix rref(const Matrix& m, int* rankOut = nullptr, double tol = -1);
int rank(const Matrix& m);
cplx trace(const Matrix& m);
double normFrobenius(const Matrix& m);
std::vector<cplx> eigenvalues(const Matrix& m);
// Eigenvectors as columns, each scaled so its largest entry is 1.
Matrix eigenvectors(const Matrix& m, std::vector<cplx>* valuesOut = nullptr);
Matrix nullspace(const Matrix& m, double tol);

}  // namespace cact
