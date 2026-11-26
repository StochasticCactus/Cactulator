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

#include "theme.hpp"

#include <algorithm>
#include <cctype>

namespace ui {

const std::vector<Theme>& themes() {
  static const std::vector<Theme> t = {
      // name, body(bg fg dim brand accent)
      {"Classic", 236, 252, 245, 179, 208,
       // lcd: bg fg dim strong accent error selBg selFg border
       151, 234, 240, 16, 24, 124, 108, 16, 242,
       // keys: key num op app enter shift shiftLabel press
       238, 254, 240, 231, 237, 223, 24, 231, 179, 16, 208, 16, 214, 250,
       // menu
       234, 151, 179,
       // syntax: num func var op paren match keyword string comment error
       18, 88, 54, 236, 240, 160, 94, 22, 243, 160,
       // graph: axis grid plot[6] cursor
       241, 108, {18, 124, 22, 130, 90, 31}, 160},
      {"Cactus", 22, 194, 108, 186, 173,
       230, 236, 245, 22, 130, 160, 186, 16, 64,
       28, 230, 65, 231, 29, 230, 130, 230, 179, 16, 173, 16, 216, 151,
       22, 230, 186,
       25, 130, 91, 236, 244, 160, 28, 64, 246, 160,
       245, 252, {28, 166, 25, 125, 94, 30}, 160},
      {"Retro LCD", 232, 34, 22, 46, 40,
       16, 46, 28, 118, 48, 196, 22, 118, 28,
       234, 40, 235, 46, 234, 82, 22, 120, 28, 16, 34, 16, 34, 28,
       22, 46, 118,
       120, 46, 82, 34, 28, 226, 48, 157, 28, 196,
       28, 22, {46, 226, 51, 213, 214, 159}, 226},
      {"Midnight", 235, 250, 242, 75, 204,
       234, 252, 243, 231, 81, 203, 238, 231, 238,
       237, 252, 239, 231, 237, 117, 24, 231, 32, 231, 204, 16, 211, 244,
       238, 252, 75,
       117, 215, 183, 250, 244, 226, 204, 150, 242, 203,
       241, 237, {81, 204, 150, 221, 141, 45}, 226},
      {"Paper", 254, 236, 245, 25, 166,
       231, 235, 246, 16, 25, 160, 153, 16, 249,
       250, 16, 255, 16, 252, 25, 25, 231, 166, 231, 208, 16, 166, 244,
       238, 231, 179,
       25, 130, 91, 238, 245, 160, 28, 64, 246, 160,
       245, 253, {25, 160, 28, 166, 91, 30}, 160},
  };
  return t;
}

static std::string lower(std::string s) {
  for (auto& c : s) c = char(std::tolower((unsigned char)c));
  return s;
}

int themeIndex(const std::string& name) {
  const auto& t = themes();
  std::string n = lower(name);
  for (size_t i = 0; i < t.size(); ++i)
    if (lower(t[i].name) == n || lower(t[i].name).rfind(n, 0) == 0) return int(i);
  if (n == "hp" || n == "hp classic") return 0;
  return -1;
}

const Theme& themeByName(const std::string& name) {
  int i = themeIndex(name);
  return themes()[i < 0 ? 0 : size_t(i)];
}

}  // namespace ui
