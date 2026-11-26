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

// Cactulator - GRAPH app: braille plots, trace, zoom, analysis, table, Y= editor.
#include <functional>
#include <cstdlib>
#include <memory>
#include <cctype>
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "app.hpp"
#include "canvas.hpp"
#include "core/format.hpp"
#include "core/numeric.hpp"
#include "core/parser.hpp"

namespace ui {

using cact::GraphEntry;
using cact::GraphState;
using cact::u8len;
using cact::u8substr;

namespace {

const double kPi = 3.14159265358979323846;

std::vector<std::string> splitComma(const std::string& s) {
  std::vector<std::string> out;
  int depth = 0;
  std::string cur;
  for (char c : s) {
    if (c == '(' || c == '[') ++depth;
    if (c == ')' || c == ']') --depth;
    if (c == ',' && depth == 0) {
      out.push_back(cact::trim(cur));
      cur.clear();
      continue;
    }
    cur += c;
  }
  out.push_back(cact::trim(cur));
  return out;
}

std::string shortNum(double v, double scale) {
  if (std::abs(v) < 1e-9 * std::max(1e-300, scale)) return "0";
  char buf[32];
  std::snprintf(buf, sizeof buf, "%.4g", v);
  std::string s = buf;
  size_t e = s.find('e');
  if (e != std::string::npos) {
    std::string mant = s.substr(0, e), ex = s.substr(e + 1);
    int exp = std::atoi(ex.c_str());
    s = mant + "E" + std::to_string(exp);
  }
  return s;
}

std::string piLabel(double v) {
  long long p, q;
  if (std::abs(v) < 1e-12) return "0";
  if (cact::toFraction(v / kPi, p, q, 4, 1e-9)) {
    std::string num = p == 1 ? "π" : p == -1 ? "-π" : std::to_string(p) + "π";
    return q == 1 ? num : num + "/" + std::to_string(q);
  }
  return shortNum(v, 1);
}

class GraphScreen : public Screen {
 public:
  explicit GraphScreen(App& a) : Screen(a) {}

  std::string title() const override {
    switch (mode_) {
      case Mode::Edit: return "Y= FUNCTIONS";
      case Mode::Window: return "WINDOW";
      case Mode::Table: return "TABLE";
      default: return "GRAPH";
    }
  }

  void enter(const std::string& arg) override {
    message_.clear();
    markers_.clear();
    shade_.on = false;
    menu_ = Menu::Main;
    app.s.syncGraphFromFunctions();
    if (arg == "edit" || !anyEnabled()) {
      mode_ = Mode::Edit;
      editing_ = false;
      if (!anyEnabled()) message_ = "Type a function of x (like x^2 - 3) and press Enter, then F1 to graph it.";
    } else {
      mode_ = Mode::View;
    }
  }

  std::vector<SoftKey> softKeys() override {
    if (mode_ == Mode::Edit) {
      return {
          {"Graph", [this] { finishEdit(); }},
          {"Type", [this] { cycleType(); }},
          {"On/Off", [this] { toggleSel(); }},
          {"Clear", [this] { clearSel(); }},
          {"Window", [this] { mode_ = Mode::Window; winSel_ = 0; }},
          {"", nullptr},
          {"", nullptr},
          {"Home", [this] { app.back(); }},
      };
    }
    if (mode_ == Mode::Window) {
      return {
          {"Graph", [this] { mode_ = Mode::View; }},
          {"Auto", [this] { autoFit(); mode_ = Mode::View; }},
          {"Std", [this] { zoomStd(); }},
          {"Trig", [this] { zoomTrig(); }},
          {"Square", [this] { zoomSquare(); }},
          {"Grid", [this] { g().grid = !g().grid; }, g().grid},
          {"", nullptr},
          {"Home", [this] { app.back(); }},
      };
    }
    if (mode_ == Mode::Table) {
      return {
          {"Graph", [this] { mode_ = Mode::View; }},
          {"Start", [this] { askNumber("Table starts at x =", tableStart_); }},
          {"Step", [this] { askNumber("Table step Δx =", tableStep_); if (tableStep_ == 0) tableStep_ = 1; }},
          {"Y=", [this] { mode_ = Mode::Edit; }},
          {"", nullptr},
          {"", nullptr},
          {"", nullptr},
          {"Home", [this] { app.back(); }},
      };
    }
    if (menu_ == Menu::Zoom) {
      return {
          {"In", [this] { zoom(0.5); }},
          {"Out", [this] { zoom(2.0); }},
          {"Std", [this] { zoomStd(); }},
          {"Trig", [this] { zoomTrig(); }},
          {"Fit", [this] { autoFit(); }},
          {"Square", [this] { zoomSquare(); }},
          {"", nullptr},
          {"Back", [this] { menu_ = Menu::Main; }},
      };
    }
    if (menu_ == Menu::Analyze) {
      return {
          {"Root", [this] { analyzeRoot(); }},
          {"Min", [this] { analyzeExtremum(false); }},
          {"Max", [this] { analyzeExtremum(true); }},
          {"Isect", [this] { analyzeIntersect(); }},
          {"dy/dx", [this] { analyzeDerivative(); }},
          {"∫dx", [this] { analyzeIntegral(); }},
          {"Value", [this] { analyzeValue(); }},
          {"Back", [this] { menu_ = Menu::Main; }},
      };
    }
    return {
        {"Y=", [this] { mode_ = Mode::Edit; editing_ = false; }},
        {"Zoom", [this] { menu_ = Menu::Zoom; }},
        {"Trace", [this] { toggleTrace(); }, mode_ == Mode::Trace},
        {"Analyze", [this] { menu_ = Menu::Analyze; if (mode_ != Mode::Trace) startTrace(); }},
        {"Table", [this] { openTable(); }},
        {"Window", [this] { mode_ = Mode::Window; winSel_ = 0; }},
        {"Grid", [this] { g().grid = !g().grid; }, g().grid},
        {"Home", [this] { app.back(); }},
    };
  }

  void draw(const Rect& r) override {
    rect_ = r;
    switch (mode_) {
      case Mode::Edit: drawEdit(r); break;
      case Mode::Window: drawWindow(r); break;
      case Mode::Table: drawTable(r); break;
      default: drawGraph(r);
    }
  }

  bool key(const Event& e) override {
    switch (mode_) {
      case Mode::Edit: return keyEdit(e);
      case Mode::Window: return keyWindow(e);
      case Mode::Table: return keyTable(e);
      default: return keyGraph(e);
    }
  }

  void insert(const std::string& text) override {
    if (mode_ == Mode::Edit) {
      if (!editing_) beginEdit(false);
      lineEdit_.insert(text);
      return;
    }
    if (mode_ == Mode::Window) {
      if (!winEditing_) {
        winEditing_ = true;
        winEdit_.clear();
      }
      winEdit_.insert(text);
      return;
    }
    if (mode_ == Mode::Trace || mode_ == Mode::View) {
      if (text.size() == 1 && (std::isdigit((unsigned char)text[0]) || text == "-" || text == ".")) {
        gotoX(text);
      } else if (text == "+") {
        zoom(0.5);
      } else if (text == "-") {
        zoom(2.0);
      }
    }
  }

