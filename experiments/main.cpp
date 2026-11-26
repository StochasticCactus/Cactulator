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
#include <string>
#include <cmath>

void drawStatus(int row, const std::string& text) {
    std::cout << "\x1b[" << row << ";1H\x1b[2K" << text << std::flush;
}

int main() {
  
  int maxi = 30;
  int maxj = 80;
  std::string buffer = "";

  std::cout << "\x1b[2J"; 

  std::cout << R"cactus(
         \ \ | | / / 
       _.-'''''''''-._
    .'  /  /  |  \  \  '.
  \/  \/ \/  \|/  \/ \/  \/
  |   |   |   |   |   |   |
 \|  \|  \|  \|/  |/  |/  |/
  |   |   |  \|/  |   |   |
 \|  \|  \|  \|/  |/  |/  |/
   \   \  \   |   /  /   /
    '.  \  \  |  /  /  .'
       '-._________.-'
       [_____________]
        \___________/
    ~~~~~~~~~~~~~~~~~~~~~
)cactus";

  std::cout << "\n";

  for (int i = 0; i < maxi; i++)
  {

    if (i == 0) 
    {
      

      for (int j = 0; j < maxj; j++)
      {
        std::cout << "_";
      }
    
    
    }

    std::cout << "\n";

    for (int j = 0; j < maxj; j++)
    {
      if (j == 0)
      {
        std::cout << "|";
      }
      else {
        std::cout << " ";
      }
      if (j >= maxj - 1)
      {
        std::cout << "|";

      }
    }
    
    if (i >= 29) 
    {
    

      std::cout << "\n";
      std::cout << "|";      
      
      for (int j = 0; j < maxj - 1; j++)
      {
        std::cout << "_";
      }
    
      std::cout << "|";      
    
    }



  }

  std::cout << "\n";
  std::cout << "\x1b[24;15H";
  std::cout << "Cactus!\n" << std::flush;
  std::cout << "\x1b[25;15H";


  while (buffer != "exit") {

    std::cout << "\x1b[25;16H"; 
    std::cout << "\x1b[2K" << std::flush;
    std::cout << ">";
    std::cin >> buffer;
    std::cout << "\x1b[28;15H";
    std::cout << "\x1b[2K" << std::flush;
    std::cout << buffer << std::endl << std::flush;
    std::cout <<"\x1b[70;0H";

  }
  

  for (int i = 0; i < 360; i++)
  {

    
    std::cout << "\x1b[" << std::to_string((int) (std::sin(i) * 100) ) << ";" << std::to_string(i) << "H";
    std::cout << "*";


  }

  return 0;
}
