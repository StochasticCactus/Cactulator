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

// Cactulator - the interpreter: variables, user functions, programs, builtins.
#pragma once

#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "ast.hpp"
#include "format.hpp"
#include "value.hpp"

namespace cact {

enum class AngleMode { DEG, RAD };

struct Settings {
  AngleMode angle = AngleMode::DEG;
  DisplaySettings display;
};

struct UserFunc {
  std::string name;
  std::vector<std::string> params;
  bool isProg = false;
  NodeP expr;   // one-line function body
  Block body;   // program body
  std::string source;
};

class Interp;

// A built-in function and its documentation.
struct Builtin {
  std::string name;
  int minArgs = 1, maxArgs = 1;  // maxArgs < 0 means "any number"
  std::string category, signature, help, example;
  std::function<Value(Interp&, std::vector<Value>&)> fn;                 // eager
  std::function<Value(Interp&, const std::vector<NodeP>&)> lazy;         // gets unevaluated args
};

const std::vector<Builtin>& builtinList();
const Builtin* findBuiltin(const std::string& name);

struct Constant {
  std::string name, category, help;
  double value;
};
const std::vector<Constant>& constantList();
const Constant* findConstant(const std::string& name);

struct ExecResult {
  Value value;
  bool hasValue = false;
  std::string message;  // e.g. "Defined f(x)"
};

class Interp {
 public:
  Interp();

  Settings settings;
  std::map<std::string, Value> globals;
  std::map<std::string, UserFunc> funcs;

  // Hooks supplied by the UI / REPL.
  std::function<void(const std::string&)> onPrint;
  std::function<bool(const std::string& prompt, std::string& answer)> onInput;
  std::function<bool()> checkInterrupt;

  // Run statements typed at the command line (or loaded from a file).
  ExecResult execute(const std::string& src);
  // Evaluate a standalone expression.
  Value evalString(const std::string& src);
  Value eval(const NodeP& n);

  // Call any function by name (user function, program or builtin).
  Value call(const std::string& name, std::vector<Value> args);

  // Make a real-valued function of one variable from an expression; errors
  // during evaluation yield NaN. `probe` evaluates once with errors reported.
  std::function<double(double)> realFunction(const NodeP& expr, const std::string& var);
  std::function<cplx(cplx)> complexFunction(const NodeP& expr, const std::string& var);
  void probe(const NodeP& expr, const std::string& var, double at);

  // Evaluate `expr` with extra variable bindings (transparent scope).
  Value evalWith(const NodeP& expr, const std::map<std::string, Value>& binds);

  // Angle helpers honoring the current mode.
  double toRadians(double a) const;
  double fromRadians(double r) const;
  cplx toRadians(cplx a) const;
  cplx fromRadians(cplx r) const;

  bool isFunctionName(const std::string& name) const;
  bool isReservedName(const std::string& name) const;
  std::string suggest(const std::string& name) const;  // "did you mean ...?"
  std::vector<std::string> allNames() const;           // for completion

  void setVar(const std::string& name, const Value& v);   // global, with checks
  bool getVar(const std::string& name, Value& out) const;  // locals then globals then constants
  void defineFunction(const std::string& name, const std::vector<std::string>& params, const NodeP& body,
                      const std::string& source);
  void defineProgram(const StmtP& def);

  // For long computations: polls checkInterrupt every few thousand steps.
  void tick();

 private:
  struct Frame {
    std::map<std::string, Value> vars;
    std::set<std::string> globalNames;
    bool opaque = true;  // function call frames hide outer locals
  };
  std::vector<Frame> frames_;
  int callDepth_ = 0;
  long long steps_ = 0;

  enum class Flow { Normal, Break, Continue, Return };
  Flow execBlock(const Block& b, Value& ret, Value& last);
  Flow execStmt(const StmtP& s, Value& ret, Value& last);
  void assign(const std::string& name, const Value& v);
  void indexAssign(const std::string& name, const std::vector<NodeP>& idx, const Value& v);
  Value callUser(const UserFunc& f, std::vector<Value>& args);
  Value evalCall(const NodeP& n);
  Value evalMatLit(const NodeP& n);
  Value lookup(const std::string& name, int pos) const;

  friend struct BindingScope;
  friend class ScopedBinding;

 public:
  // RAII helper for special forms: pushes a transparent frame with one variable.
  class ScopedBinding {
   public:
    ScopedBinding(Interp& in, const std::string& var, const Value& v);
    ~ScopedBinding();
    void set(const Value& v);

   private:
    Interp& in_;
    size_t index_;
    std::string var_;
  };
};

// Arithmetic on values (shared with the RPN engine).
Value binaryOp(const std::string& op, const Value& a, const Value& b);
Value negate(const Value& a);
cplx powc(cplx a, cplx b);
Value mapNumbers(const Value& v, const std::function<cplx(cplx)>& f);

}  // namespace cact