  bool mouse(const Event& e, const Rect&) override {
    if (mode_ != Mode::View && mode_ != Mode::Trace) {
      if (mode_ == Mode::Edit && e.mouse == Event::Mouse::Press) {
        int row = e.my - (rect_.y + 1);
        if (row >= 0 && row < GraphState::kSlots) {
          commitEdit();
          sel_ = row;
          if (e.mx < rect_.x + 4) toggleSel();
          else beginEdit(false);
        }
        return true;
      }
      return false;
    }
    if (!plot_.contains(e.mx, e.my)) return false;
    double fx = g().xmin + (e.mx - plot_.x + 0.5) / plot_.w * (g().xmax - g().xmin);
    double fy = g().ymax - (e.my - plot_.y + 0.5) / plot_.h * (g().ymax - g().ymin);
    if (e.mouse == Event::Mouse::WheelUp) zoomAt(fx, fy, 0.7);
    else if (e.mouse == Event::Mouse::WheelDown) zoomAt(fx, fy, 1.0 / 0.7);
    else if (e.mouse == Event::Mouse::Press) {
      // Trace the function closest to the click.
      int best = -1;
      double bestD = 1e300;
      for (int k = 0; k < GraphState::kSlots; ++k) {
        const auto& en = g().entries[k];
        if (!en.enabled || en.text.empty() || en.type != GraphEntry::Type::Func) continue;
        double y = evalFunc(k, fx);
        if (std::isfinite(y) && std::abs(y - fy) < bestD) {
          bestD = std::abs(y - fy);
          best = k;
        }
      }
      if (best >= 0) {
        mode_ = Mode::Trace;
        traceIdx_ = best;
        traceX_ = fx;
      }
    }
    return true;
  }

 private:
  enum class Mode { View, Trace, Edit, Window, Table };
  enum class Menu { Main, Zoom, Analyze };
  Mode mode_ = Mode::View;
  Menu menu_ = Menu::Main;
  Rect rect_, plot_;
  std::string message_;
  bool messageError_ = false;
  struct Marker {
    double x, y;
  };
  std::vector<Marker> markers_;
  struct Shade {
    bool on = false;
    int idx = 0;
    double a = 0, b = 0;
  } shade_;

  // trace
  int traceIdx_ = 0;
  double traceX_ = 0, traceT_ = 0;
  // Y= editor
  int sel_ = 0;
  bool editing_ = false;
  LineEdit lineEdit_;
  // window form
  int winSel_ = 0;
  bool winEditing_ = false;
  LineEdit winEdit_;
  // table
  double tableStart_ = 0, tableStep_ = 1;

  GraphState& g() { return app.s.graph; }
  Term& term() { return app.term; }
  bool deg() const { return app.s.in.settings.angle == cact::AngleMode::DEG; }

  bool anyEnabled() const {
    for (const auto& e : app.s.graph.entries)
      if (e.enabled && !e.text.empty()) return true;
    return false;
  }

  bool trigWindow() const {
    for (const auto& e : app.s.graph.entries)
      if (e.enabled && e.type == GraphEntry::Type::Func)
        for (const char* f : {"sin", "cos", "tan"})
          if (e.text.find(f) != std::string::npos) return true;
    return false;
  }

  void setMessage(const std::string& m, bool err = false) {
    message_ = m;
    messageError_ = err;
  }

  // ---- evaluation helpers

  cact::NodeP parsed(const std::string& text) {
    try {
      return cact::parseExpression(text);
    } catch (const cact::CalcError&) {
      return nullptr;
    }
  }

  double evalFunc(int idx, double x) {
    const auto& en = g().entries[idx];
    cact::NodeP n = parsed(en.text);
    if (!n) return NAN;
    return app.s.in.realFunction(n, "x")(x);
  }

  double evalT(const cact::NodeP& n, double t) {
    try {
      cact::Value v = app.s.in.evalWith(n, {{"t", cact::Value(t)}, {"θ", cact::Value(t)}});
      if (!v.isNum() || std::abs(v.num.imag()) > 1e-9 * std::max(1.0, std::abs(v.num.real()))) return NAN;
      return v.num.real();
    } catch (const cact::CalcError& e) {
      if (std::string(e.what()) == "Interrupted") throw;
      return NAN;
    }
  }

  bool curvePoint(int idx, double t, double& x, double& y) {
    const auto& en = g().entries[idx];
    if (en.type == GraphEntry::Type::Param) {
      auto parts = splitComma(en.text);
      if (parts.size() != 2) return false;
      auto nx = parsed(parts[0]), ny = parsed(parts[1]);
      if (!nx || !ny) return false;
      x = evalT(nx, t);
      y = evalT(ny, t);
    } else {
      auto nr = parsed(en.text);
      if (!nr) return false;
      double r = evalT(nr, t), th = app.s.in.toRadians(t);
      x = r * std::cos(th);
      y = r * std::sin(th);
    }
    return std::isfinite(x) && std::isfinite(y);
  }

  std::string entryError(int idx) {
    const auto& en = g().entries[idx];
    try {
      if (en.type == GraphEntry::Type::Func) {
        auto n = cact::parseExpression(en.text);
        app.s.in.probe(n, "x", 0.5 * (g().xmin + g().xmax) + 1e-7);
      } else {
        auto parts = en.type == GraphEntry::Type::Param ? splitComma(en.text) : std::vector<std::string>{en.text};
        if (en.type == GraphEntry::Type::Param && parts.size() != 2)
          return "parametric needs two parts: x(t), y(t)";
        for (const auto& p : parts) {
          auto n = cact::parseExpression(p);
          app.s.in.evalWith(n, {{"t", cact::Value(0.5 * (g().tmin + g().tmax))}, {"θ", cact::Value(0.5 * (g().tmin + g().tmax))}});
        }
      }
    } catch (const cact::CalcError& e) {
      std::string m = e.what();
      if (m.find("not defined") != std::string::npos || m.find("Syntax") != std::string::npos ||
          m.find("unknown function") != std::string::npos)
        return m;
    }
    return "";
  }

  // ---- drawing the graph

  struct Ticks {
    double step;
    bool pi;
  };

  Ticks ticks(double range, double target, bool forX) {
    double raw = range / std::max(1.0, target);
    if (forX && trigWindow()) {
      if (deg()) {
        for (double c : {15.0, 30.0, 45.0, 90.0, 180.0, 360.0, 720.0})
          if (c >= raw) return {c, false};
      } else {
        for (double c : {kPi / 4, kPi / 2, kPi, 2 * kPi, 4 * kPi})
          if (c >= raw) return {c, true};
      }
    }
    double mag = std::pow(10.0, std::floor(std::log10(raw)));
    double f = raw / mag;
    double nf = f < 1.5 ? 1 : f < 3 ? 2 : f < 7 ? 5 : 10;
    return {nf * mag, false};
  }

