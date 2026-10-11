#include <iostream>
#include <string>
#include <cmath>

void put(int row, int col, const std::string& s)
{
  std::cout << "\x1b[" << row << ";" << col << "H" << s;
}

int main()
{
  const int originRow = 12;   // row of the x axis (y = 0)
  const int originCol = 6;    // column of the y axis (leaves room for labels)
  const int amplitude = 10;   // rows above/below the axis
  const int width = 120;      // columns: 3 degrees each = 360 degrees

  std::cout << "\x1b[2J";     // clear screen

  // Axes in gray
  std::cout << "\x1b[90m";

  for (int r = originRow - amplitude - 1; r <= originRow + amplitude + 1; r++)
    put(r, originCol, "|");
  for (int c = originCol; c <= originCol + width; c++)
    put(originRow, c, "-");

  put(originRow, originCol, "+");
  put(originRow - amplitude - 1, originCol, "^");      // y arrow
  put(originRow, originCol + width + 1, ">");          // x arrow
  put(originRow, originCol + width + 3, "x");

  // Y labels
  put(originRow - amplitude, 2, "+1");
  put(originRow, 2, " 0");
  put(originRow + amplitude, 2, "-1");

  // X ticks every 90 degrees (30 columns), labels along the bottom
  const int labelRow = originRow + amplitude + 2;
  for (int k = 1; k <= 4; k++)
  {
    int col = originCol + k * 30;
    put(originRow, col, "+");
    put(labelRow, col - 1, std::to_string(k * 90));
  }

  std::cout << "\x1b[0m";  // reset color

  // Wave
  for (int col = 1; col <= width; col++)
  {
    double radians = col * 3.0 * M_PI / 180.0;
    int row = originRow - (int)std::round(std::sin(radians) * amplitude);
    put(row, originCol + col, "*");
  }

  put(labelRow + 2, 1, "");   // park cursor below the plot
  std::cout << std::flush;
  return 0;
}
