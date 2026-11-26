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

// Cactulator - SOLVE app: HP-style equation solver (solve for any variable)
// and a polynomial root finder.
#include <memory>
#include <algorithm>
#include <cmath>

#include "app.hpp"
#include "core/format.hpp"
#include "core/numeric.hpp"
#include "core/parser.hpp"

namespace ui {

using cact::u8len;
using cact::Value;

namespace {

const char* kExamples[] = {
    "x^2 - 5x + 6 = 0",
    "P*V = n*_R*T",
    "v = u + a*t",
    "A = P*(1 + r/100)^t",
    "a^2 + b^2 = c^2",
    "F = m*a",
    "pH = -log(H)",
    "cos(x) = x/90",
};

class SolveScreen : public Screen {
 public:
  explicit SolveScreen(App& a) : Screen(a) {
    eq_.set(kExamples[0]);
    poly_.set("1, -3, 2");
  }

  std::string title() const override { return poly_mode_ ? "POLYNOMIAL ROOTS" : "EQUATION SOLVER"; }

  void enter(const std::string& arg) override {
    poly_mode_ = arg == "poly";
    message_.clear();
    refreshVars();
    if (poly_mode_) computePoly();
  }

  std::vector<SoftKey> softKeys() override {
    return {
        {"Solve", [this] { poly_mode_ ? computePoly() : solveSelected(); }},
        {"Equation", [this] { poly_mode_ = false; refreshVars(); }, !poly_mode_},
        {"Poly", [this] { poly_mode_ = true; computePoly(); }, poly_mode_},
        {"Example", [this] { nextExample(); }},
        {"Clear", [this] { clearAll(); }},
        {"", nullptr},
        {"", nullptr},
        {"Home", [this] { app.back(); }},
    };
  }

  void draw(const Rect& r) override {
    if (poly_mode_) drawPoly(r);
    else drawEquation(r);
  }

  bool key(const Event& e) override { return poly_mode_ ? keyPoly(e) : keyEquation(e); }

  void insert(const std::string& text) override {
    if (poly_mode_) {
      poly_.insert(text);
      computePoly();
    } else if (field_ == 0) {
      eq_.insert(text);
    } else {
      if (!valEditing_) {
        valEditing_ = true;
        val_.clear();
      }
      val_.insert(text);
    }
  }

  bool mouse(const Event& e, const Rect&) override {
    if (e.mouse != Event::Mouse::Press || poly_mode_) return false;
    for (const auto& [y, idx] : rows_)
      if (y == e.my) {
        commitValue();
        field_ = idx;
        return true;
      }
    return false;
  }

 private:
  bool poly_mode_ = false;
  LineEdit eq_, val_, poly_;
  int field_ = 0;  // 0 = equation, 1.. = variables
  bool valEditing_ = false;
  std::vector<std::string> vars_;
  std::string message_;
  bool messageError_ = false;
  int example_ = 0;
  std::vector<cact::cplx> roots_;
  std::vector<std::pair<int, int>> rows_;

  Term& term() { return app.term; }

  std::string fmt(double v) const {
    cact::DisplaySettings d = app.s.in.settings.display;
    d.base = 10;
    d.frac = false;
    return cact::formatReal(v, d);
  }

  void refreshVars() {
    vars_.clear();
    try {
      auto n = cact::parseExpression(eq_.text);
      std::vector<std::string> all;
      cact::collectVars(n, all);
      for (const auto& v : all) {
        bool userVar = app.s.in.globals.count(v) > 0;
        if (!userVar && (cact::findConstant(v) || v == "i")) continue;
        vars_.push_back(v);
      }
      if (message_.rfind("Syntax", 0) == 0) message_.clear();
    } catch (const cact::CalcError& e) {
      message_ = e.what();
      messageError_ = true;
    }
    field_ = std::min(field_, int(vars_.size()));
  }

  bool hasValue(const std::string& v, double& out) const {
    auto it = app.s.in.globals.find(v);
    if (it == app.s.in.globals.end() || !it->second.isRealNum()) return false;
    out = it->second.num.real();
    return true;
  }

