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
