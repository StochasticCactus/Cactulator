#include "format.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace cact {

static const double kPi = 3.14159265358979323846;

size_t u8len(const std::string& s) {
  size_t n = 0;
  for (unsigned char c : s)
    if ((c & 0xC0) != 0x80) ++n;
  return n;
}

std::string u8substr(const std::string& s, size_t start, size_t count) {
  size_t i = 0, cp = 0;
  while (i < s.size() && cp < start) {
    ++i;
    while (i < s.size() && (static_cast<unsigned char>(s[i]) & 0xC0) == 0x80) ++i;
    ++cp;
  }
  size_t begin = i;
  size_t taken = 0;
  while (i < s.size() && taken < count) {
    ++i;
    while (i < s.size() && (static_cast<unsigned char>(s[i]) & 0xC0) == 0x80) ++i;
    ++taken;
  }
  return s.substr(begin, i - begin);
}

std::string padLeft(const std::string& s, size_t width) {
  size_t n = u8len(s);
  return n >= width ? s : std::string(width - n, ' ') + s;
}

std::string padRight(const std::string& s, size_t width) {
  size_t n = u8len(s);
  return n >= width ? s : s + std::string(width - n, ' ');
}

// "1.5e-07" -> "1.5E-7", "2e+20" -> "2E20"
static std::string tidyExponent(const std::string& s) {
  size_t e = s.find_first_of("eE");
  if (e == std::string::npos) return s;
  std::string mant = s.substr(0, e);
  std::string ex = s.substr(e + 1);
  bool neg = false;
  if (!ex.empty() && (ex[0] == '+' || ex[0] == '-')) {
    neg = ex[0] == '-';
    ex = ex.substr(1);
  }
  while (ex.size() > 1 && ex[0] == '0') ex.erase(0, 1);
  return mant + "E" + (neg ? "-" : "") + ex;
}

bool toFraction(double x, long long& p, long long& q, long long maxDen, double tol) {
  if (!std::isfinite(x) || std::abs(x) > 1e12) return false;
  double ax = std::abs(x);
  // Continued fraction expansion.
  long long h0 = 0, h1 = 1, k0 = 1, k1 = 0;
  double r = ax;
  for (int iter = 0; iter < 64; ++iter) {
    double a = std::floor(r);
    if (a > 9.2e15) break;
    long long ai = (long long)a;
    long long h2 = ai * h1 + h0, k2 = ai * k1 + k0;
    if (k2 > maxDen) break;
    h0 = h1; h1 = h2; k0 = k1; k1 = k2;
    double approx = double(h1) / double(k1);
    if (std::abs(approx - ax) <= tol * std::max(1.0, ax)) {
      p = x < 0 ? -h1 : h1;
      q = k1;
      return true;
    }
    double frac = r - a;
    if (frac < 1e-15) break;
    r = 1.0 / frac;
  }
  return false;
}

static std::string toBase(double x, int base) {
  long long v = std::llround(std::abs(x));
  const char* digits = "0123456789ABCDEF";
  std::string out;
  if (v == 0) out = "0";
  while (v > 0) {
    out.insert(out.begin(), digits[v % base]);
    v /= base;
  }
  const char* prefix = base == 16 ? "0x" : base == 2 ? "0b" : "0o";
  return std::string(x < 0 ? "-" : "") + prefix + out;
}

static std::string fmtSci(double x, int digits) {
  char buf[64];
  std::snprintf(buf, sizeof buf, "%.*E", std::clamp(digits, 0, 15), x);
  return tidyExponent(buf);
}

static std::string fmtEng(double x, int digits) {
  if (x == 0) return fmtSci(0, digits);
  int e = (int)std::floor(std::log10(std::abs(x)));
  int e3 = (int)std::floor(e / 3.0) * 3;
  double m = x / std::pow(10.0, e3);
  char buf[64];
  std::snprintf(buf, sizeof buf, "%.*f", std::clamp(digits, 0, 15), m);
  // Rounding may push the mantissa to 1000.
  if (std::abs(std::atof(buf)) >= 1000.0) {
    e3 += 3;
    m = x / std::pow(10.0, e3);
    std::snprintf(buf, sizeof buf, "%.*f", std::clamp(digits, 0, 15), m);
  }
  return std::string(buf) + "E" + std::to_string(e3);
}

