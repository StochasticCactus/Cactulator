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

#include <iostream>
#include <cmath>

int main()
{
  std::cout << "\x1b[2J";                      // clear screen

  for (int col = 1; col <= 120; col++)
  {
    double radians = col * 3.0 * M_PI / 180.0; // 3 degrees per column
    int row = 12 + (int)std::round(std::sin(radians) * 10);

    std::cout << "\x1b[" << row << ";" << col << "H*";
  }

  std::cout << "\x1b[25;1H" << std::flush;     // park cursor below the plot
  return 0;
}