  void drawGraph(const Rect& r) {
    const Theme& t = app.th();
    GraphState& G = g();
    if (r.h < 4 || r.w < 10) return;
    // Legend
    int lx = r.x;
    for (int k = 0; k < GraphState::kSlots; ++k) {
      const auto& en = G.entries[k];
      if (!en.enabled || en.text.empty()) continue;
      std::string label = entryLabel(k) + en.text;
      if (lx + (int)u8len(label) + 2 > r.x + r.w) {
        if (lx < r.x + r.w - 1) term().put(lx, r.y, "…", app.lcdDim());
        break;
      }
      bool sel = mode_ == Mode::Trace && traceIdx_ == k;
      term().put(lx, r.y, app.term.ascii ? "-" : "━", Style(t.plot[k], t.lcdBg, true));
      term().put(lx + 1, r.y, label, Style(t.plot[k], t.lcdBg, sel));
      lx += int(u8len(label)) + 3;
    }
    plot_ = {r.x, r.y + 1, r.w, r.h - 2};
    Canvas cv(plot_.w, plot_.h);
    int PW = cv.pw(), PH = cv.ph();
    double xr = G.xmax - G.xmin, yr = G.ymax - G.ymin;
    auto PX = [&](double x) { return (x - G.xmin) / xr * (PW - 1); };
    auto PY = [&](double y) { return (G.ymax - y) / yr * (PH - 1); };

    // Grid and axes
    Ticks tx = ticks(xr, plot_.w / 9.0, true), ty = ticks(yr, plot_.h / 3.0, false);
    double x0 = std::ceil(G.xmin / tx.step) * tx.step;
    double y0 = std::ceil(G.ymin / ty.step) * ty.step;
    if (G.grid)
      for (double x = x0; x <= G.xmax; x += tx.step)
        for (double y = y0; y <= G.ymax; y += ty.step) cv.set(int(std::lround(PX(x))), int(std::lround(PY(y))), t.grid);
    bool xAxis = G.ymin <= 0 && G.ymax >= 0, yAxis = G.xmin <= 0 && G.xmax >= 0;
    int axY = xAxis ? int(std::lround(PY(0))) : -1, axX = yAxis ? int(std::lround(PX(0))) : -1;
    if (xAxis)
      for (int px = 0; px < PW; ++px) cv.set(px, axY, t.axis);
    if (yAxis)
      for (int py = 0; py < PH; ++py) cv.set(axX, py, t.axis);
    // Ticks on the axes
    for (double x = x0; x <= G.xmax + 1e-12 * xr; x += tx.step) {
      int px = int(std::lround(PX(x)));
      int base = xAxis ? axY : PH - 1;
      cv.set(px, base - 1, t.axis);
      cv.set(px, base + 1, t.axis);
    }
    for (double y = y0; y <= G.ymax + 1e-12 * yr; y += ty.step) {
      int py = int(std::lround(PY(y)));
      int base = yAxis ? axX : 0;
      cv.set(base - 1, py, t.axis);
      cv.set(base + 1, py, t.axis);
    }

    // Shaded integral
    if (shade_.on) {
      int k = shade_.idx;
      for (int px = 0; px < PW; px += 2) {
        double x = G.xmin + px * xr / (PW - 1);
        if (x < std::min(shade_.a, shade_.b) || x > std::max(shade_.a, shade_.b)) continue;
        double y = evalFunc(k, x);
        if (!std::isfinite(y)) continue;
        double p1 = PY(0), p2 = PY(y);
        if (p1 > p2) std::swap(p1, p2);
        for (int py = std::max(0, int(p1)); py <= std::min(PH - 1, int(p2)); py += 2) cv.set(px, py, t.plot[k]);
      }
    }

    // Curves
    for (int k = 0; k < GraphState::kSlots; ++k) {
      const auto& en = G.entries[k];
      if (!en.enabled || en.text.empty()) continue;
      int col = t.plot[k];
      if (en.type == GraphEntry::Type::Func) {
        auto n = parsed(en.text);
        if (!n) continue;
        auto f = app.s.in.realFunction(n, "x");
        int N = PW * 2;
        double px0 = NAN, py0 = NAN;
        for (int i = 0; i < N; ++i) {
          double x = G.xmin + xr * i / (N - 1);
          double y = f(x);
          double px = PX(x), py = std::isfinite(y) ? PY(y) : NAN;
          if (std::isfinite(py) && std::isfinite(py0)) {
            bool jump = (py0 < -PH && py > 2 * PH) || (py < -PH && py0 > 2 * PH);
            if (!jump) cv.line(px0, py0, px, py, col);
          } else if (std::isfinite(py)) {
            cv.set(int(std::lround(px)), int(std::lround(py)), col);
          }
          px0 = px;
          py0 = py;
        }
      } else {
        double tmin = G.tmin, tmax = G.tmax;
        int N = G.tstep > 0 ? int(std::min(20000.0, (tmax - tmin) / G.tstep)) : std::max(400, PW * 3);
        double pxPrev = NAN, pyPrev = NAN;
        for (int i = 0; i <= N; ++i) {
          double tt = tmin + (tmax - tmin) * i / N;
          double x, y;
          if (!curvePoint(k, tt, x, y)) {
            pxPrev = NAN;
            continue;
          }
          double px = PX(x), py = PY(y);
          if (std::isfinite(pxPrev) && std::abs(px - pxPrev) + std::abs(py - pyPrev) < 3 * (PW + PH))
            cv.line(pxPrev, pyPrev, px, py, col);
          pxPrev = px;
          pyPrev = py;
        }
      }
    }

    // Axis labels (text overlay)
    {
      int labelRow = xAxis ? std::min(plot_.h - 1, axY / 4 + 1) : plot_.h - 1;
      int lastEnd = -2;
      for (double x = x0; x <= G.xmax + 1e-12 * xr; x += tx.step) {
        if (std::abs(x) < 1e-12 * xr && yAxis) continue;
        std::string lab = tx.pi ? piLabel(x) : shortNum(x, xr);
        int col = int(std::lround(PX(x))) / 2 - int(u8len(lab)) / 2;
        col = std::max(0, std::min(plot_.w - int(u8len(lab)), col));
        if (col <= lastEnd + 1) continue;
        cv.text(col, labelRow, lab, t.axis);
        lastEnd = col + int(u8len(lab));
      }
      int lastRow = -2;
      for (double y = y0; y <= G.ymax + 1e-12 * yr; y += ty.step) {
        if (std::abs(y) < 1e-12 * yr && xAxis) continue;
        std::string lab = shortNum(y, yr);
        int row = int(std::lround(PY(y))) / 4;
        if (row == lastRow || row < 0 || row >= plot_.h) continue;
        int col = yAxis ? axX / 2 + 1 : 0;
        if (col + (int)u8len(lab) > plot_.w) col = plot_.w - int(u8len(lab));
        cv.text(col, row, lab, t.axis);
        lastRow = row;
      }
    }

    // Markers (analysis results) and trace cursor
    for (const auto& m : markers_)
      cv.marker(int(std::lround(PX(m.x))), int(std::lround(PY(m.y))), app.term.ascii ? "o" : "●", t.cursor);
    std::string info;
    if (mode_ == Mode::Trace) info = drawTrace(cv, PX, PY);
    cv.render(term(), plot_.x, plot_.y, t.lcdBg);

    // Info line
    int iy = r.y + r.h - 1;
    std::string line = info;
    Style st = app.lcdDim();
    if (!message_.empty()) {
      line = message_;
      st = messageError_ ? app.lcdError() : app.lcdAccent();
    } else if (line.empty()) {
      std::string err;
      for (int k = 0; k < GraphState::kSlots && err.empty(); ++k)
        if (G.entries[k].enabled && !G.entries[k].text.empty()) {
          std::string e = entryError(k);
          if (!e.empty()) err = entryName(k) + ": " + e;
        }
      if (!err.empty()) {
        line = err;
        st = app.lcdError();
      } else {
        line = "x " + shortNum(G.xmin, xr) + "…" + shortNum(G.xmax, xr) + "   y " + shortNum(G.ymin, yr) + "…" +
               shortNum(G.ymax, yr) + "   ←↑→↓ pan · +/− zoom · click or F3 to trace";
      }
    } else {
      st = Style(t.plot[traceIdx_], t.lcdBg, true);
    }
    if ((int)u8len(line) > r.w) line = u8substr(line, 0, size_t(r.w - 1)) + "…";
    term().put(r.x, iy, line, st);
  }

