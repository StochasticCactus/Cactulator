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

#include "widgets.hpp"

#include <cctype>

#include "core/format.hpp"
#include "core/lexer.hpp"

namespace ui {

using cact::u8len;
using cact::u8substr;

// Split a UTF-8 string into characters.
static std::vector<std::string> chars(const std::string& s) {
  std::vector<std::string> out;
  for (size_t i = 0; i < s.size();) {
    size_t j = i + 1;
    while (j < s.size() && ((unsigned char)s[j] & 0xC0) == 0x80) ++j;
    out.push_back(s.substr(i, j - i));
    i = j;
  }
  return out;
}

static bool isWordChar(const std::string& c) {
  if (c.empty()) return false;
  unsigned char b = c[0];
  if (b >= 0x80) return c != "×" && c != "÷" && c != "−" && c != "√" && c != "²" && c != "³" && c != "°" && c != "→";
  return std::isalnum(b) || b == '_';
}

void LineEdit::set(const std::string& s) {
  text = s;
  cur = u8len(text);
}
void LineEdit::clear() {
  text.clear();
  cur = 0;
}
size_t LineEdit::length() const { return u8len(text); }
std::string LineEdit::beforeCursor() const { return u8substr(text, 0, cur); }
std::string LineEdit::afterCursor() const { return u8substr(text, cur); }

void LineEdit::insert(const std::string& s) {
  text = beforeCursor() + s + afterCursor();
  cur += u8len(s);
}

bool LineEdit::backspace() {
  if (cur == 0) return false;
  text = u8substr(text, 0, cur - 1) + afterCursor();
  --cur;
  return true;
}

bool LineEdit::del() {
  if (cur >= length()) return false;
  text = beforeCursor() + u8substr(text, cur + 1);
  return true;
}

void LineEdit::left() {
  if (cur > 0) --cur;
}
void LineEdit::right() {
  if (cur < length()) ++cur;
}
void LineEdit::home() { cur = 0; }
void LineEdit::end() { cur = length(); }

void LineEdit::wordLeft() {
  auto c = chars(text);
  while (cur > 0 && !isWordChar(c[cur - 1])) --cur;
  while (cur > 0 && isWordChar(c[cur - 1])) --cur;
}

void LineEdit::wordRight() {
  auto c = chars(text);
  while (cur < c.size() && !isWordChar(c[cur])) ++cur;
  while (cur < c.size() && isWordChar(c[cur])) ++cur;
}

bool LineEdit::handle(const Event& e) {
  if (e.type == Event::Type::Text) {
    insert(e.text);
    return true;
  }
  if (e.type != Event::Type::Key) return false;
  int k = e.key;
  if (k == keys::Left) left();
  else if (k == keys::Right) right();
  else if (k == keys::Home || k == keys::Ctrl('a')) home();
  else if (k == keys::End || k == keys::Ctrl('e')) end();
  else if (k == keys::Backspace) backspace();
  else if (k == keys::Delete) del();
  else if (k == keys::CtrlLeft) wordLeft();
  else if (k == keys::CtrlRight) wordRight();
  else if (k == keys::Ctrl('u')) clear();
  else if (k == keys::Ctrl('w')) {
    size_t c = cur;
    wordLeft();
    text = u8substr(text, 0, cur) + u8substr(text, c);
  } else if (k == keys::Ctrl('k')) {
    text = beforeCursor();
  } else return false;
  return true;
}

std::vector<Span> highlight(const std::string& text, const Theme& th, const cact::Interp* in, long cursor) {
  std::vector<Span> spans;
  auto c = chars(text);
  size_t n = c.size();
  std::vector<int> parenAt;  // positions of brackets
  size_t i = 0;
  while (i < n) {
    const std::string& ch = c[i];
    unsigned char b = ch[0];
    if (ch == "#") {
      spans.push_back({i, n - i, th.synComment});
      break;
    }
    if (ch == "\"") {
      size_t j = i + 1;
      while (j < n && c[j] != "\"") ++j;
      if (j < n) ++j;
      spans.push_back({i, j - i, th.synString});
      i = j;
      continue;
    }
    if (std::isdigit(b) || (ch == "." && i + 1 < n && std::isdigit((unsigned char)c[i + 1][0]))) {
      size_t j = i;
      auto digitAt = [&](size_t p) { return p < n && std::isdigit((unsigned char)c[p][0]); };
      if (c[i] == "0" && i + 2 < n && (c[i + 1] == "x" || c[i + 1] == "b" || c[i + 1] == "o") &&
          std::isxdigit((unsigned char)c[i + 2][0])) {
        j = i + 2;
        while (j < n && std::isxdigit((unsigned char)c[j][0])) ++j;
      } else {
        while (j < n && (digitAt(j) || c[j] == ".")) ++j;
        if (j < n && (c[j] == "e" || c[j] == "E")) {
          size_t k = j + 1;
          if (k < n && (c[k] == "-" || c[k] == "+")) ++k;
          if (digitAt(k)) {
            j = k;
            while (digitAt(j)) ++j;
          }
        }
      }
      if (j < n && c[j] == "i" && (j + 1 >= n || !isWordChar(c[j + 1]))) ++j;
      spans.push_back({i, j - i, th.synNum});
      i = j;
      continue;
    }
    if (ch == "π" || ch == "∞") {
      spans.push_back({i, 1, th.synVar});
      ++i;
      continue;
    }
    if (isWordChar(ch) && !std::isdigit(b)) {
      size_t j = i;
      std::string word;
      while (j < n && isWordChar(c[j])) word += c[j++];
      int col = -1;
      bool bold = false;
      if (cact::isKeyword(word)) {
        col = th.synKeyword;
        bold = true;
      } else if (cact::findBuiltin(word) || (in && in->funcs.count(word))) {
        col = th.synFunc;
      } else if (in) {
        cact::Value v;
        if (in->getVar(word, v)) col = th.synVar;
      }
      if (col >= 0) spans.push_back({i, j - i, col, bold});
      i = j;
      continue;
    }
    if (ch == "(" || ch == ")" || ch == "[" || ch == "]" || ch == "{" || ch == "}") {
      parenAt.push_back(int(i));
      spans.push_back({i, 1, th.synParen});
      ++i;
      continue;
    }
    if (b < 0x80 ? std::string("+-*/^%!=<>'\\,;").find(ch) != std::string::npos
                 : (ch == "×" || ch == "÷" || ch == "−" || ch == "√" || ch == "²" || ch == "³" || ch == "°" ||
                    ch == "→" || ch == "≤" || ch == "≥" || ch == "≠")) {
      spans.push_back({i, 1, th.synOp});
    }
    ++i;
  }
  // Highlight the bracket matching the one next to the cursor.
  if (cursor >= 0) {
    auto isOpen = [&](size_t p) { return c[p] == "(" || c[p] == "[" || c[p] == "{"; };
    auto isClose = [&](size_t p) { return c[p] == ")" || c[p] == "]" || c[p] == "}"; };
    long pos = -1;
    if (cursor > 0 && size_t(cursor - 1) < n && (isOpen(cursor - 1) || isClose(cursor - 1))) pos = cursor - 1;
    else if (size_t(cursor) < n && (isOpen(cursor) || isClose(cursor))) pos = cursor;
    if (pos >= 0) {
      int depth = 0;
      long match = -1;
      if (isOpen(size_t(pos))) {
        for (size_t p = size_t(pos); p < n; ++p) {
          if (isOpen(p)) ++depth;
          if (isClose(p) && --depth == 0) {
            match = long(p);
            break;
          }
        }
      } else {
        for (long p = pos; p >= 0; --p) {
          if (isClose(size_t(p))) ++depth;
          if (isOpen(size_t(p)) && --depth == 0) {
            match = p;
            break;
          }
        }
      }
      if (match >= 0) {
        spans.push_back({size_t(pos), 1, th.synMatch, true});
        spans.push_back({size_t(match), 1, th.synMatch, true});
      }
    }
  }
  return spans;
}

void drawSpans(Term& t, int x, int y, int w, const std::string& text, const std::vector<Span>& spans,
               const Style& base, size_t offset) {
  auto c = chars(text);
  std::vector<int> col(c.size(), -1);
  std::vector<bool> bold(c.size(), false);
  for (const auto& s : spans)
    for (size_t k = s.start; k < s.start + s.len && k < c.size(); ++k) {
      col[k] = s.color;
      bold[k] = s.bold;
    }
  int cx = x;
  for (size_t k = offset; k < c.size() && cx < x + w;) {
    size_t j = k;
    std::string run;
    while (j < c.size() && col[j] == col[k] && bold[j] == bold[k] && cx + int(j - k) < x + w) run += c[j++];
    Style st = base;
    if (col[k] >= 0) st.fg = col[k];
    if (bold[k]) st.bold = true;
    t.put(cx, y, run, st);
    cx += int(j - k);
    k = j;
  }
}

void drawEdit(Term& t, int x, int y, int w, const LineEdit& e, const Theme& th, const Style& base, bool focused,
              const std::vector<Span>& spans, const std::string& ghost) {
  if (w <= 0) return;
  size_t len = e.length();
  size_t offset = 0;
  if (e.cur + 1 > size_t(w)) offset = e.cur + 1 - size_t(w);
  if (len > offset + w && e.cur < offset) offset = e.cur;
  drawSpans(t, x, y, w, e.text, spans, base, offset);
  int cx = x + int(e.cur - offset);
  if (!ghost.empty() && e.cur == len) {
    int room = x + w - (cx + 1);
    if (room > 0) t.put(cx + 1, y, u8substr(ghost, 1), Style(th.lcdDim, base.bg), room);
  }
  if (focused && cx < x + w) {
    std::string under = e.cur < len ? u8substr(e.text, e.cur, 1) : (!ghost.empty() ? u8substr(ghost, 0, 1) : " ");
    t.put(cx, y, under, Style(base.bg, th.lcdFg, true));
  }
}

std::vector<std::string> wrapText(const std::string& s, int width) {
  std::vector<std::string> out;
  if (width < 4) width = 4;
  size_t start = 0;
  while (start <= s.size()) {
    size_t nl = s.find('\n', start);
    std::string para = s.substr(start, nl == std::string::npos ? std::string::npos : nl - start);
    // Keep leading indentation for wrapped lines.
    size_t indent = para.find_first_not_of(' ');
    if (indent == std::string::npos) indent = 0;
    std::string pad(indent, ' ');
    if ((int)u8len(para) <= width) {
      out.push_back(para);
    } else {
      std::string line;
      std::string word;
      auto flushWord = [&]() {
        if (word.empty()) return;
        if (!line.empty() && (int)(u8len(line) + 1 + u8len(word)) > width) {
          out.push_back(line);
          line = pad;
        }
        if (!line.empty() && line != pad) line += " ";
        while ((int)u8len(word) > width - (int)indent) {
          if (!line.empty() && line != pad) {
            out.push_back(line);
            line = pad;
          }
          out.push_back(pad + u8substr(word, 0, size_t(width) - indent));
          word = u8substr(word, size_t(width) - indent);
        }
        line += word;
        word.clear();
      };
      line = pad;
      for (size_t k = indent; k < para.size(); ++k) {
        if (para[k] == ' ') flushWord();
        else word += para[k];
      }
      flushWord();
      if (!line.empty() && line != pad) out.push_back(line);
    }
    if (nl == std::string::npos) break;
    start = nl + 1;
  }
  return out;
}

std::string identBeforeCursor(const LineEdit& e, size_t* start) {
  auto c = chars(e.text);
  size_t j = e.cur;
  while (j > 0 && isWordChar(c[j - 1])) --j;
  if (j < e.cur && std::isdigit((unsigned char)c[j][0])) return "";
  if (e.cur < c.size() && isWordChar(c[e.cur])) return "";
  std::string w;
  for (size_t k = j; k < e.cur; ++k) w += c[k];
  if (start) *start = j;
  return w;
}

std::string identAtCursor(const LineEdit& e) {
  auto c = chars(e.text);
  size_t j = e.cur, k = e.cur;
  while (j > 0 && isWordChar(c[j - 1])) --j;
  while (k < c.size() && isWordChar(c[k])) ++k;
  std::string w;
  for (size_t m = j; m < k; ++m) w += c[m];
  return w;
}

std::string enclosingCall(const LineEdit& e) {
  auto c = chars(e.text);
  int depth = 0;
  for (long p = long(e.cur) - 1; p >= 0; --p) {
    if (c[p] == ")" || c[p] == "]") ++depth;
    else if (c[p] == "(" || c[p] == "[") {
      if (depth == 0) {
        if (c[p] == "[") return "";
        long q = p;
        std::string w;
        while (q > 0 && isWordChar(c[q - 1])) w = c[--q] + w;
        return w;
      }
      --depth;
    }
  }
  return "";
}

std::string balanceBrackets(const std::string& s) {
  std::string stack;
  bool inStr = false;
  for (char ch : s) {
    if (ch == '"') inStr = !inStr;
    if (inStr) continue;
    if (ch == '(' || ch == '[' || ch == '{') stack.push_back(ch);
    if ((ch == ')' || ch == ']' || ch == '}') && !stack.empty()) stack.pop_back();
  }
  std::string out = s;
  for (auto it = stack.rbegin(); it != stack.rend(); ++it) out += *it == '(' ? ')' : *it == '[' ? ']' : '}';
  return out;
}

}  // namespace ui
