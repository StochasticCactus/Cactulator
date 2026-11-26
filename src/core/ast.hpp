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

// Cactulator - syntax tree for expressions and program statements.
#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "value.hpp"

namespace cact {

enum class NK {
  Num,      // numeric literal (possibly imaginary)
  Str,      // "text"
  Var,      // identifier
  Call,     // name(args)
  Index,    // base[i, j]       args[0] = base, args[1..] = indices
  Neg,      // -x
  Not,      // not x
  Binary,   // + - * / ^ % \    (name holds the operator)
  Compare,  // == != < > <= >=
  And,
  Or,
  Postfix,  // ! ' ² ³ ° ⁻¹
  MatLit,   // [a, b; c, d]
  Eq,       // lhs = rhs (equation / comparison / assignment target)
};

struct Node;
using NodeP = std::shared_ptr<Node>;

struct Node {
  NK k = NK::Num;
  int pos = 0;  // byte offset in the source, for error messages
  cplx num{0.0, 0.0};
  std::string name;
  std::vector<NodeP> args;
  std::vector<std::vector<NodeP>> rows;
};

enum class SK {
  Expr,
  Assign,
  IndexAssign,
  FuncDef,
  ProgDef,
  If,
  While,
  For,
  ForIn,
  Repeat,
  Return,
  Break,
  Continue,
  Print,
  Global,
};

struct Stmt;
using StmtP = std::shared_ptr<Stmt>;
using Block = std::vector<StmtP>;

struct Stmt {
  SK k = SK::Expr;
  int line = 1;
  NodeP expr;                       // value / condition / return value
  std::string name;                 // assignment target, function/program name, loop variable
  std::vector<NodeP> idx;           // indices for IndexAssign
  std::vector<std::string> params;  // FuncDef / ProgDef parameters, Global names
  std::vector<NodeP> items;         // print arguments
  NodeP from, to, step;             // for loops
  std::vector<std::pair<NodeP, Block>> branches;  // if / elif
  Block elseBody;
  Block body;
  std::string source;  // original text of a definition
};

// Collect identifiers used as variables in an expression (for the solver).
void collectVars(const NodeP& n, std::vector<std::string>& out);

}  // namespace cact
