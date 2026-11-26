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
