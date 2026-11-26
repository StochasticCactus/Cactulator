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

#include "canvas.hpp"

#include <algorithm>
#include <cmath>

#include "core/format.hpp"

namespace ui {

Canvas::Canvas(int cols, int rows)
    : cols_(std::max(1, cols)),
      rows_(std::max(1, rows)),
      dots_(size_t(cols_) * rows_, 0),
      color_(size_t(cols_) * rows_, -1),
      over_(size_t(cols_) * rows_),
      overColor_(size_t(cols_) * rows_, -1),
      overBold_(size_t(cols_) * rows_, false) {}

void Canvas::set(int px, int py, int color) {
  if (px < 0 || py < 0 || px >= pw() || py >= ph()) return;
  static const uint8_t bits[4][2] = {{0x01, 0x08}, {0x02, 0x10}, {0x04, 0x20}, {0x40, 0x80}};
  size_t idx = size_t(py / 4) * cols_ + px / 2;
  dots_[idx] |= bits[py % 4][px % 2];
  color_[idx] = color;
}

void Canvas::line(double x0, double y0, double x1, double y1, int color) {
  if (!std::isfinite(x0) || !std::isfinite(y0) || !std::isfinite(x1) || !std::isfinite(y1)) return;
  // Liang–Barsky clipping to the pixel area (with a small margin).
  double xmin = -1, ymin = -1, xmax = pw(), ymax = ph();
  double dx = x1 - x0, dy = y1 - y0, t0 = 0, t1 = 1;
  double p[4] = {-dx, dx, -dy, dy}, q[4] = {x0 - xmin, xmax - x0, y0 - ymin, ymax - y0};
  for (int i = 0; i < 4; ++i) {
    if (p[i] == 0) {
      if (q[i] < 0) return;
      continue;
    }
    double r = q[i] / p[i];
    if (p[i] < 0) {
      if (r > t1) return;
      if (r > t0) t0 = r;
    } else {
      if (r < t0) return;
      if (r < t1) t1 = r;
    }
  }
  double ax = x0 + t0 * dx, ay = y0 + t0 * dy, bx = x0 + t1 * dx, by = y0 + t1 * dy;
  int ix0 = int(std::lround(ax)), iy0 = int(std::lround(ay)), ix1 = int(std::lround(bx)), iy1 = int(std::lround(by));
  int sx = ix0 < ix1 ? 1 : -1, sy = iy0 < iy1 ? 1 : -1;
  int ddx = std::abs(ix1 - ix0), ddy = -std::abs(iy1 - iy0), err = ddx + ddy;
  for (int guard = 0; guard < 100000; ++guard) {
    set(ix0, iy0, color);
    if (ix0 == ix1 && iy0 == iy1) break;
    int e2 = 2 * err;
    if (e2 >= ddy) {
      err += ddy;
      ix0 += sx;
    }
    if (e2 <= ddx) {
      err += ddx;
      iy0 += sy;
    }
  }
}

void Canvas::text(int col, int row, const std::string& s, int color, bool bold) {
  if (row < 0 || row >= rows_) return;
  size_t n = cact::u8len(s);
  for (size_t i = 0; i < n; ++i) {
    int c = col + int(i);
    if (c < 0 || c >= cols_) continue;
    size_t idx = size_t(row) * cols_ + c;
    over_[idx] = cact::u8substr(s, i, 1);
    overColor_[idx] = color;
    overBold_[idx] = bold;
  }
}

void Canvas::marker(int px, int py, const std::string& glyph, int color) {
  if (px < 0 || py < 0 || px >= pw() || py >= ph()) return;
  text(px / 2, py / 4, glyph, color, true);
}

static std::string brailleChar(uint8_t bits) {
  unsigned cp = 0x2800 + bits;
  std::string s;
  s += char(0xE0 | (cp >> 12));
  s += char(0x80 | ((cp >> 6) & 0x3F));
  s += char(0x80 | (cp & 0x3F));
  return s;
}

static std::string asciiDots(uint8_t bits) {
  bool top = bits & (0x01 | 0x08 | 0x02 | 0x10);
  bool bottom = bits & (0x04 | 0x20 | 0x40 | 0x80);
  if (top && bottom) return ":";
  if (top) return "'";
  return ".";
}

void Canvas::render(Term& t, int x, int y, int bg) const {
  for (int r = 0; r < rows_; ++r) {
    // Batch runs of the same color for speed.
    std::string run;
    int runColor = -2, runStart = 0;
    bool runBold = false;
    auto flush = [&]() {
      if (!run.empty()) t.put(x + runStart, y + r, run, Style(runColor < 0 ? bg : runColor, bg, runBold));
      run.clear();
    };
    for (int c = 0; c < cols_; ++c) {
      size_t idx = size_t(r) * cols_ + c;
      std::string glyph;
      int col;
      bool bold = false;
      if (!over_[idx].empty()) {
        glyph = over_[idx];
        col = overColor_[idx];
        bold = overBold_[idx];
      } else if (dots_[idx]) {
        glyph = t.ascii ? asciiDots(dots_[idx]) : brailleChar(dots_[idx]);
        col = color_[idx];
      } else {
        glyph = " ";
        col = runColor;  // spaces join any run
        if (run.empty()) col = -1;
      }
      if (col != runColor || bold != runBold) {
        flush();
        runColor = col;
        runBold = bold;
        runStart = c;
      }
      run += glyph;
    }
    flush();
  }
}

}  // namespace ui
