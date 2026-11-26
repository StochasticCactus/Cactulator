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
