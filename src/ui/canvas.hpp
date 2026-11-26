// Cactulator - braille-dot drawing canvas (2×4 dots per character cell).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "term.hpp"

namespace ui {

class Canvas {
 public:
  Canvas(int cols, int rows);
  int cols() const { return cols_; }
  int rows() const { return rows_; }
  int pw() const { return cols_ * 2; }
  int ph() const { return rows_ * 4; }

  void set(int px, int py, int color);
  // Line in pixel coordinates (doubles, clipped to the canvas).
  void line(double x0, double y0, double x1, double y1, int color);
  // Text overlay at a character cell (wins over dots).
  void text(int col, int row, const std::string& s, int color, bool bold = false);
  // Draw a marker glyph at the cell containing the pixel.
  void marker(int px, int py, const std::string& glyph, int color);

  void render(Term& t, int x, int y, int bg) const;

 private:
  int cols_, rows_;
  std::vector<uint8_t> dots_;
  std::vector<int> color_;
  std::vector<std::string> over_;
  std::vector<int> overColor_;
  std::vector<bool> overBold_;
};

}  // namespace ui