  template <class FX, class FY>
  std::string drawTrace(Canvas& cv, FX PX, FY PY) {
    const Theme& t = app.th();
    if (!validTrace()) return "";
    const auto& en = g().entries[traceIdx_];
    double x, y;
    std::string info;
    if (en.type == GraphEntry::Type::Func) {
      x = traceX_;
      y = evalFunc(traceIdx_, x);
      info = entryName(traceIdx_) + "   x = " + fmt(x) + "   y = " + (std::isfinite(y) ? fmt(y) : "undefined");
    } else {
      if (!curvePoint(traceIdx_, traceT_, x, y)) {
        x = y = NAN;
      }
      info = entryName(traceIdx_) + "   " + (deg() && en.type == GraphEntry::Type::Polar ? "θ" : "t") + " = " +
             fmt(traceT_) + "   x = " + fmt(x) + "   y = " + fmt(y);
    }
    if (std::isfinite(x) && std::isfinite(y)) {
      int px = int(std::lround(PX(x))), py = int(std::lround(PY(y)));
      // Dotted crosshair
      for (int d = 3; d < 12; d += 2) {
        cv.set(px - d, py, t.cursor);
        cv.set(px + d, py, t.cursor);
        cv.set(px, py - d, t.cursor);
        cv.set(px, py + d, t.cursor);
      }
      cv.marker(px, py, app.term.ascii ? "+" : "◆", t.cursor);
    }
    return info;
  }

  std::string fmt(double v) {
    cact::DisplaySettings d = app.s.in.settings.display;
    d.base = 10;
    d.frac = false;
    if (d.fmt == cact::DisplaySettings::Fmt::STD) {
      char buf[32];
      std::snprintf(buf, sizeof buf, "%.8g", v);
      std::string s = buf;
      size_t e = s.find('e');
      if (e != std::string::npos) s = s.substr(0, e) + "E" + std::to_string(std::atoi(s.c_str() + e + 1));
      if (std::abs(v) < 1e-12) s = "0";
      return s;
    }
    return cact::formatReal(v, d);
  }

  std::string entryName(int k) const {
    const auto& en = app.s.graph.entries[k];
    std::string n = std::to_string(k + 1);
    switch (en.type) {
      case GraphEntry::Type::Func: return "y" + n;
      case GraphEntry::Type::Param: return "xy" + n;
      default: return "r" + n;
    }
  }

  std::string entryLabel(int k) const {
    const auto& en = app.s.graph.entries[k];
    std::string n = std::to_string(k + 1);
    switch (en.type) {
      case GraphEntry::Type::Func: return "y" + n + "=";
      case GraphEntry::Type::Param: return "(x,y)" + n + "=";
      default: return "r" + n + "=";
    }
  }

  // ---- view / trace keys

  bool validTrace() {
    if (traceIdx_ < 0 || traceIdx_ >= GraphState::kSlots) return false;
    const auto& en = g().entries[traceIdx_];
    return en.enabled && !en.text.empty();
  }

  void startTrace() {
    for (int k = 0; k < GraphState::kSlots; ++k) {
      const auto& en = g().entries[k];
      if (en.enabled && !en.text.empty()) {
        traceIdx_ = k;
        break;
      }
    }
    traceX_ = 0.5 * (g().xmin + g().xmax);
    if (g().xmin <= 0 && g().xmax >= 0) traceX_ = 0;
    traceT_ = g().tmin;
    if (validTrace()) mode_ = Mode::Trace;
  }

  void toggleTrace() {
    if (mode_ == Mode::Trace) mode_ = Mode::View;
    else startTrace();
    message_.clear();
  }

  void nextTrace(int dir) {
    for (int step = 1; step <= GraphState::kSlots; ++step) {
      int k = ((traceIdx_ + dir * step) % GraphState::kSlots + GraphState::kSlots) % GraphState::kSlots;
      const auto& en = g().entries[k];
      if (en.enabled && !en.text.empty()) {
        traceIdx_ = k;
        return;
      }
    }
  }

  void moveTrace(int pixels) {
    const auto& en = g().entries[traceIdx_];
    if (en.type == GraphEntry::Type::Func) {
      double dx = (g().xmax - g().xmin) / std::max(1, plot_.w * 2 - 1);
      traceX_ += pixels * dx;
      // Snap to "nice" values when very close (e.g. 0, 90, 1.5).
      double nice = std::round(traceX_ / dx) * dx;
      traceX_ = nice;
      if (traceX_ < g().xmin) pan(traceX_ - g().xmin, 0);
      if (traceX_ > g().xmax) pan(traceX_ - g().xmax, 0);
    } else {
      double dt = g().tstep > 0 ? g().tstep : (g().tmax - g().tmin) / 200;
      traceT_ = std::clamp(traceT_ + pixels * dt, g().tmin, g().tmax);
    }
    message_.clear();
  }

