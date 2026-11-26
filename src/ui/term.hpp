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

// Cactulator - thin wrapper around ncurses: colors, UTF-8 text, input events.
#pragma once

#include <map>
#include <string>
#include <utility>

namespace ui {

struct Rect {
  int x = 0, y = 0, w = 0, h = 0;
  bool contains(int px, int py) const { return px >= x && px < x + w && py >= y && py < y + h; }
  int right() const { return x + w; }
  int bottom() const { return y + h; }
};

struct Style {
  int fg = 7, bg = 0;  // xterm-256 color indices
  bool bold = false;
  bool underline = false;
  Style() = default;
  Style(int f, int b, bool bo = false, bool ul = false) : fg(f), bg(b), bold(bo), underline(ul) {}
  Style withFg(int f) const { Style s = *this; s.fg = f; return s; }
  Style withBg(int b) const { Style s = *this; s.bg = b; return s; }
  Style withBold(bool b = true) const { Style s = *this; s.bold = b; return s; }
};

struct Event {
  enum class Type { None, Key, Text, Mouse, Resize };
  Type type = Type::None;
  int key = 0;       // ncurses KEY_* code or control character (Ctrl-A = 1 ...)
  std::string text;  // UTF-8 for Type::Text
  int mx = 0, my = 0;
  enum class Mouse { Press, Release, WheelUp, WheelDown, Other };
  Mouse mouse = Mouse::Other;
  bool is(int k) const { return type == Type::Key && key == k; }
  bool isChar(char c) const { return type == Type::Text && text.size() == 1 && text[0] == c; }
};

// Key codes we use (mirrors of ncurses values, so screens don't need curses.h).
namespace keys {
extern const int Up, Down, Left, Right, Home, End, PageUp, PageDown, Backspace, Delete, Insert, Enter, BackTab,
    SLeft, SRight, SUp, SDown, CtrlLeft, CtrlRight;
int F(int n);
constexpr int Esc = 27;
constexpr int Tab = 9;
constexpr int Ctrl(char c) { return c & 0x1f; }
}  // namespace keys

class Term {
 public:
  bool init(bool mouse);
  void shutdown();
  bool active() const { return active_; }
  int width() const;
  int height() const;

  void begin();  // start a frame
  void end();    // flush to the terminal

  // Draw text (clipped at the screen edge and to maxw columns if given).
  void put(int x, int y, const std::string& text, const Style& st, int maxw = -1);
  void fill(const Rect& r, const Style& st, const std::string& ch = " ");
  void hrule(int x, int y, int w, const Style& st, const std::string& ch);
  void vrule(int x, int y, int h, const Style& st, const std::string& ch);
  void frame(const Rect& r, const Style& st, bool rounded = true);

  // Wait for input (-1 = forever). Returns Event::None on timeout.
  Event next(int timeoutMs);
  // Non-blocking check for Esc / Ctrl-C (used while long computations run).
  bool interrupted();

  void resetColors();
  int colors() const { return colors_; }
  bool ascii = false;  // avoid box-drawing / braille glyphs

 private:
  bool active_ = false;
  int colors_ = 8;
  std::map<std::pair<int, int>, short> pairs_;
  short nextPair_ = 1;
  short pairFor(int fg, int bg);
  int mapColor(int c) const;
  void apply(const Style& st);
};

}  // namespace ui
