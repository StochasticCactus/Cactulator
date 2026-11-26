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

#include "lexer.hpp"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <set>

#include "value.hpp"

namespace cact {

bool isKeyword(const std::string& s) {
  static const std::set<std::string> kw = {
      "if",    "then",  "elif",   "elseif", "else",  "end",      "while",    "do",
      "for",   "to",    "step",   "in",     "prog",  "function", "return",   "break",
      "continue", "print", "global", "repeat", "until", "and",   "or",       "not"};
  return kw.count(s) > 0;
}

int columnOf(const std::string& src, int pos) {
  int col = 1;
  for (int i = 0; i < pos && i < (int)src.size(); ++i) {
    unsigned char c = src[i];
    if (c == '\n') col = 1;
    else if ((c & 0xC0) != 0x80) ++col;
  }
  return col;
}

namespace {

struct Sym {
  const char* utf8;
  Tok type;
  const char* text;
};

// Unicode symbols and multi-character operators, longest first.
const Sym kSymbols[] = {
    {"⁻¹", Tok::Op, "⁻¹"}, {"×", Tok::Op, "*"},   {"·", Tok::Op, "*"},   {"÷", Tok::Op, "/"},
    {"−", Tok::Op, "-"},    {"√", Tok::Op, "√"},   {"²", Tok::Op, "²"},   {"³", Tok::Op, "³"},
    {"°", Tok::Op, "°"},    {"≤", Tok::Op, "<="},  {"≥", Tok::Op, ">="},  {"≠", Tok::Op, "!="},
    {"→", Tok::Op, "->"},   {"π", Tok::Ident, "pi"}, {"∞", Tok::Ident, "inf"},
    {"==", Tok::Op, "=="},  {"!=", Tok::Op, "!="}, {"<>", Tok::Op, "!="}, {"<=", Tok::Op, "<="},
    {">=", Tok::Op, ">="},  {"->", Tok::Op, "->"}, {"&&", Tok::Op, "and"}, {"||", Tok::Op, "or"},
    {"**", Tok::Op, "^"},
};

bool isIdentStart(unsigned char c) { return std::isalpha(c) || c == '_' || c >= 0x80; }
bool isIdentChar(unsigned char c) { return std::isalnum(c) || c == '_' || c >= 0x80; }

bool startsWithSymbol(const std::string& s, size_t i, size_t& len) {
  for (const auto& sym : kSymbols) {
    size_t n = std::strlen(sym.utf8);
    if (s.compare(i, n, sym.utf8) == 0 && (unsigned char)sym.utf8[0] >= 0x80) {
      len = n;
      return true;
    }
  }
  return false;
}

}  // namespace

std::vector<Token> tokenize(const std::string& src) {
  std::vector<Token> out;
  size_t i = 0;
  int line = 1;
  int depth = 0;  // nesting of ( and [ : newlines inside are ignored
  const size_t n = src.size();

  auto push = [&](Tok t, std::string text, size_t start, size_t end) {
    Token tk;
    tk.type = t;
    tk.text = std::move(text);
    tk.pos = int(start);
    tk.end = int(end);
    tk.line = line;
    out.push_back(std::move(tk));
  };

  while (i < n) {
    unsigned char c = src[i];
    if (c == ' ' || c == '\t' || c == '\r') {
      ++i;
      continue;
    }
    if (c == '#') {
      while (i < n && src[i] != '\n') ++i;
      continue;
    }
    if (c == '\n') {
      if (depth == 0) push(Tok::Newline, "\n", i, i + 1);
      ++line;
      ++i;
      continue;
    }

    // Numbers
    if (std::isdigit(c) || (c == '.' && i + 1 < n && std::isdigit((unsigned char)src[i + 1]))) {
      size_t start = i;
      double value = 0;
      if (c == '0' && i + 2 < n && std::strchr("xXbBoO", src[i + 1]) &&
          std::isxdigit((unsigned char)src[i + 2])) {
        int base = (src[i + 1] == 'x' || src[i + 1] == 'X') ? 16 : (src[i + 1] == 'b' || src[i + 1] == 'B') ? 2 : 8;
        i += 2;
        size_t ds = i;
        while (i < n && std::isxdigit((unsigned char)src[i])) ++i;
        std::string digits = src.substr(ds, i - ds);
        for (char d : digits) {
          int v = std::isdigit((unsigned char)d) ? d - '0' : std::tolower(d) - 'a' + 10;
          if (v >= base) throw CalcError("digit '" + std::string(1, d) + "' is not valid in base " + std::to_string(base));
          value = value * base + v;
        }
      } else {
        while (i < n && std::isdigit((unsigned char)src[i])) ++i;
        if (i < n && src[i] == '.') {
          ++i;
          while (i < n && std::isdigit((unsigned char)src[i])) ++i;
        }
        if (i < n && (src[i] == 'e' || src[i] == 'E')) {
          size_t j = i + 1;
          if (j < n && (src[j] == '+' || src[j] == '-')) ++j;
          if (j < n && std::isdigit((unsigned char)src[j])) {
            i = j;
            while (i < n && std::isdigit((unsigned char)src[i])) ++i;
          }
        }
        value = std::strtod(src.substr(start, i - start).c_str(), nullptr);
      }
      Token tk;
      tk.type = Tok::Num;
      tk.num = value;
      tk.pos = int(start);
      tk.line = line;
      // Imaginary literal: 4i (but not 4if / 4inch)
      if (i < n && src[i] == 'i' && (i + 1 >= n || !isIdentChar((unsigned char)src[i + 1]))) {
        tk.imag = true;
        ++i;
      }
      tk.end = int(i);
      tk.text = src.substr(start, i - start);
      out.push_back(tk);
      continue;
    }

    // Strings
    if (c == '"') {
      size_t start = i++;
      std::string s;
      while (i < n && src[i] != '"') {
        if (src[i] == '\\' && i + 1 < n) {
          char e = src[i + 1];
          s += e == 'n' ? '\n' : e == 't' ? '\t' : e;
          i += 2;
          continue;
        }
        if (src[i] == '\n') ++line;
        s += src[i++];
      }
      if (i >= n) throw CalcError("text is missing its closing \"");
      ++i;
      push(Tok::Str, s, start, i);
      continue;
    }

    // Symbols (unicode + multi-char ASCII)
    {
      bool matched = false;
      for (const auto& sym : kSymbols) {
        size_t len = std::strlen(sym.utf8);
        if (src.compare(i, len, sym.utf8) == 0) {
          push(sym.type, sym.text, i, i + len);
          i += len;
          matched = true;
          break;
        }
      }
      if (matched) continue;
    }

    // Identifiers (ASCII or UTF-8 letters such as θ)
    if (isIdentStart(c)) {
      size_t start = i;
      size_t symLen;
      while (i < n && isIdentChar((unsigned char)src[i])) {
        if ((unsigned char)src[i] >= 0x80 && startsWithSymbol(src, i, symLen)) break;
        ++i;
      }
      if (i == start) {
        // A stray non-ASCII byte that is not a known symbol.
        size_t j = i + 1;
        while (j < n && ((unsigned char)src[j] & 0xC0) == 0x80) ++j;
        throw CalcError("unexpected character '" + src.substr(i, j - i) + "'");
      }
      push(Tok::Ident, src.substr(start, i - start), start, i);
      continue;
    }

    switch (c) {
      case '(': ++depth; push(Tok::LParen, "(", i, i + 1); break;
      case ')': depth = depth > 0 ? depth - 1 : 0; push(Tok::RParen, ")", i, i + 1); break;
      case '[': case '{': ++depth; push(Tok::LBrack, "[", i, i + 1); break;
      case ']': case '}': depth = depth > 0 ? depth - 1 : 0; push(Tok::RBrack, "]", i, i + 1); break;
      case ',': push(Tok::Comma, ",", i, i + 1); break;
      case ';': push(Tok::Semi, ";", i, i + 1); break;
      case '+': case '-': case '*': case '/': case '^': case '%': case '!': case '\'':
      case '=': case '<': case '>': case '\\':
        push(Tok::Op, std::string(1, char(c)), i, i + 1);
        break;
      default:
        throw CalcError(std::string("unexpected character '") + char(c) + "'");
    }
    ++i;
  }
  Token end;
  end.type = Tok::End;
  end.pos = end.end = int(n);
  end.line = line;
  out.push_back(end);
  return out;
}

}  // namespace cact
