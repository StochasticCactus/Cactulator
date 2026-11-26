#include "units.hpp"

#include <algorithm>
#include <cctype>

#include "value.hpp"

namespace cact {

const std::vector<UnitCategory>& unitCategories() {
  static const double NA = 6.02214076e23, H = 6.62607015e-34, C = 299792458.0, EV = 1.602176634e-19;
  static const std::vector<UnitCategory> cats = {
      {"Length",
       {{"m", "metre", 1},
        {"km", "kilometre", 1e3},
        {"cm", "centimetre", 1e-2},
        {"mm", "millimetre", 1e-3},
        {"um", "micrometre (µm)", 1e-6},
        {"nm", "nanometre", 1e-9},
        {"pm", "picometre", 1e-12},
        {"Å", "ångström", 1e-10},
        {"in", "inch", 0.0254},
        {"ft", "foot", 0.3048},
        {"yd", "yard", 0.9144},
        {"mi", "mile", 1609.344},
        {"nmi", "nautical mile", 1852},
        {"au", "astronomical unit", 149597870700.0},
        {"ly", "light year", 9460730472580800.0},
        {"pc", "parsec", 3.0856775814913673e16}}},
      {"Mass",
       {{"kg", "kilogram", 1},
        {"g", "gram", 1e-3},
        {"mg", "milligram", 1e-6},
        {"ug", "microgram (µg)", 1e-9},
        {"t", "tonne", 1e3},
        {"lb", "pound", 0.45359237},
        {"oz", "ounce", 0.028349523125},
        {"st", "stone", 6.35029318},
        {"ton", "short ton (US)", 907.18474},
        {"Da", "dalton / amu", 1.66053906660e-27},
        {"ct", "carat", 2e-4}}},
      {"Time",
       {{"s", "second", 1},
        {"ms", "millisecond", 1e-3},
        {"us", "microsecond (µs)", 1e-6},
        {"ns", "nanosecond", 1e-9},
        {"min", "minute", 60},
        {"h", "hour", 3600},
        {"d", "day", 86400},
        {"wk", "week", 604800},
        {"yr", "year (365.25 d)", 31557600}}},
      {"Temperature",
       {{"K", "kelvin", 1, 0},
        {"C", "degree Celsius", 1, 273.15},
        {"F", "degree Fahrenheit", 5.0 / 9.0, 273.15 - 32 * 5.0 / 9.0},
        {"R", "degree Rankine", 5.0 / 9.0, 0}}},
      {"Volume",
       {{"L", "litre", 1e-3},
        {"mL", "millilitre", 1e-6},
        {"uL", "microlitre (µL)", 1e-9},
        {"m3", "cubic metre", 1},
        {"dm3", "cubic decimetre", 1e-3},
        {"cm3", "cubic centimetre", 1e-6},
        {"gal", "US gallon", 3.785411784e-3},
        {"qt", "US quart", 9.46352946e-4},
        {"pt", "US pint", 4.73176473e-4},
        {"cup", "US cup", 2.365882365e-4},
        {"floz", "US fluid ounce", 2.95735295625e-5},
        {"tbsp", "tablespoon", 1.478676478125e-5},
        {"tsp", "teaspoon", 4.92892159375e-6},
        {"ukgal", "imperial gallon", 4.54609e-3},
        {"ft3", "cubic foot", 0.028316846592},
        {"in3", "cubic inch", 1.6387064e-5}}},
      {"Area",
       {{"m2", "square metre", 1},
        {"km2", "square kilometre", 1e6},
        {"cm2", "square centimetre", 1e-4},
        {"mm2", "square millimetre", 1e-6},
        {"ha", "hectare", 1e4},
        {"acre", "acre", 4046.8564224},
        {"ft2", "square foot", 0.09290304},
        {"in2", "square inch", 6.4516e-4},
        {"yd2", "square yard", 0.83612736},
        {"mi2", "square mile", 2589988.110336}}},
      {"Speed",
       {{"m/s", "metres per second", 1},
        {"km/h", "kilometres per hour", 1 / 3.6},
        {"mph", "miles per hour", 0.44704},
        {"kn", "knot", 1852.0 / 3600.0},
        {"ft/s", "feet per second", 0.3048},
        {"c", "speed of light", C}}},
      {"Energy",
       {{"J", "joule", 1},
        {"kJ", "kilojoule", 1e3},
        {"MJ", "megajoule", 1e6},
        {"cal", "calorie", 4.184},
        {"kcal", "kilocalorie", 4184},
        {"Wh", "watt hour", 3600},
        {"kWh", "kilowatt hour", 3.6e6},
        {"eV", "electronvolt", EV},
        {"BTU", "British thermal unit", 1055.05585262},
        {"erg", "erg", 1e-7},
        {"Eh", "hartree", 4.3597447222071e-18}}},
      {"Molar energy",
       {{"J/mol", "joule per mole", 1},
        {"kJ/mol", "kilojoule per mole", 1e3},
        {"kcal/mol", "kilocalorie per mole", 4184},
        {"eV/particle", "electronvolt per particle", EV * NA},
        {"cm-1", "wavenumber", H * C * 100 * NA},
        {"Eh/particle", "hartree per particle", 4.3597447222071e-18 * NA}}},
      {"Pressure",
       {{"Pa", "pascal", 1},
        {"kPa", "kilopascal", 1e3},
        {"MPa", "megapascal", 1e6},
        {"bar", "bar", 1e5},
        {"mbar", "millibar", 100},
        {"atm", "atmosphere", 101325},
        {"torr", "torr", 101325.0 / 760.0},
        {"mmHg", "millimetre of mercury", 133.322387415},
        {"psi", "pound per square inch", 6894.757293168},
        {"inHg", "inch of mercury", 3386.389}}},
      {"Power",
       {{"W", "watt", 1},
        {"kW", "kilowatt", 1e3},
        {"MW", "megawatt", 1e6},
        {"hp", "horsepower", 745.69987158227022},
        {"BTU/h", "BTU per hour", 0.29307107017}}},
      {"Force",
       {{"N", "newton", 1},
        {"kN", "kilonewton", 1e3},
        {"lbf", "pound-force", 4.4482216152605},
        {"kgf", "kilogram-force", 9.80665},
        {"dyn", "dyne", 1e-5}}},
      {"Angle",
       {{"deg", "degree", 3.14159265358979323846 / 180},
        {"rad", "radian", 1},
        {"grad", "gradian", 3.14159265358979323846 / 200},
        {"arcmin", "arc minute", 3.14159265358979323846 / 10800},
        {"arcsec", "arc second", 3.14159265358979323846 / 648000},
        {"turn", "full turn", 2 * 3.14159265358979323846}}},
      {"Frequency",
       {{"Hz", "hertz", 1},
        {"kHz", "kilohertz", 1e3},
        {"MHz", "megahertz", 1e6},
        {"GHz", "gigahertz", 1e9},
        {"rpm", "revolutions per minute", 1.0 / 60.0}}},
      {"Data",
       {{"bit", "bit", 0.125},
        {"B", "byte", 1},
        {"KB", "kilobyte (1000 B)", 1e3},
        {"MB", "megabyte", 1e6},
        {"GB", "gigabyte", 1e9},
        {"TB", "terabyte", 1e12},
        {"KiB", "kibibyte (1024 B)", 1024.0},
        {"MiB", "mebibyte", 1048576.0},
        {"GiB", "gibibyte", 1073741824.0},
        {"TiB", "tebibyte", 1099511627776.0}}},
  };
  return cats;
}

static std::string lower(std::string s) {
  for (auto& c : s) c = char(std::tolower((unsigned char)c));
  return s;
}

bool findUnit(const std::string& raw, int& category, int& index) {
  std::string name = raw;
  // Friendly aliases.
  if (name == "°C" || name == "degC" || name == "celsius") name = "C";
  if (name == "°F" || name == "degF" || name == "fahrenheit") name = "F";
  if (name == "kelvin") name = "K";
  if (name == "µm" || name == "μm" || name == "micron") name = "um";
  if (name == "µg" || name == "μg") name = "ug";
  if (name == "µs" || name == "μs") name = "us";
  if (name == "µL" || name == "μL" || name == "uL") name = "uL";
  if (name == "l") name = "L";
  if (name == "ml") name = "mL";
  if (name == "kph" || name == "kmh") name = "km/h";
  if (name == "A" || name == "angstrom" || name == "Ang") name = "Å";
  if (name == "°" || name == "degree" || name == "degrees") name = "deg";
  if (name == "lbs") name = "lb";
  if (name == "hr") name = "h";
  if (name == "sec") name = "s";
  if (name == "inch" || name == "inches") name = "in";
  if (name == "feet" || name == "foot") name = "ft";
  if (name == "mile" || name == "miles") name = "mi";
  if (name == "amu" || name == "u") name = "Da";
  if (name == "wavenumber" || name == "cm^-1") name = "cm-1";
  if (name == "m^3") name = "m3";
  if (name == "m^2") name = "m2";
  const auto& cats = unitCategories();
  for (int pass = 0; pass < 2; ++pass)
    for (size_t c = 0; c < cats.size(); ++c)
      for (size_t u = 0; u < cats[c].units.size(); ++u) {
        const std::string& n = cats[c].units[u].name;
        if (pass == 0 ? n == name : lower(n) == lower(name)) {
          category = int(c);
          index = int(u);
          return true;
        }
      }
  return false;
}

double convertUnits(double value, int category, int from, int to) {
  const auto& cat = unitCategories()[category];
  const Unit& a = cat.units[from];
  const Unit& b = cat.units[to];
  double base = value * a.factor + a.offset;
  return (base - b.offset) / b.factor;
}

double convertUnits(double value, const std::string& from, const std::string& to) {
  int c1, i1, c2, i2;
  if (!findUnit(from, c1, i1)) throw CalcError("unknown unit '" + from + "'");
  if (!findUnit(to, c2, i2)) throw CalcError("unknown unit '" + to + "'");
  if (c1 != c2) {
    const auto& cats = unitCategories();
    throw CalcError("can't convert " + cats[c1].name + " (" + from + ") to " + cats[c2].name + " (" + to + ")");
  }
  return convertUnits(value, c1, i1, i2);
}

}  // namespace cact
