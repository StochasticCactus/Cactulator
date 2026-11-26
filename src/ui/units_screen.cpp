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

// Cactulator - UNITS app: convert between units, showing every unit of the category.
#include <memory>
#include <algorithm>

#include "app.hpp"
#include "core/format.hpp"
#include "core/units.hpp"

namespace ui {

using cact::u8len;

namespace {

class UnitsScreen : public Screen {
 public:
  explicit UnitsScreen(App& a) : Screen(a) {
    value_.set("1");
    to_ = 1;
  }

  std::string title() const override { return "UNIT CONVERTER"; }

  std::vector<SoftKey> softKeys() override {
    return {
        {"Swap", [this] { std::swap(from_, to_); }},
        {"→ Home", [this] { sendHome(); }},
        {"Prev cat", [this] { changeCat(-1); }},
        {"Next cat", [this] { changeCat(1); }},
        {"", nullptr},
        {"", nullptr},
        {"", nullptr},
        {"Home", [this] { app.back(); }},
    };
  }

  void draw(const Rect& r) override {
    const Theme& t = app.th();
    const auto& cats = cact::unitCategories();
    const auto& cat = cats[size_t(cat_)];
    rows_.clear();
    auto field = [&](int idx, int y, const std::string& label, const std::string& value, bool arrows) {
      bool sel = sel_ == idx;
      Style st = sel ? app.lcdSel() : app.lcd();
      term().put(r.x, y, cact::padLeft(label, 9), Style(sel ? t.lcdAccent : t.lcdDim, t.lcdBg, true));
      std::string v = arrows ? (app.term.ascii ? "< " : "◂ ") + value + (app.term.ascii ? " >" : " ▸") : value;
      term().fill({r.x + 10, y, std::min(r.w - 10, 34), 1}, st);
      term().put(r.x + 11, y, v, Style(st.fg, st.bg, true), std::min(r.w - 11, 33));
      rows_.push_back({y, idx});
    };
    field(0, r.y, "Category", cat.name, true);
    // Value field with editor
    {
      bool sel = sel_ == 1;
      Style st = sel ? app.lcdSel() : app.lcd();
      term().put(r.x, r.y + 1, cact::padLeft("Value", 9), Style(sel ? t.lcdAccent : t.lcdDim, t.lcdBg, true));
      term().fill({r.x + 10, r.y + 1, std::min(r.w - 10, 34), 1}, st);
      ui::drawEdit(term(), r.x + 11, r.y + 1, std::min(r.w - 11, 33), value_, t, st, sel,
                   highlight(value_.text, t, &app.s.in, sel ? long(value_.cur) : -1));
      rows_.push_back({r.y + 1, 1});
    }
    field(2, r.y + 2, "From", cat.units[size_t(from_)].name + "  " + cat.units[size_t(from_)].label, true);
    field(3, r.y + 3, "To", cat.units[size_t(to_)].name + "  " + cat.units[size_t(to_)].label, true);

    double v = 0;
    bool ok = value(v);
    int y = r.y + 5;
    if (ok) {
      double res = cact::convertUnits(v, cat_, from_, to_);
      std::string line = fmt(v) + " " + cat.units[size_t(from_)].name + "  =  " + fmt(res) + " " + cat.units[size_t(to_)].name;
      term().put(r.x + 2, y, line, Style(t.lcdStrong, t.lcdBg, true), r.w - 2);
    } else {
      term().put(r.x + 2, y, "Type a number (expressions like 3*12 work too)", app.lcdError(), r.w - 2);
    }
    // Table of all units
    y += 2;
    tableRows_.clear();
    if (ok && y < r.y + r.h - 1) {
      term().put(r.x, y++, "In every " + cat.name + " unit (click to pick):", app.lcdDim(), r.w);
      int colW = 30;
      int ncol = std::max(1, r.w / colW);
      int nrows = (int(cat.units.size()) + ncol - 1) / ncol;
      for (size_t i = 0; i < cat.units.size(); ++i) {
        int c = int(i) / nrows, rr = int(i) % nrows;
        int yy = y + rr;
        if (yy >= r.y + r.h - 1) continue;
        double res = cact::convertUnits(v, cat_, from_, int(i));
        std::string s = cact::padLeft(cact::formatShort(res, 14, plainDs()), 15) + " " + cat.units[i].name;
        bool cur = int(i) == to_;
        term().put(r.x + c * colW, yy, s, cur ? Style(t.lcdAccent, t.lcdBg, true) : app.lcd(), colW - 1);
        tableRows_.push_back({Rect{r.x + c * colW, yy, colW - 1, 1}, int(i)});
      }
    }
    term().put(r.x, r.y + r.h - 1, "↑↓ field · ←→ change · type a value · tip: on HOME you can type  5 km to mi",
               app.lcdDim(), r.w);
  }