  void pan(double dx, double dy) {
    g().xmin += dx;
    g().xmax += dx;
    g().ymin += dy;
    g().ymax += dy;
    g().userWindow = true;
  }

  void zoom(double f) {
    double cx = 0.5 * (g().xmin + g().xmax), cy = 0.5 * (g().ymin + g().ymax);
    if (mode_ == Mode::Trace && validTrace() && g().entries[traceIdx_].type == GraphEntry::Type::Func) {
      cx = traceX_;
      double y = evalFunc(traceIdx_, traceX_);
      if (std::isfinite(y)) cy = y;
    }
    zoomAt(cx, cy, f);
  }

  void zoomAt(double cx, double cy, double f) {
    GraphState& G = g();
    double nx0 = cx - (cx - G.xmin) * f, nx1 = cx + (G.xmax - cx) * f;
    double ny0 = cy - (cy - G.ymin) * f, ny1 = cy + (G.ymax - cy) * f;
    if (nx1 - nx0 < 1e-12 || ny1 - ny0 < 1e-12 || nx1 - nx0 > 1e15) return;
    G.xmin = nx0;
    G.xmax = nx1;
    G.ymin = ny0;
    G.ymax = ny1;
    G.userWindow = true;
  }

  void zoomStd() {
    g().xmin = g().ymin = -10;
    g().xmax = g().ymax = 10;
    menu_ = Menu::Main;
  }

  void zoomTrig() {
    g().xmin = deg() ? -360 : -2 * kPi;
    g().xmax = deg() ? 360 : 2 * kPi;
    g().ymin = -4;
    g().ymax = 4;
    menu_ = Menu::Main;
  }

  void zoomSquare() {
    if (plot_.w <= 0 || plot_.h <= 0) return;
    double xr = g().xmax - g().xmin;
    double yr = xr * (plot_.h * 4.0) / (plot_.w * 2.0);
    double cy = 0.5 * (g().ymin + g().ymax);
    g().ymin = cy - yr / 2;
    g().ymax = cy + yr / 2;
    menu_ = Menu::Main;
  }

  void autoFit() {
    double xmin = g().xmin, xmax = g().xmax;
    app.s.autoWindow(true);
    bool onlyFuncs = true;
    for (const auto& en : g().entries)
      if (en.enabled && !en.text.empty() && en.type != GraphEntry::Type::Func) onlyFuncs = false;
    if (onlyFuncs && g().userWindow == false) {
      // Keep the user's x range, fit y to it.
      g().xmin = xmin;
      g().xmax = xmax;
      std::vector<double> ys;
      for (int k = 0; k < GraphState::kSlots; ++k) {
        const auto& en = g().entries[k];
        if (!en.enabled || en.text.empty()) continue;
        for (int i = 0; i <= 300; ++i) {
          double y = evalFunc(k, xmin + (xmax - xmin) * i / 300);
          if (std::isfinite(y)) ys.push_back(y);
        }
      }
      if (!ys.empty()) {
        std::sort(ys.begin(), ys.end());
        double lo = ys[size_t(ys.size() * 0.02)], hi = ys[std::min(ys.size() - 1, size_t(ys.size() * 0.98))];
        if (ys.front() >= lo - 0.25 * (hi - lo)) lo = ys.front();
        if (ys.back() <= hi + 0.25 * (hi - lo)) hi = ys.back();
        double pad = (hi - lo) * 0.1;
        if (hi - lo < 1e-9) pad = std::max(1.0, std::abs(lo) * 0.5);
        g().ymin = lo - pad;
        g().ymax = hi + pad;
      }
    }
    menu_ = Menu::Main;
  }

  void gotoX(const std::string& initial) {
    if (mode_ != Mode::Trace) startTrace();
    if (!validTrace()) return;
    bool func = g().entries[traceIdx_].type == GraphEntry::Type::Func;
    std::string v = initial;
    if (!app.ask(func ? "Move the cursor to x =" : "Move the cursor to t =", v)) return;
    try {
      double x = app.s.in.evalString(v).real();
      if (func) {
        traceX_ = x;
        if (x < g().xmin || x > g().xmax) {
          double w = g().xmax - g().xmin;
          g().xmin = x - w / 2;
          g().xmax = x + w / 2;
        }
      } else {
        traceT_ = x;
      }
    } catch (const cact::CalcError& e) {
      setMessage(e.what(), true);
    }
  }

  bool keyGraph(const Event& e) {
    if (e.type == Event::Type::Text) {
      const std::string& c = e.text;
      if (c == "+" || c == "=") zoom(0.5);
      else if (c == "-" || c == "_") zoom(2.0);
      else if (c == "t" || c == "T") toggleTrace();
      else if (c == "g") g().grid = !g().grid;
      else if (c == "a") autoFit();
      else if (c == "z") zoomStd();
      else if (c == "r") analyzeRoot();
      else if (c == "m") analyzeExtremum(false);
      else if (c == "M") analyzeExtremum(true);
      else if (c == "i") analyzeIntersect();
      else if (c == "d") analyzeDerivative();
      else if (std::isdigit((unsigned char)c[0]) || c == ".") gotoX(c);
      else return false;
      return true;
    }
    if (e.type != Event::Type::Key) return false;
    int k = e.key;
    double xr = g().xmax - g().xmin, yr = g().ymax - g().ymin;
    if (k == keys::Esc) {
      if (menu_ != Menu::Main) {
        menu_ = Menu::Main;
        return true;
      }
      if (mode_ == Mode::Trace) {
        mode_ = Mode::View;
        return true;
      }
      return false;
    }
    if (k == keys::Enter && mode_ == Mode::Trace && validTrace()) {
      // Send the traced point to the calculator as ans.
      double x = traceX_, y = evalFunc(traceIdx_, x);
      if (g().entries[traceIdx_].type == GraphEntry::Type::Func && std::isfinite(y)) {
        app.s.in.globals["ans"] = cact::Value(y);
        app.s.in.globals["x"] = cact::Value(x);
        setMessage("Stored x = " + fmt(x) + " and ans = " + fmt(y));
      }
      return true;
    }
    if (mode_ == Mode::Trace) {
      if (k == keys::Left) moveTrace(-1);
      else if (k == keys::Right) moveTrace(1);
      else if (k == keys::SLeft || k == keys::CtrlLeft) moveTrace(-10);
      else if (k == keys::SRight || k == keys::CtrlRight) moveTrace(10);
      else if (k == keys::Up) nextTrace(-1);
      else if (k == keys::Down) nextTrace(1);
      else return false;
      return true;
    }
    if (k == keys::Left) pan(-xr / 8, 0);
    else if (k == keys::Right) pan(xr / 8, 0);
    else if (k == keys::Up) pan(0, yr / 8);
    else if (k == keys::Down) pan(0, -yr / 8);
    else if (k == keys::Enter) startTrace();
    else return false;
    return true;
  }

