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

#include "rpn.hpp"

#include <algorithm>
#include <cctype>

#include "lexer.hpp"

namespace cact {

static std::vector<std::string> splitWords(const std::string& line) {
  std::vector<std::string> out;
  std::string cur;
  int depth = 0;
  char quote = 0;
  for (size_t i = 0; i < line.size(); ++i) {
    char c = line[i];
    if (quote) {
      cur += c;
      if (c == quote) quote = 0;
      continue;
    }
    if (c == '\'' || c == '"') {
      quote = c;
      cur += c;
      continue;
    }
    if (c == '[' || c == '(' || c == '{') ++depth;
    if (c == ']' || c == ')' || c == '}') --depth;
    if ((c == ' ' || c == '\t') && depth <= 0) {
      if (!cur.empty()) out.push_back(cur);
      cur.clear();
      continue;
    }
    cur += c;
  }
  if (quote) throw CalcError("missing closing quote");
  if (!cur.empty()) out.push_back(cur);
  return out;
}

bool Rpn::isStackWord(const std::string& w) {
  static const char* words[] = {"dup", "drop", "swap", "over", "rot", "clear", "clr", "depth", "dup2", "drop2"};
  for (const char* s : words)
    if (w == s) return true;
  return false;
}

void Rpn::snapshot() {
  undo_.push_back(stack);
  if (undo_.size() > 50) undo_.erase(undo_.begin());
}

bool Rpn::undo() {
  if (undo_.empty()) return false;
  stack = undo_.back();
  undo_.pop_back();
  return true;
}

void Rpn::need(size_t n, const std::string& what) const {
  if (stack.size() < n)
    throw CalcError(what + " needs " + std::to_string(n) + " value" + (n == 1 ? "" : "s") + " on the stack");
}

Value Rpn::pop(const char*) {
  Value v = stack.back();
  stack.pop_back();
  return v;
}

static bool isNumberWord(const std::string& w) {
  try {
    auto t = tokenize(w);
    if (t.size() == 2 && t[0].type == Tok::Num) return true;
    if (t.size() == 3 && t[0].type == Tok::Op && t[0].text == "-" && t[1].type == Tok::Num) return true;
  } catch (const CalcError&) {
  }
  return false;
}

void Rpn::word(Interp& in, const std::string& w0, std::vector<std::string>& rest) {
  std::string w = w0;
  if (w == "×") w = "*";
  if (w == "÷") w = "/";
  if (w == "−") w = "-";

  if (isNumberWord(w) || w[0] == '[' || w[0] == '{' || w[0] == '(') {
    stack.push_back(in.evalString(w));
    return;
  }
  if (w.size() >= 2 && w.front() == '\'' && w.back() == '\'') {
    stack.push_back(in.evalString(w.substr(1, w.size() - 2)));
    return;
  }
  if (w.size() >= 2 && w.front() == '"' && w.back() == '"') {
    stack.push_back(Value::string(w.substr(1, w.size() - 2)));
    return;
  }
  if (w == "+" || w == "-" || w == "*" || w == "/" || w == "^" || w == "%" || w == "\\") {
    need(2, "'" + w + "'");
    Value b = pop("y"), a = pop("x");
    try {
      stack.push_back(binaryOp(w, a, b));
    } catch (...) {
      stack.push_back(a);
      stack.push_back(b);
      throw;
    }
    return;
  }
  if (w == "neg" || w == "chs" || w == "±" || w == "+/-") {
    need(1, "neg");
    stack.back() = negate(stack.back());
    return;
  }
  if (w == "dup") {
    need(1, "dup");
    stack.push_back(stack.back());
    return;
  }
  if (w == "dup2") {
    need(2, "dup2");
    Value a = stack[stack.size() - 2], b = stack.back();
    stack.push_back(a);
    stack.push_back(b);
    return;
  }
  if (w == "drop") {
    need(1, "drop");
    stack.pop_back();
    return;
  }
  if (w == "drop2") {
    need(2, "drop2");
    stack.pop_back();
    stack.pop_back();
    return;
  }
  if (w == "swap") {
    need(2, "swap");
    std::swap(stack[stack.size() - 1], stack[stack.size() - 2]);
    return;
  }
  if (w == "over") {
    need(2, "over");
    stack.push_back(stack[stack.size() - 2]);
    return;
  }
  if (w == "rot") {
    need(3, "rot");
    Value a = stack[stack.size() - 3];
    stack.erase(stack.end() - 3);
    stack.push_back(a);
    return;
  }
  if (w == "clear" || w == "clr") {
    stack.clear();
    return;
  }
  if (w == "depth") {
    stack.push_back(Value(double(stack.size())));
    return;
  }
  if (w == "sq" || w == "²") {
    need(1, "x²");
    stack.back() = binaryOp("^", stack.back(), Value(2.0));
    return;
  }
  if (w == "!") w = "fact";
  if (w == "√") w = "sqrt";
  if (w == "⁻¹") w = "inv";
  if (w == "sto" || w == "→" || w == "->") {
    if (rest.empty()) throw CalcError("sto needs a variable name: 5 sto x");
    std::string name = rest.front();
    rest.erase(rest.begin());
    need(1, "sto");
    in.setVar(name, stack.back());
    stack.pop_back();
    return;
  }
  if (w.rfind("→", 0) == 0 || w.rfind("->", 0) == 0) {
    std::string name = w.substr(w.rfind("→", 0) == 0 ? std::string("→").size() : 2);
    need(1, "sto");
    in.setVar(name, stack.back());
    stack.pop_back();
    return;
  }

  // Functions
  auto uf = in.funcs.find(w);
  if (uf != in.funcs.end()) {
    size_t n = uf->second.params.size();
    need(n, w);
    std::vector<Value> args(stack.end() - long(n), stack.end());
    Value r = in.call(w, args);
    stack.resize(stack.size() - n);
    if (!r.isNone()) stack.push_back(r);
    return;
  }
  if (const Builtin* b = findBuiltin(w)) {
    if (b->lazy) {
      if ((w == "sum" || w == "prod") && !stack.empty() && stack.back().isMat()) {
        cplx acc = w == "sum" ? 0.0 : 1.0;
        for (const auto& z : stack.back().elements()) acc = w == "sum" ? acc + z : acc * z;
        stack.back() = Value(acc);
        return;
      }
      throw CalcError(w + " works on expressions; type it in quotes, e.g. '" + b->signature + "'");
    }
    size_t n = size_t(std::max(0, b->minArgs));
    if (b->maxArgs < 0) n = (!stack.empty() && stack.back().isMat()) ? 1 : 2;
    need(n, w);
    std::vector<Value> args(stack.end() - long(n), stack.end());
    Value r = in.call(w, args);
    stack.resize(stack.size() - n);
    stack.push_back(r);
    return;
  }
  Value v;
  if (in.getVar(w, v)) {
    stack.push_back(v);
    return;
  }
  throw CalcError("unknown word '" + w + "'" + in.suggest(w));
}

void Rpn::process(Interp& in, const std::string& line) {
  std::vector<Value> saved = stack;
  snapshot();
  try {
    std::vector<std::string> words = splitWords(line);
    while (!words.empty()) {
      std::string w = words.front();
      words.erase(words.begin());
      word(in, w, words);
    }
  } catch (...) {
    stack = saved;
    if (!undo_.empty()) undo_.pop_back();
    throw;
  }
}

void Rpn::enter(Interp& in, const std::string& entry) {
  bool blank = entry.find_first_not_of(" \t") == std::string::npos;
  if (blank) {
    if (stack.empty()) return;
    snapshot();
    stack.push_back(stack.back());
    return;
  }
  process(in, entry);
}

void Rpn::apply(Interp& in, const std::string& w) { process(in, w); }

}  // namespace cact