  bool key(const Event& e) override {
    if (e.is(keys::Up)) sel_ = std::max(0, sel_ - 1);
    else if (e.is(keys::Down) || e.is(keys::Tab) || e.is(keys::Enter)) sel_ = std::min(3, sel_ + 1);
    else if (sel_ == 1 && e.type == Event::Type::Text) value_.insert(e.text);
    else if (sel_ == 1 && (e.is(keys::Backspace) || e.is(keys::Delete) || e.is(keys::Left) || e.is(keys::Right) ||
                           e.is(keys::Home) || e.is(keys::End)))
      value_.handle(e);
    else if (e.is(keys::Left) || e.is(keys::Right)) change(e.is(keys::Left) ? -1 : 1);
    else if (e.type == Event::Type::Text) {
      sel_ = 1;
      value_.clear();
      value_.insert(e.text);
    } else return false;
    return true;
  }

  void insert(const std::string& text) override {
    if (sel_ != 1) {
      sel_ = 1;
      value_.clear();
    }
    value_.insert(text);
  }

  bool mouse(const Event& e, const Rect&) override {
    if (e.mouse != Event::Mouse::Press) return false;
    for (const auto& [rc, idx] : tableRows_)
      if (rc.contains(e.mx, e.my)) {
        to_ = idx;
        return true;
      }
    for (const auto& [y, idx] : rows_)
      if (y == e.my) {
        sel_ = idx;
        return true;
      }
    return false;
  }

 private:
  int cat_ = 0, from_ = 0, to_ = 1, sel_ = 1;
  LineEdit value_;
  std::vector<std::pair<int, int>> rows_;
  std::vector<std::pair<Rect, int>> tableRows_;

  Term& term() { return app.term; }

  cact::DisplaySettings plainDs() const {
    cact::DisplaySettings d = app.s.in.settings.display;
    d.base = 10;
    d.frac = false;
    return d;
  }

  std::string fmt(double v) const { return cact::formatReal(v, plainDs()); }

  bool value(double& v) {
    try {
      v = app.s.in.evalString(value_.text).real();
      return true;
    } catch (const cact::CalcError&) {
      return false;
    }
  }

  void changeCat(int d) {
    int n = int(cact::unitCategories().size());
    cat_ = (cat_ + d + n) % n;
    from_ = 0;
    to_ = std::min(1, int(cact::unitCategories()[size_t(cat_)].units.size()) - 1);
  }

  void change(int d) {
    const auto& units = cact::unitCategories()[size_t(cat_)].units;
    int n = int(units.size());
    if (sel_ == 0) changeCat(d);
    else if (sel_ == 2) from_ = (from_ + d + n) % n;
    else if (sel_ == 3) to_ = (to_ + d + n) % n;
  }

  void sendHome() {
    double v;
    if (!value(v)) return;
    double res = cact::convertUnits(v, cat_, from_, to_);
    app.s.in.globals["ans"] = cact::Value(res);
    calcInsert(app, cact::serializeReal(res));
    app.toast("Result inserted on HOME (also in ans)");
  }
};

}  // namespace

std::unique_ptr<Screen> makeUnitsScreen(App& a) { return std::make_unique<UnitsScreen>(a); }

}  // namespace ui
