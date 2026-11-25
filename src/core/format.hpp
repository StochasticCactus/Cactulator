// Cactulator - turning values into text.
#pragma once

#include <string>
#include <vector>

#include "value.hpp"

namespace cact {

struct DisplaySettings {
  enum class Fmt { STD, FIX, SCI, ENG };
  Fmt fmt = Fmt::STD;
  int digits = 4;      // used by FIX / SCI / ENG
  bool frac = false;   // show results as fractions (and multiples of pi) when exact
  int base = 10;       // 10, 16, 2 or 8 for integer results
  bool unicode = true; // use pretty glyphs (matrix brackets, pi)
};

std::string formatReal(double x, const DisplaySettings& ds);
std::string formatNumber(cplx z, const DisplaySettings& ds);

// Single-line rendering ([[1, 2], [3, 4]]).
std::string formatInline(const Value& v, const DisplaySettings& ds);
// Possibly multi-line rendering (pretty matrices).
std::vector<std::string> formatLines(const Value& v, const DisplaySettings& ds);

// Short form used in grid cells, at most `width` characters.
std::string formatShort(cplx z, int width, const DisplaySettings& ds);

// Exact, parseable rendering (17 significant digits) used for saving state.
std::string serialize(const Value& v);
std::string serializeReal(double x);

// Best rational approximation p/q with q <= maxDen; returns false if none is close.
bool toFraction(double x, long long& p, long long& q, long long maxDen = 10000, double tol = 1e-12);

// UTF-8 helpers (all glyphs we use are single-width).
size_t u8len(const std::string& s);
std::string u8substr(const std::string& s, size_t start, size_t count = std::string::npos);
std::string padLeft(const std::string& s, size_t width);
std::string padRight(const std::string& s, size_t width);

}  // namespace cact
