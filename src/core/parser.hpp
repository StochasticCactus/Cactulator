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

// Cactulator - parser for expressions and the program language.
#pragma once

#include <functional>
#include <string>

#include "ast.hpp"
#include "lexer.hpp"

namespace cact {

// Parse a single expression (the whole input must be consumed).
NodeP parseExpression(const std::string& src);

// Parse a sequence of statements (command line or program source).
Block parseStatements(const std::string& src);

// Used by the parser to allow "sin 30" (call without parentheses).
void setFunctionNamePredicate(std::function<bool(const std::string&)> pred);

// True if the source looks like an unfinished block (e.g. "prog f(x)" without
// "end"); used by the REPL to keep reading lines.
bool needsMoreInput(const std::string& src);

}  // namespace cact
