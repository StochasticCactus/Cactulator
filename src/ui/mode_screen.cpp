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

// Cactulator - MODE app: all settings in one list.
#include <memory>
#include <algorithm>

#include "app.hpp"
#include "core/format.hpp"

namespace ui {

using cact::DisplaySettings;
using cact::u8len;

namespace {

class ModeScreen : public Screen {
 public:
  explicit ModeScreen(App& a) : Screen(a) {}

  std::string title() const override { return "MODE"; }

  std::vector<SoftKey> softKeys() override {
    return {
        {"◂", [this] { change(-1); }},
        {"▸", [this] { change(1); }},
        {"", nullptr},
        {"", nullptr},
        {"", nullptr},
        {"", nullptr},
        {"", nullptr},
        {"Home", [this] { app.back(); }},
    };
  }

  struct Item {
    std::string name, value, help;
  };

  std::vector<Item> items() {
    auto& st = app.s.in.settings;
    auto& d = st.display;
    const char* fmts[] = {"Standard", "Fixed decimals", "Scientific", "Engineering"};
    const char* bases[] = {"Decimal", "Hexadecimal", "Binary", "Octal"};
    int bi = d.base == 16 ? 1 : d.base == 2 ? 2 : d.base == 8 ? 3 : 0;
    std::string keypad = app.s.prefs.count("keypad") ? app.s.prefs["keypad"] : "auto";
    std::string splash = app.s.prefs.count("splash") ? app.s.prefs["splash"] : "on";
    return {
        {"Angle unit", st.angle == cact::AngleMode::DEG ? "Degrees (DEG)" : "Radians (RAD)",
         "Degrees: a full turn is 360. Radians: a full turn is 2π. Shortcut: F9."},
        {"Entry style", app.s.rpnMode ? "RPN (stack)" : "Algebraic",
         "Algebraic: type 3+4 Enter. RPN (classic HP): 3 Enter 4 +. Shortcut: F10."},
        {"Number format", fmts[int(d.fmt)], "How results are written: 12 digits, fixed decimals, or ×10ⁿ notation."},
        {"Decimals", std::to_string(d.digits), "Digits after the point for Fixed, Scientific and Engineering."},
        {"Fractions", d.frac ? "On (0.75 → 3/4)" : "Off", "Show exact fractions and multiples of π when possible."},
        {"Whole numbers in", bases[bi], "Show integer results in another base (for programmers)."},
        {"Theme", app.th().name, "Colors of the calculator. Shortcut: Ctrl-T."},
        {"Keypad", keypad == "auto" ? "Automatic" : keypad == "show" ? "Always" : "Hidden",
         "Automatic shows the clickable keypad when the window is big enough. Ctrl-K toggles."},
        {"Splash screen", splash == "off" ? "Off" : "On", "The cactus welcome screen at start-up."},
    };
  }

  void draw(const Rect& r) override {
    const Theme& t = app.th();
    auto it = items();
    rows_.clear();
    term().put(r.x, r.y, "↑↓ choose · ←→ or Enter change", app.lcdDim(), r.w);
    for (size_t i = 0; i < it.size(); ++i) {
      int y = r.y + 2 + int(i);
      if (y >= r.y + r.h - 3) break;
      bool sel = int(i) == sel_;
      Style st = sel ? app.lcdSel() : app.lcd();
      if (sel) term().fill({r.x, y, std::min(r.w, 52), 1}, st);
      term().put(r.x + 1, y, it[i].name, st);
      std::string v = (app.term.ascii ? "< " : "◂ ") + it[i].value + (app.term.ascii ? " >" : " ▸");
      term().put(r.x + 20, y, v, Style(sel ? st.fg : t.lcdAccent, st.bg, true), r.w - 20);
      rows_.push_back({y, int(i)});
    }
    int y = r.y + r.h - 3;
    term().put(r.x, y, it[size_t(sel_)].help, app.lcdDim(), r.w);
    // Live preview
    cact::Value v(1.0 / 3.0);
    std::string preview = "Preview:  1/3 → " + cact::formatInline(v, app.s.in.settings.display) +
                          "    255 → " + cact::formatInline(cact::Value(255.0), app.s.in.settings.display) +
                          "    12345.678 → " + cact::formatInline(cact::Value(12345.678), app.s.in.settings.display);
    term().put(r.x, y + 2, preview, app.lcdStrong(), r.w);
  }

  bool key(const Event& e) override {
    int n = int(items().size());
    if (e.is(keys::Up)) sel_ = std::max(0, sel_ - 1);
    else if (e.is(keys::Down)) sel_ = std::min(n - 1, sel_ + 1);
    else if (e.is(keys::Left)) change(-1);
    else if (e.is(keys::Right) || e.is(keys::Enter) || e.isChar(' ')) change(1);
    else return false;
    return true;
  }

  bool mouse(const Event& e, const Rect&) override {
    if (e.mouse != Event::Mouse::Press) return false;
    for (const auto& [y, idx] : rows_)
      if (y == e.my) {
        if (idx == sel_) change(1);
        sel_ = idx;
        return true;
      }
    return false;
  }

 private:
  int sel_ = 0;
  std::vector<std::pair<int, int>> rows_;

  Term& term() { return app.term; }

  void change(int d) {
    auto& st = app.s.in.settings;
    auto& ds = st.display;
    switch (sel_) {
      case 0: app.toggleAngle(); break;
      case 1:
        app.s.rpnMode = !app.s.rpnMode;
        break;
      case 2: ds.fmt = DisplaySettings::Fmt((int(ds.fmt) + d + 4) % 4); break;
      case 3: ds.digits = std::clamp(ds.digits + d, 0, 12); break;
      case 4: ds.frac = !ds.frac; break;
      case 5: {
        int order[] = {10, 16, 2, 8};
        int i = 0;
        while (order[i] != ds.base) ++i;
        ds.base = order[(i + d + 4) % 4];
        break;
      }
      case 6: {
        if (d > 0) app.cycleTheme();
        else {
          int n = int(themes().size());
          int cur = themeIndex(app.th().name);
          app.setTheme(themes()[size_t((cur + n - 1) % n)].name);
        }
        break;
      }
      case 7: {
        std::string& k = app.s.prefs["keypad"];
        if (k.empty()) k = "auto";
        const char* order[] = {"auto", "show", "hide"};
        int i = k == "show" ? 1 : k == "hide" ? 2 : 0;
        k = order[(i + d + 3) % 3];
        app.setKeypadMode(k);
        break;
      }
      case 8: {
        std::string& s = app.s.prefs["splash"];
        s = s == "off" ? "on" : "off";
        break;
      }
    }
    app.save();
  }
};

}  // namespace

std::unique_ptr<Screen> makeModeScreen(App& a) { return std::make_unique<ModeScreen>(a); }

}  // namespace ui
