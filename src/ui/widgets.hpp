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

// Cactulator - small reusable UI pieces: line editor, syntax colors, wrapping.
#pragma once

#include <string>
#include <vector>

#include "core/interp.hpp"
#include "term.hpp"
#include "theme.hpp"

namespace ui {

class LineEdit {
 public:
  std::string text;
  size_t cur = 0;  // cursor position in characters

  void set(const std::string& s);
  void clear();
  void insert(const std::string& s);
  bool backspace();
  bool del();
  void left();
  void right();
  void home();
  void end();
  void wordLeft();
  void wordRight();
  size_t length() const;
  bool empty() const { return text.empty(); }
  std::string beforeCursor() const;
  std::string afterCursor() const;
  // Handles common editing keys. Returns true if the event was used.
  bool handle(const Event& e);
};

struct Span {
  size_t start, len;  // in characters
  int color;
  bool bold = false;
};

// Syntax colors for an expression / program line. `cursor` enables paren matching.
std::vector<Span> highlight(const std::string& text, const Theme& th, const cact::Interp* in, long cursor = -1);

// Draw a line editor (with block cursor and optional ghost completion text).
void drawEdit(Term& t, int x, int y, int w, const LineEdit& e, const Theme& th, const Style& base, bool focused,
              const std::vector<Span>& spans, const std::string& ghost = "");

// Draw text with color spans (no cursor), clipped to w.
void drawSpans(Term& t, int x, int y, int w, const std::string& text, const std::vector<Span>& spans,
               const Style& base, size_t offset = 0);

std::vector<std::string> wrapText(const std::string& s, int width);

// The identifier being typed just before the cursor (for completion), and its start.
std::string identBeforeCursor(const LineEdit& e, size_t* start = nullptr);
// Identifier under/just before the cursor, to show help for it.
std::string identAtCursor(const LineEdit& e);
// The function whose argument list contains the cursor: "sin(3|" -> "sin".
std::string enclosingCall(const LineEdit& e);

// Auto-close parentheses/brackets that were left open.
std::string balanceBrackets(const std::string& s);

}  // namespace ui