  void drawEquation(const Rect& r) {
    const Theme& t = app.th();
    rows_.clear();
    term().put(r.x, r.y, "Equation", Style(field_ == 0 ? t.lcdAccent : t.lcdDim, t.lcdBg, true));
    Style fieldSt = field_ == 0 ? app.lcdSel() : app.lcd();
    term().fill({r.x + 10, r.y, r.w - 10, 1}, fieldSt);
    ui::drawEdit(term(), r.x + 10, r.y, r.w - 10, eq_, t, fieldSt, field_ == 0,
                 highlight(eq_.text, t, &app.s.in, field_ == 0 ? long(eq_.cur) : -1));
    rows_.push_back({r.y, 0});
    term().put(r.x, r.y + 2, "Variables — give values to all but one, select the unknown and press F1 Solve",
               app.lcdDim(), r.w);
    int y = r.y + 3;
    for (size_t i = 0; i < vars_.size() && y < r.y + r.h - 2; ++i, ++y) {
      bool sel = field_ == int(i) + 1;
      Style st = sel ? app.lcdSel() : app.lcd();
      if (sel) term().fill({r.x, y, std::min(r.w, 40), 1}, st);
      term().put(r.x + 2, y, cact::padLeft(vars_[i], 8) + " =", Style(st.fg, st.bg, true));
      double v;
      if (sel && valEditing_) {
        ui::drawEdit(term(), r.x + 13, y, std::min(26, r.w - 13), val_, t, st, true,
                     highlight(val_.text, t, &app.s.in, long(val_.cur)));
      } else if (hasValue(vars_[i], v)) {
        term().put(r.x + 13, y, fmt(v), st, std::min(26, r.w - 13));
      } else {
        term().put(r.x + 13, y, "? (unknown)", Style(t.lcdDim, st.bg), std::min(26, r.w - 13));
      }
      rows_.push_back({y, int(i) + 1});
    }
    if (vars_.empty())
      term().put(r.x + 2, y++, "(type an equation above, then press Enter)", app.lcdDim());
    if (!message_.empty()) {
      int my = r.y + r.h - 2;
      for (const auto& l : wrapText(message_, r.w))
        if (my < r.y + r.h) term().put(r.x, my++, l, messageError_ ? app.lcdError() : app.lcdStrong());
    } else {
      term().put(r.x, r.y + r.h - 1, "↑↓ move · type a value · F1 solves for the selected variable · F4 examples",
                 app.lcdDim(), r.w);
    }
  }

  void commitValue() {
    if (!valEditing_ || field_ < 1) return;
    valEditing_ = false;
    const std::string& name = vars_[size_t(field_ - 1)];
    std::string text = cact::trim(val_.text);
    try {
      if (text.empty()) app.s.in.globals.erase(name);
      else app.s.in.setVar(name, app.s.in.evalString(text));
      message_.clear();
    } catch (const cact::CalcError& e) {
      message_ = e.what();
      messageError_ = true;
    }
  }

  bool keyEquation(const Event& e) {
    if (e.is(keys::Up) || e.is(keys::Down)) {
      commitValue();
      if (field_ == 0) refreshVars();
      field_ = std::clamp(field_ + (e.is(keys::Up) ? -1 : 1), 0, int(vars_.size()));
      return true;
    }
    if (field_ == 0) {
      if (e.is(keys::Enter)) {
        refreshVars();
        if (!vars_.empty()) field_ = 1;
        return true;
      }
      if (e.is(keys::Esc)) return false;
      bool used = eq_.handle(e);
      if (used) refreshVars();
      return used;
    }
    if (valEditing_) {
      if (e.is(keys::Enter)) {
        commitValue();
        field_ = std::min(field_ + 1, int(vars_.size()));
        return true;
      }
      if (e.is(keys::Esc)) {
        valEditing_ = false;
        return true;
      }
      return val_.handle(e);
    }
    if (e.type == Event::Type::Text) {
      valEditing_ = true;
      val_.clear();
      val_.insert(e.text);
      return true;
    }
    if (e.is(keys::Enter)) {
      solveSelected();
      return true;
    }
    if (e.is(keys::Delete) || e.is(keys::Backspace)) {
      app.s.in.globals.erase(vars_[size_t(field_ - 1)]);
      return true;
    }
    return false;
  }

  void solveSelected() {
    commitValue();
    refreshVars();
    if (vars_.empty()) {
      message_ = "Type an equation first, like  x^2 = 2";
      messageError_ = true;
      return;
    }
    // Pick the unknown: the selected variable, or the only one without a value.
    std::string unknown;
    if (field_ >= 1) unknown = vars_[size_t(field_ - 1)];
    else {
      for (const auto& v : vars_) {
        double tmp;
        if (!hasValue(v, tmp)) {
          if (!unknown.empty()) {
            message_ = "Several variables have no value — select the one to solve for";
            messageError_ = true;
            return;
          }
          unknown = v;
        }
      }
      if (unknown.empty()) unknown = vars_[0];
    }
    for (const auto& v : vars_) {
      double tmp;
      if (v != unknown && !hasValue(v, tmp)) {
        message_ = "Give " + v + " a value first (or select it to solve for it instead)";
        messageError_ = true;
        return;
      }
    }
    try {
      cact::NodeP eq = cact::parseExpression(eq_.text);
      cact::NodeP diff = eq;
      if (eq->k == cact::NK::Eq) {
        auto n = std::make_shared<cact::Node>();
        n->k = cact::NK::Binary;
        n->name = "-";
        n->args = {eq->args[0], eq->args[1]};
        diff = n;
      }
      double guess = 1, cur;
      if (hasValue(unknown, cur)) guess = cur;
      app.s.in.globals.erase(unknown);
      app.s.in.probe(diff, unknown, guess);
      auto f = app.s.in.realFunction(diff, unknown);
      double x = cact::findRoot(f, guess);
      double rx = std::round(x);
      if (std::abs(x - rx) < 1e-11 * std::max(1.0, std::abs(rx))) x = rx;
      app.s.in.setVar(unknown, Value(x));
      app.s.in.globals["ans"] = Value(x);
      double res = f(x);
      message_ = unknown + " = " + fmt(x) + "      (left − right = " + fmt(res) + ")";
      messageError_ = false;
      field_ = int(std::find(vars_.begin(), vars_.end(), unknown) - vars_.begin()) + 1;
    } catch (const cact::CalcError& e) {
      message_ = e.what();
      messageError_ = true;
    }
  }