  // ---- analysis

  bool needFunc(int& idx) {
    if (mode_ != Mode::Trace) startTrace();
    if (!validTrace()) {
      setMessage("Nothing to analyze — add a function with Y=", true);
      return false;
    }
    if (g().entries[traceIdx_].type != GraphEntry::Type::Func) {
      setMessage("Analysis works on y = f(x) functions; use ↑/↓ to pick one", true);
      return false;
    }
    idx = traceIdx_;
    return true;
  }

  void analyzeRoot() {
    int idx;
    if (!needFunc(idx)) return;
    auto n = parsed(g().entries[idx].text);
    auto f = app.s.in.realFunction(n, "x");
    double r;
    if (!rootNear(f, r)) {
      setMessage("No root (zero crossing) visible — zoom out or pan", true);
      return;
    }
    traceX_ = r;
    markers_ = {{r, 0}};
    app.s.in.globals["ans"] = cact::Value(r);
    setMessage("Root of " + entryName(idx) + ":  x = " + fmt(r) + "   (stored in ans)");
    menu_ = Menu::Main;
  }

  bool rootNear(const std::function<double(double)>& f, double& out) {
    GraphState& G = g();
    const int N = 400;
    double best = 1e300;
    bool found = false;
    double prevX = G.xmin, prevY = f(prevX);
    for (int i = 1; i <= N; ++i) {
      double x = G.xmin + (G.xmax - G.xmin) * i / N, y = f(x);
      if (std::isfinite(prevY) && std::isfinite(y) && ((prevY <= 0) != (y <= 0) || y == 0)) {
        try {
          double r = y == 0 ? x : cact::brent(f, prevX, x);
          double fr = f(r);
          bool pole = !std::isfinite(fr) || std::abs(fr) > 1e-6 * std::max({1.0, std::abs(prevY), std::abs(y)});
          if (!pole && std::abs(r - traceX_) < best) {
            best = std::abs(r - traceX_);
            out = r;
            found = true;
          }
        } catch (const cact::CalcError&) {
        }
      }
      prevX = x;
      prevY = y;
    }
    if (found) {
      double rr = std::round(out);
      if (std::abs(out - rr) < 1e-10 * std::max(1.0, std::abs(rr))) out = rr;
    }
    return found;
  }

  void analyzeExtremum(bool max) {
    int idx;
    if (!needFunc(idx)) return;
    auto n = parsed(g().entries[idx].text);
    auto f = app.s.in.realFunction(n, "x");
    GraphState& G = g();
    const int N = 400;
    std::vector<double> xs(N + 1), ys(N + 1);
    for (int i = 0; i <= N; ++i) {
      xs[i] = G.xmin + (G.xmax - G.xmin) * i / N;
      double y = f(xs[i]);
      ys[i] = std::isfinite(y) ? (max ? -y : y) : NAN;
    }
    int bestI = -1;
    double bestD = 1e300;
    for (int i = 1; i < N; ++i) {
      if (!std::isfinite(ys[i - 1]) || !std::isfinite(ys[i]) || !std::isfinite(ys[i + 1])) continue;
      if (ys[i] <= ys[i - 1] && ys[i] <= ys[i + 1] && !(ys[i] == ys[i - 1] && ys[i] == ys[i + 1])) {
        double d = std::abs(xs[i] - traceX_);
        if (d < bestD) {
          bestD = d;
          bestI = i;
        }
      }
    }
    if (bestI < 0) {
      setMessage(std::string("No local ") + (max ? "maximum" : "minimum") + " visible — try zooming out", true);
      return;
    }
    auto g2 = [&](double x) {
      double v = f(x);
      return std::isfinite(v) ? (max ? -v : v) : 1e300;
    };
    double x = cact::snapMinimum(g2, cact::minimize(g2, xs[bestI - 1], xs[bestI + 1]));
    double y = f(x);
    traceX_ = x;
    markers_ = {{x, y}};
    app.s.in.globals["ans"] = cact::Value(y);
    setMessage(std::string(max ? "Maximum" : "Minimum") + " of " + entryName(idx) + ":  x = " + fmt(x) + "   y = " + fmt(y));
    menu_ = Menu::Main;
  }

  void analyzeIntersect() {
    int idx;
    if (!needFunc(idx)) return;
    int other = -1;
    for (int step = 1; step < GraphState::kSlots; ++step) {
      int k = (idx + step) % GraphState::kSlots;
      const auto& en = g().entries[k];
      if (en.enabled && !en.text.empty() && en.type == GraphEntry::Type::Func) {
        other = k;
        break;
      }
    }
    if (other < 0) {
      setMessage("Intersect needs two functions turned on (see Y=)", true);
      return;
    }
    auto n1 = parsed(g().entries[idx].text), n2 = parsed(g().entries[other].text);
    auto f1 = app.s.in.realFunction(n1, "x"), f2 = app.s.in.realFunction(n2, "x");
    std::function<double(double)> d = [&](double x) { return f1(x) - f2(x); };
    double r;
    if (!rootNear(d, r)) {
      setMessage("No intersection of " + entryName(idx) + " and " + entryName(other) + " visible", true);
      return;
    }
    double y = f1(r);
    traceX_ = r;
    markers_ = {{r, y}};
    app.s.in.globals["ans"] = cact::Value(r);
    setMessage(entryName(idx) + " ∩ " + entryName(other) + ":  x = " + fmt(r) + "   y = " + fmt(y));
    menu_ = Menu::Main;
  }

  void analyzeDerivative() {
    int idx;
    if (!needFunc(idx)) return;
    auto n = parsed(g().entries[idx].text);
    auto f = app.s.in.realFunction(n, "x");
    try {
      double d = cact::derivative(f, traceX_);
      app.s.in.globals["ans"] = cact::Value(d);
      setMessage("dy/dx of " + entryName(idx) + " at x = " + fmt(traceX_) + " is " + fmt(d));
    } catch (const cact::CalcError& e) {
      setMessage(e.what(), true);
    }
    menu_ = Menu::Main;
  }

  void analyzeIntegral() {
    int idx;
    if (!needFunc(idx)) return;
    std::string a = fmt(g().xmin < 0 && g().xmax > 0 ? 0.0 : g().xmin), b = fmt(traceX_);
    if (!app.ask("∫ " + entryName(idx) + " dx — lower limit a =", a)) return;
    if (!app.ask("∫ " + entryName(idx) + " dx — upper limit b =", b)) return;
    try {
      double lo = app.s.in.evalString(a).real(), hi = app.s.in.evalString(b).real();
      auto n = parsed(g().entries[idx].text);
      double v = cact::integrate(app.s.in.realFunction(n, "x"), lo, hi);
      shade_ = {true, idx, lo, hi};
      app.s.in.globals["ans"] = cact::Value(v);
      setMessage("∫ " + entryName(idx) + " dx from " + fmt(lo) + " to " + fmt(hi) + " = " + fmt(v));
    } catch (const cact::CalcError& e) {
      setMessage(e.what(), true);
    }
    menu_ = Menu::Main;
  }

