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

// Cactulator - color themes (xterm-256 color indices).
#pragma once

#include <string>
#include <vector>

namespace ui {

struct Theme {
  std::string name;
  // calculator body
  int bodyBg, bodyFg, bodyDim, brand, brandAccent;
  // LCD display
  int lcdBg, lcdFg, lcdDim, lcdStrong, lcdAccent, lcdError, lcdSelBg, lcdSelFg, lcdBorder;
  // keys
  int keyBg, keyFg, numBg, numFg, opBg, opFg, appBg, appFg, enterBg, enterFg, shiftBg, shiftFg, shiftLabel, pressBg;
  // soft-key menu tabs
  int menuBg, menuFg, menuKey;
  // syntax highlighting
  int synNum, synFunc, synVar, synOp, synParen, synMatch, synKeyword, synString, synComment, synError;
  // graphs
  int axis, grid, plot[6], cursor;
};

const std::vector<Theme>& themes();
const Theme& themeByName(const std::string& name);
int themeIndex(const std::string& name);

}  // namespace ui
