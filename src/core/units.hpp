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