std::string formatReal(double x, const DisplaySettings& ds) {
  if (std::isnan(x)) return "undefined";
  if (std::isinf(x)) return x > 0 ? (ds.unicode ? "∞" : "inf") : (ds.unicode ? "-∞" : "-inf");
  if (x == 0) x = 0;  // no "-0"

  bool isInt = std::abs(x) < 9.007199254740992e15 && x == std::floor(x);
  if (ds.base != 10 && isInt) return toBase(x, ds.base);

  if (ds.frac && x != 0 && !isInt) {
    long long p, q;
    // Multiples of pi first: pi/4, 3pi/2 ...
    if (toFraction(x / kPi, p, q, 12, 1e-12) && std::llabs(p) <= 100) {
      std::string pi = ds.unicode ? "π" : "pi";
      std::string num = p == 1 ? pi : p == -1 ? "-" + pi : std::to_string(p) + pi;
      return q == 1 ? num : num + "/" + std::to_string(q);
    }
    if (toFraction(x, p, q, 10000, 1e-12)) return std::to_string(p) + "/" + std::to_string(q);
  }

  char buf[64];
  switch (ds.fmt) {
    case DisplaySettings::Fmt::STD: {
      if (isInt && std::abs(x) < 1e15) {
        std::snprintf(buf, sizeof buf, "%.0f", x);
        return buf;
      }
      std::snprintf(buf, sizeof buf, "%.12g", x);
      return tidyExponent(buf);
    }
    case DisplaySettings::Fmt::FIX: {
      double ax = std::abs(x);
      if (ax >= 1e12 || (ax > 0 && ax < 0.5 * std::pow(10.0, -ds.digits)))
        return fmtSci(x, ds.digits);
      std::snprintf(buf, sizeof buf, "%.*f", std::clamp(ds.digits, 0, 15), x);
      std::string s = buf;
      if (s == "-0" || s.rfind("-0.", 0) == 0) {
        bool allZero = s.find_first_not_of("-0.") == std::string::npos;
        if (allZero) s.erase(0, 1);
      }
      return s;
    }
    case DisplaySettings::Fmt::SCI: return fmtSci(x, ds.digits);
    case DisplaySettings::Fmt::ENG: return fmtEng(x, ds.digits);
  }
  return "?";
}

std::string formatNumber(cplx z, const DisplaySettings& ds) {
  z = chopTiny(z);
  if (z.imag() == 0.0) return formatReal(z.real(), ds);
  DisplaySettings d = ds;
  d.frac = false;
  d.base = 10;
  auto imagPart = [&](double im) {
    std::string t = formatReal(im, d);
    if (t == "1") return std::string("i");
    if (t == "-1") return std::string("-i");
    return t + "i";
  };
  if (z.real() == 0.0) return imagPart(z.imag());
  std::string re = formatReal(z.real(), d);
  double im = z.imag();
  return re + (im < 0 ? "-" : "+") + imagPart(std::abs(im));
}

// Hide round-off noise like 1.1E-16 next to entries of size ~1.
static Value cleanMatrix(const Value& v) {
  if (!v.isMat()) return v;
  double scale = 0;
  for (const auto& z : v.mat.a) scale = std::max(scale, std::abs(z));
  Value out = v;
  for (auto& z : out.mat.a) {
    double re = std::abs(z.real()) < 1e-13 * scale ? 0.0 : z.real();
    double im = std::abs(z.imag()) < 1e-13 * scale ? 0.0 : z.imag();
    z = cplx(re, im);
  }
  return out;
}