  void analyzeValue() {
    int idx;
    if (!needFunc(idx)) return;
    gotoX(fmt(traceX_));
    double y = evalFunc(idx, traceX_);
    markers_ = {{traceX_, y}};
    setMessage(entryName(idx) + "(" + fmt(traceX_) + ") = " + (std::isfinite(y) ? fmt(y) : "undefined"));
    menu_ = Menu::Main;
  }

  // ---- Y= editor

  void beginEdit(bool replace) {
    editing_ = true;
    if (replace) lineEdit_.clear();
    else lineEdit_.set(g().entries[sel_].text);
  }

  void commitEdit() {
    if (!editing_) return;
    std::string text = cact::trim(lineEdit_.text);
    for (const char* prefix : {"y=", "y =", "r=", "r ="})
      if (text.rfind(prefix, 0) == 0) text = cact::trim(text.substr(std::string(prefix).size()));
    auto& en = g().entries[sel_];
    try {
      if (!text.empty()) {
        if (en.type == GraphEntry::Type::Param) {
          auto parts = splitComma(text);
          if (parts.size() != 2) throw cact::CalcError("write both parts separated by a comma: cos(t), sin(t)");
          for (auto& p : parts) cact::parseExpression(p);
        } else {
          cact::parseExpression(text);
        }
      }
      app.s.setGraphEntry(sel_, en.type, text, true);
      editing_ = false;
      message_.clear();
      if (!g().userWindow) app.s.autoWindow(true);
    } catch (const cact::CalcError& e) {
      setMessage(e.what(), true);
    }
  }

  void finishEdit() {
    commitEdit();
    if (editing_) return;
    if (!anyEnabled()) {
      setMessage("Type a function first, e.g. x^2 - 4", true);
      return;
    }
    mode_ = Mode::View;
    message_.clear();
  }

  void cycleType() {
    auto& en = g().entries[sel_];
    auto next = en.type == GraphEntry::Type::Func    ? GraphEntry::Type::Param
                : en.type == GraphEntry::Type::Param ? GraphEntry::Type::Polar
                                                     : GraphEntry::Type::Func;
    std::string sample = next == GraphEntry::Type::Param ? "cos(3t), sin(2t)"
                         : next == GraphEntry::Type::Polar ? "1 + cos(t)"
                                                           : "x^2";
    app.s.setGraphEntry(sel_, next, sample, true);
    editing_ = false;
    if (next != GraphEntry::Type::Func) {
      g().tmin = 0;
      g().tmax = deg() ? 360 : 2 * kPi;
    }
    app.s.autoWindow(true);
    setMessage(next == GraphEntry::Type::Func    ? "Function y = f(x)"
               : next == GraphEntry::Type::Param ? "Parametric: x(t), y(t) — separate with a comma"
                                                 : "Polar: r as a function of the angle t (θ)");
  }

  void toggleSel() {
    auto& en = g().entries[sel_];
    en.enabled = !en.enabled;
  }

  void clearSel() {
    app.s.setGraphEntry(sel_, GraphEntry::Type::Func, "", false);
    editing_ = false;
  }

  void drawEdit(const Rect& r) {
    const Theme& t = app.th();
    term().put(r.x, r.y, "Your functions — ↑↓ choose · Enter edit · Space on/off · F2 type", app.lcdDim(), r.w);
    for (int k = 0; k < GraphState::kSlots; ++k) {
      int y = r.y + 1 + k;
      if (y >= r.y + r.h - 1) break;
      const auto& en = g().entries[k];
      bool sel = k == sel_;
      Style base = sel ? app.lcdSel() : app.lcd();
      if (sel) term().fill({r.x, y, r.w, 1}, base);
      std::string box = app.term.ascii ? (en.enabled ? "[x]" : "[ ]") : (en.enabled ? "▣" : "□");
      term().put(r.x, y, box, Style(t.plot[k], base.bg, true));
      int x = r.x + int(u8len(box)) + 1;
      std::string label = entryLabel(k);
      term().put(x, y, label, Style(t.plot[k], base.bg, true));
      x += int(u8len(label)) + 1;
      int w = r.w - (x - r.x);
      if (sel && editing_) {
        drawEditLine(x, y, w, base);
      } else if (en.text.empty()) {
        term().put(x, y, sel ? "type a function…" : "", Style(t.lcdDim, base.bg), w);
      } else {
        drawSpans(term(), x, y, w, en.text, highlight(en.text, t, &app.s.in), base);
      }
    }
    int iy = r.y + r.h - 1;
    if (!message_.empty()) term().put(r.x, iy, message_, messageError_ ? app.lcdError() : app.lcdAccent(), r.w);
    else
      term().put(r.x, iy,
                 g().entries[sel_].type == GraphEntry::Type::Func ? "Use x as the variable: 2x+1, sin(x), x^3-4x"
                 : g().entries[sel_].type == GraphEntry::Type::Param ? "Two expressions in t: cos(3t), sin(2t)"
                                                                      : "r in terms of the angle t: 2sin(4t)",
                 app.lcdDim(), r.w);
  }

  void drawEditLine(int x, int y, int w, const Style& base) {
    ui::drawEdit(term(), x, y, w, lineEdit_, app.th(), base, true,
                 highlight(lineEdit_.text, app.th(), &app.s.in, long(lineEdit_.cur)));
  }

  bool keyEdit(const Event& e) {
    if (editing_) {
      if (e.type == Event::Type::Key) {
        if (e.key == keys::Enter) {
          commitEdit();
          if (!editing_ && sel_ < GraphState::kSlots - 1 && g().entries[sel_ + 1].text.empty()) sel_++;
          return true;
        }
        if (e.key == keys::Esc) {
          editing_ = false;
          return true;
        }
        if (e.key == keys::Up || e.key == keys::Down) {
          commitEdit();
          if (!editing_) sel_ = std::clamp(sel_ + (e.key == keys::Up ? -1 : 1), 0, GraphState::kSlots - 1);
          return true;
        }
      }
      return lineEdit_.handle(e);
    }
    if (e.type == Event::Type::Text) {
      if (e.text == " ") {
        toggleSel();
        return true;
      }
      beginEdit(g().entries[sel_].text.empty());
      lineEdit_.insert(e.text);
      return true;
    }
    if (e.type != Event::Type::Key) return false;
    int k = e.key;
    if (k == keys::Up) sel_ = std::max(0, sel_ - 1);
    else if (k == keys::Down) sel_ = std::min(GraphState::kSlots - 1, sel_ + 1);
    else if (k == keys::Enter) beginEdit(false);
    else if (k == keys::Delete || k == keys::Backspace) clearSel();
    else if (k == keys::Esc) {
      if (anyEnabled()) mode_ = Mode::View;
      else return false;
    } else return false;
    return true;
  }

