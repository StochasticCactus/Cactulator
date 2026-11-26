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

// Cactulator - tokenizer.
#pragma once

#include <string>
#include <vector>

namespace cact {

enum class Tok { Num, Ident, Str, Op, LParen, RParen, LBrack, RBrack, Comma, Semi, Newline, End };

struct Token {
  Tok type = Tok::End;
  std::string text;  // identifier / operator / string contents
  double num = 0.0;
  bool imag = false;  // numeric literal followed by i (4i)
  int pos = 0;        // byte offset
  int end = 0;        // byte offset one past the token
  int line = 1;
};

// Tokenize source text. Unicode operators (× ÷ − √ ² ³ ° ≤ ≥ ≠ → π ∞ ⁻¹) are
// normalized. Throws CalcError on bad input.
std::vector<Token> tokenize(const std::string& src);

bool isKeyword(const std::string& s);

// Column (in characters, 1-based) for a byte offset, for error messages.
int columnOf(const std::string& src, int pos);

}  // namespace cact
