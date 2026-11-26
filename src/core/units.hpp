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

// Cactulator - unit conversion tables.
#pragma once

#include <string>
#include <vector>

namespace cact {

struct Unit {
  std::string name;   // what you type: "km"
  std::string label;  // long name: "kilometre"
  double factor;      // value in the category's base unit
  double offset = 0;  // for temperatures: base = value * factor + offset
};

struct UnitCategory {
  std::string name;
  std::vector<Unit> units;
};

const std::vector<UnitCategory>& unitCategories();
// Finds a unit by name (case-sensitive first, then case-insensitive).
bool findUnit(const std::string& name, int& category, int& index);
double convertUnits(double value, const std::string& from, const std::string& to);
double convertUnits(double value, int category, int from, int to);

}  // namespace cact