  // ---- window form

  struct Field {
    const char* name;
    double* value;
  };

  std::vector<Field> fields() {
    GraphState& G = g();
    return {{"x min", &G.xmin}, {"x max", &G.xmax},   {"y min", &G.ymin},
            {"y max", &G.ymax}, {"t (θ) min", &G.tmin}, {"t (θ) max", &G.tmax}, {"t step (0 = auto)", &G.tstep}};
  }

  void drawWindow(const Rect& r) {
    auto f = fields();
    term().put(r.x, r.y, "Graph window — ↑↓ choose · type a value · Enter apply", app.lcdDim(), r.w);
    for (size_t i = 0; i < f.size(); ++i) {
      int y = r.y + 2 + int(i);
      if (y >= r.y + r.h) break;
      bool sel = int(i) == winSel_;
      Style base = sel ? app.lcdSel() : app.lcd();
      if (sel) term().fill({r.x, y, std::min(r.w, 44), 1}, base);
      term().put(r.x + 1, y, f[i].name, base);
      if (sel && winEditing_) {
        ui::drawEdit(term(), r.x + 22, y, std::min(20, r.w - 22), winEdit_, app.th(), base, true,
                     highlight(winEdit_.text, app.th(), &app.s.in, long(winEdit_.cur)));
      } else {
        term().put(r.x + 22, y, fmt(*f[i].value), Style(base.fg, base.bg, true));
      }
    }
    int y = r.y + 3 + int(f.size());
    if (y < r.y + r.h) term().put(r.x + 1, y, std::string("Grid: ") + (g().grid ? "on" : "off") + "   (F6 toggles)", app.lcdDim());
    if (!message_.empty() && r.h > 1) term().put(r.x, r.y + r.h - 1, message_, messageError_ ? app.lcdError() : app.lcdAccent(), r.w);
  }

  bool keyWindow(const Event& e) {
    auto f = fields();
    if (winEditing_) {
      if (e.is(keys::Enter) || e.is(keys::Up) || e.is(keys::Down)) {
        try {
          double v = app.s.in.evalString(winEdit_.text).real();
          *f[size_t(winSel_)].value = v;
          g().userWindow = true;
          message_.clear();
          if (!(g().xmin < g().xmax) || !(g().ymin < g().ymax))
            setMessage("min must be less than max", true);
        } catch (const cact::CalcError& err) {
          setMessage(err.what(), true);
        }
        winEditing_ = false;
        if (e.is(keys::Up)) winSel_ = std::max(0, winSel_ - 1);
        if (e.is(keys::Down) || e.is(keys::Enter)) winSel_ = std::min(int(f.size()) - 1, winSel_ + 1);
        return true;
      }
      if (e.is(keys::Esc)) {
        winEditing_ = false;
        return true;
      }
      return winEdit_.handle(e);
    }
    if (e.type == Event::Type::Text) {
      winEditing_ = true;
      winEdit_.clear();
      winEdit_.insert(e.text);
      return true;
    }
    if (e.is(keys::Up)) winSel_ = std::max(0, winSel_ - 1);
    else if (e.is(keys::Down)) winSel_ = std::min(int(f.size()) - 1, winSel_ + 1);
    else if (e.is(keys::Enter)) {
      winEditing_ = true;
      winEdit_.set(fmt(*f[size_t(winSel_)].value));
    } else if (e.is(keys::Esc)) mode_ = Mode::View;
    else return false;
    return true;
  }

  // ---- table

  void openTable() {
    double xr = g().xmax - g().xmin;
    tableStep_ = ticks(xr, 10, true).step;
    tableStart_ = mode_ == Mode::Trace ? traceX_ : std::ceil(g().xmin / tableStep_) * tableStep_;
    mode_ = Mode::Table;
  }

  void askNumber(const std::string& q, double& v) {
    std::string s = fmt(v);
    if (!app.ask(q, s)) return;
    try {
      v = app.s.in.evalString(s).real();
    } catch (const cact::CalcError& e) {
      setMessage(e.what(), true);
    }
  }

  void drawTable(const Rect& r) {
    const Theme& t = app.th();
    std::vector<int> cols;
    for (int k = 0; k < GraphState::kSlots; ++k) {
      const auto& en = g().entries[k];
      if (en.enabled && !en.text.empty() && en.type == GraphEntry::Type::Func) cols.push_back(k);
    }
    int ncols = int(cols.size()) + 1;
    int cw = std::max(8, std::min(16, r.w / ncols - 1));
    auto cell = [&](double v) {
      if (!std::isfinite(v)) return std::string("—");
      return cact::formatShort(v, cw - 1, app.s.in.settings.display);
    };
    term().put(r.x, r.y, padLeft("x", cw), Style(t.lcdStrong, t.lcdBg, true));
    for (size_t c = 0; c < cols.size(); ++c)
      term().put(r.x + int(c + 1) * (cw + 1), r.y, padLeft(entryName(cols[c]), cw), Style(t.plot[cols[c]], t.lcdBg, true));
    term().hrule(r.x, r.y + 1, std::min(r.w, ncols * (cw + 1)), app.lcdDim(), app.term.ascii ? "-" : "─");
    int rows = r.h - 3;
    for (int i = 0; i < rows; ++i) {
      double x = tableStart_ + i * tableStep_;
      if (std::abs(x) < 1e-12 * std::abs(tableStep_)) x = 0;
      int y = r.y + 2 + i;
      term().put(r.x, y, padLeft(cell(x), cw), app.lcdStrong());
      for (size_t c = 0; c < cols.size(); ++c)
        term().put(r.x + int(c + 1) * (cw + 1), y, padLeft(cell(evalFunc(cols[c], x)), cw), app.lcd());
    }
    term().put(r.x, r.y + r.h - 1, "↑↓ scroll · PgUp/PgDn page · F2 start · F3 step Δx = " + fmt(tableStep_), app.lcdDim(), r.w);
  }

  static std::string padLeft(const std::string& s, int w) { return cact::padLeft(s, size_t(w)); }

  bool keyTable(const Event& e) {
    int page = std::max(1, rect_.h - 4);
    if (e.is(keys::Up)) tableStart_ -= tableStep_;
    else if (e.is(keys::Down)) tableStart_ += tableStep_;
    else if (e.is(keys::PageUp)) tableStart_ -= page * tableStep_;
    else if (e.is(keys::PageDown)) tableStart_ += page * tableStep_;
    else if (e.is(keys::Esc)) mode_ = Mode::View;
    else return false;
    return true;
  }
};

}  // namespace

std::unique_ptr<Screen> makeGraphScreen(App& a) { return std::make_unique<GraphScreen>(a); }

}  // namespace ui