std::string formatInline(const Value& v0, const DisplaySettings& ds) {
  Value v = cleanMatrix(v0);
  switch (v.kind) {
    case Value::Kind::None: return "";
    case Value::Kind::Num: return formatNumber(v.num, ds);
    case Value::Kind::Str: return v.str;
    case Value::Kind::Mat: {
      const Matrix& m = v.mat;
      if (v.vec) {
        std::string s = "[";
        for (size_t i = 0; i < m.a.size(); ++i) {
          if (i) s += ", ";
          s += formatNumber(m.a[i], ds);
        }
        return s + "]";
      }
      std::string s = "[";
      for (int r = 0; r < m.rows; ++r) {
        if (r) s += ", ";
        s += "[";
        for (int c = 0; c < m.cols; ++c) {
          if (c) s += ", ";
          s += formatNumber(m(r, c), ds);
        }
        s += "]";
      }
      return s + "]";
    }
  }
  return "";
}

std::vector<std::string> formatLines(const Value& v0, const DisplaySettings& ds) {
  if (!v0.isMat() || v0.vec || v0.mat.rows == 0) return {formatInline(v0, ds)};
  Value v = cleanMatrix(v0);
  const Matrix& m = v.mat;
  std::vector<std::vector<std::string>> cells(m.rows, std::vector<std::string>(m.cols));
  std::vector<size_t> width(m.cols, 1);
  for (int r = 0; r < m.rows; ++r)
    for (int c = 0; c < m.cols; ++c) {
      cells[r][c] = formatNumber(m(r, c), ds);
      width[c] = std::max(width[c], u8len(cells[r][c]));
    }
  std::vector<std::string> out;
  for (int r = 0; r < m.rows; ++r) {
    std::string left, right;
    if (!ds.unicode || m.rows == 1) {
      left = "[";
      right = "]";
    } else if (r == 0) {
      left = "⎡";
      right = "⎤";
    } else if (r == m.rows - 1) {
      left = "⎣";
      right = "⎦";
    } else {
      left = "⎢";
      right = "⎥";
    }
    std::string line = left + " ";
    for (int c = 0; c < m.cols; ++c) {
      if (c) line += "  ";
      line += padLeft(cells[r][c], width[c]);
    }
    out.push_back(line + " " + right);
  }
  return out;
}

std::string formatShort(cplx z, int width, const DisplaySettings& ds) {
  DisplaySettings d = ds;
  std::string s = formatNumber(z, d);
  if ((int)u8len(s) <= width) return s;
  d.frac = false;
  if (z.imag() == 0.0) {
    for (int digits = 10; digits >= 1; --digits) {
      char buf[64];
      std::snprintf(buf, sizeof buf, "%.*g", digits, z.real());
      s = tidyExponent(buf);
      if ((int)s.size() <= width) return s;
    }
  }
  return u8substr(s, 0, std::max(0, width - 1)) + "…";
}

std::string serializeReal(double x) {
  if (std::isnan(x)) return "nan";
  if (std::isinf(x)) return x > 0 ? "inf" : "-inf";
  char buf[64];
  std::snprintf(buf, sizeof buf, "%.17g", x);
  return buf;
}

static std::string serializeNum(cplx z) {
  if (z.imag() == 0.0) return serializeReal(z.real());
  std::string re = serializeReal(z.real());
  std::string im = serializeReal(std::abs(z.imag()));
  return "(" + re + (z.imag() < 0 ? "-" : "+") + im + "i)";
}

std::string serialize(const Value& v) {
  switch (v.kind) {
    case Value::Kind::None: return "0";
    case Value::Kind::Num: return serializeNum(v.num);
    case Value::Kind::Str: {
      std::string s = "\"";
      for (char c : v.str) {
        if (c == '"' || c == '\\') s += '\\';
        if (c == '\n') {
          s += "\\n";
          continue;
        }
        s += c;
      }
      return s + "\"";
    }
    case Value::Kind::Mat: {
      const Matrix& m = v.mat;
      std::string s = "[";
      if (v.vec) {
        for (size_t i = 0; i < m.a.size(); ++i) s += (i ? "," : "") + serializeNum(m.a[i]);
        return s + "]";
      }
      for (int r = 0; r < m.rows; ++r) {
        s += r ? ",[" : "[";
        for (int c = 0; c < m.cols; ++c) s += (c ? "," : "") + serializeNum(m(r, c));
        s += "]";
      }
      return s + "]";
    }
  }
  return "0";
}

}  // namespace cact
