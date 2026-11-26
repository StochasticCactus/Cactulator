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

// Cactulator - HP-style RPN stack engine.
#pragma once

#include <string>
#include <vector>

#include "interp.hpp"

namespace cact {

class Rpn {
 public:
  std::vector<Value> stack;  // back() is level 1

  // Process a line of space-separated RPN words: numbers, operators,
  // function names, stack commands (dup drop swap over rot clear), sto name,
  // 'algebraic' and [lists]. Atomic: on error the stack is restored.
  void process(Interp& in, const std::string& line);

  // Single operations used by keys.
  void enter(Interp& in, const std::string& entry);  // push entry, or DUP if empty
  void apply(Interp& in, const std::string& word);    // apply one word
  bool undo();
  void snapshot();

  static bool isStackWord(const std::string& w);

 private:
  std::vector<std::vector<Value>> undo_;
  Value pop(const char* what);
  void need(size_t n, const std::string& what) const;
  void word(Interp& in, const std::string& w, std::vector<std::string>& rest);
};

}  // namespace cact