  void nextExample() {
    if (poly_mode_) {
      static const char* polys[] = {"1, -3, 2", "1, 0, -2", "1, 0, 1", "1, -6, 11, -6", "2, 3, -5, 0, 1"};
      static int p = 0;
      p = (p + 1) % 5;
      poly_.set(polys[p]);
      computePoly();
      return;
    }
    example_ = (example_ + 1) % int(sizeof(kExamples) / sizeof(kExamples[0]));
    eq_.set(kExamples[example_]);
    field_ = 0;
    message_.clear();
    refreshVars();
  }

  void clearAll() {
    if (poly_mode_) {
      poly_.clear();
      roots_.clear();
      return;
    }
    for (const auto& v : vars_) app.s.in.globals.erase(v);
    message_ = "Cleared the values of the variables";
    messageError_ = false;
  }

  // ---- polynomial

  std::string polyText(const std::vector<double>& c) const {
    static const char* sup[] = {"", "", "²", "³", "⁴", "⁵", "⁶", "⁷", "⁸", "⁹"};
    std::string s;
    int n = int(c.size()) - 1;
    for (int i = 0; i <= n; ++i) {
      double a = c[size_t(i)];
      if (a == 0) continue;
      int p = n - i;
      std::string coef = fmt(std::abs(a));
      if (std::abs(a) == 1 && p > 0) coef = "";
      std::string term = coef + (p > 0 ? "x" : "") + (p > 1 ? (p <= 9 ? sup[p] : "^" + std::to_string(p)) : "");
      if (s.empty()) s = (a < 0 ? "-" : "") + term;
      else s += (a < 0 ? " − " : " + ") + term;
    }
    return s.empty() ? "0" : s;
  }

  void computePoly() {
    roots_.clear();
    message_.clear();
    try {
      Value v = app.s.in.evalString("[" + poly_.text + "]");
      roots_ = cact::polyRoots(v.elements());
      app.s.in.globals["ans"] = Value::vector(roots_);
    } catch (const cact::CalcError& e) {
      if (!cact::trim(poly_.text).empty()) {
        message_ = e.what();
        messageError_ = true;
      }
    }
  }

  void drawPoly(const Rect& r) {
    const Theme& t = app.th();
    term().put(r.x, r.y, "Coefficients, highest power first (separated by commas):", app.lcdDim(), r.w);
    term().fill({r.x, r.y + 1, r.w, 1}, app.lcdSel());
    ui::drawEdit(term(), r.x, r.y + 1, r.w, poly_, t, app.lcdSel(), true, highlight(poly_.text, t, &app.s.in, long(poly_.cur)));
    std::vector<double> c;
    try {
      for (const auto& z : app.s.in.evalString("[" + poly_.text + "]").elements()) c.push_back(z.real());
    } catch (const cact::CalcError&) {
    }
    int y = r.y + 3;
    if (!c.empty()) term().put(r.x, y++, "p(x) = " + polyText(c), app.lcdStrong(), r.w);
    ++y;
    if (!roots_.empty()) {
      term().put(r.x, y++, std::string("Roots (") + std::to_string(roots_.size()) + "):", app.lcdAccent());
      for (size_t i = 0; i < roots_.size() && y < r.y + r.h - 1; ++i, ++y) {
        std::string s = "x" + std::to_string(i + 1) + " = " + cact::formatNumber(roots_[i], app.s.in.settings.display);
        term().put(r.x + 2, y, s, app.lcd(), r.w - 2);
      }
    }
    if (!message_.empty()) term().put(r.x, r.y + r.h - 1, message_, messageError_ ? app.lcdError() : app.lcdAccent(), r.w);
    else term().put(r.x, r.y + r.h - 1, "Example: 1, -3, 2 means x² − 3x + 2 · complex roots are shown too · roots → ans", app.lcdDim(), r.w);
  }

  bool keyPoly(const Event& e) {
    if (e.is(keys::Esc)) return false;
    if (e.is(keys::Enter)) {
      computePoly();
      return true;
    }
    bool used = poly_.handle(e);
    if (used) computePoly();
    return used;
  }
};

}  // namespace

std::unique_ptr<Screen> makeSolveScreen(App& a) { return std::make_unique<SolveScreen>(a); }

}  // namespace ui
