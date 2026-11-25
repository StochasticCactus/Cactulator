#include "term.hpp"

#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED 1
#endif
#ifndef NCURSES_WIDECHAR
#define NCURSES_WIDECHAR 1
#endif
#if defined(__has_include)
#if __has_include(<ncursesw/curses.h>)
#include <ncursesw/curses.h>
#else
#include <curses.h>
#endif
#else
#include <curses.h>
#endif

#include <clocale>
#include <cstdlib>
#include <cstring>
#include <cwchar>

#include "core/format.hpp"

namespace ui {

namespace keys {
const int Up = KEY_UP, Down = KEY_DOWN, Left = KEY_LEFT, Right = KEY_RIGHT, Home = KEY_HOME, End = KEY_END,
          PageUp = KEY_PPAGE, PageDown = KEY_NPAGE, Backspace = KEY_BACKSPACE, Delete = KEY_DC, Insert = KEY_IC,
          Enter = KEY_ENTER, BackTab = KEY_BTAB, SLeft = KEY_SLEFT, SRight = KEY_SRIGHT, SUp = KEY_SR, SDown = KEY_SF,
          CtrlLeft = 0x7000001, CtrlRight = 0x7000002;
int F(int n) { return KEY_F(n); }
}  // namespace keys

static std::string toUtf8(wint_t c) {
  std::string out;
  unsigned long u = (unsigned long)c;
  if (u < 0x80) {
    out += char(u);
  } else if (u < 0x800) {
    out += char(0xC0 | (u >> 6));
    out += char(0x80 | (u & 0x3F));
  } else if (u < 0x10000) {
    out += char(0xE0 | (u >> 12));
    out += char(0x80 | ((u >> 6) & 0x3F));
    out += char(0x80 | (u & 0x3F));
  } else {
    out += char(0xF0 | (u >> 18));
    out += char(0x80 | ((u >> 12) & 0x3F));
    out += char(0x80 | ((u >> 6) & 0x3F));
    out += char(0x80 | (u & 0x3F));
  }
  return out;
}

// Make sure multibyte (UTF-8) output works; returns false if it can't.
static bool setupUtf8Locale() {
#ifdef _WIN32
  if (std::setlocale(LC_ALL, ".UTF-8")) return true;
#endif
  std::setlocale(LC_ALL, "");
  if (MB_CUR_MAX > 1) return true;
  for (const char* loc : {"C.UTF-8", "C.utf8", "en_US.UTF-8", "en_US.utf8"})
    if (std::setlocale(LC_CTYPE, loc) && MB_CUR_MAX > 1) return true;
  return false;
}

bool Term::init(bool mouse) {
  if (!setupUtf8Locale()) ascii = true;
  // Make Esc respond quickly (must be set before initscr).
#ifdef _WIN32
  _putenv_s("ESCDELAY", "25");
#else
  setenv("ESCDELAY", "25", 0);
#endif
  if (!initscr()) return false;
  active_ = true;
  raw();
  noecho();
  keypad(stdscr, TRUE);
  nonl();
  curs_set(0);
  if (has_colors()) {
    start_color();
    colors_ = COLORS;
  } else {
    colors_ = 0;
  }
  if (mouse) {
    mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON1_CLICKED | BUTTON4_PRESSED
#ifdef BUTTON5_PRESSED
                  | BUTTON5_PRESSED
#endif
              ,
              nullptr);
    mouseinterval(0);
  }
  return true;
}

void Term::shutdown() {
  if (!active_) return;
  endwin();
  active_ = false;
}

int Term::width() const { return COLS; }
int Term::height() const { return LINES; }

void Term::begin() { erase(); }
void Term::end() { refresh(); }

void Term::resetColors() {
  pairs_.clear();
  nextPair_ = 1;
}

int Term::mapColor(int c) const {
  if (colors_ >= 256 || c < 0) return c;
  // Approximate an xterm-256 color with the basic palette.
  auto rgb = [](int idx, int& r, int& g, int& b) {
    static const int basic[16][3] = {{0, 0, 0},       {205, 0, 0},     {0, 205, 0},     {205, 205, 0},
                                     {0, 0, 238},     {205, 0, 205},   {0, 205, 205},   {229, 229, 229},
                                     {127, 127, 127}, {255, 0, 0},     {0, 255, 0},     {255, 255, 0},
                                     {92, 92, 255},   {255, 0, 255},   {0, 255, 255},   {255, 255, 255}};
    if (idx < 16) {
      r = basic[idx][0];
      g = basic[idx][1];
      b = basic[idx][2];
    } else if (idx < 232) {
      static const int lv[6] = {0, 95, 135, 175, 215, 255};
      idx -= 16;
      r = lv[idx / 36];
      g = lv[(idx / 6) % 6];
      b = lv[idx % 6];
    } else {
      r = g = b = 8 + 10 * (idx - 232);
    }
  };
  int r, g, b;
  rgb(c, r, g, b);
  int n = colors_ >= 16 ? 16 : 8, best = 0;
  long bestD = -1;
  for (int k = 0; k < n; ++k) {
    int r2, g2, b2;
    rgb(k, r2, g2, b2);
    long d = long(r - r2) * (r - r2) + long(g - g2) * (g - g2) + long(b - b2) * (b - b2);
    if (bestD < 0 || d < bestD) {
      bestD = d;
      best = k;
    }
  }
  return best;
}

short Term::pairFor(int fg, int bg) {
  if (colors_ == 0) return 0;
  fg = mapColor(fg);
  bg = mapColor(bg);
  auto key = std::make_pair(fg, bg);
  auto it = pairs_.find(key);
  if (it != pairs_.end()) return it->second;
  if (nextPair_ >= COLOR_PAIRS - 1 || nextPair_ >= 32000) return 0;
  short p = nextPair_++;
  init_pair(p, short(fg), short(bg));
  pairs_[key] = p;
  return p;
}

void Term::apply(const Style& st) {
  attr_t a = A_NORMAL;
  if (st.bold) a |= A_BOLD;
  if (st.underline) a |= A_UNDERLINE;
  short p = pairFor(st.fg, st.bg);
  attr_set(a, p, nullptr);
}

void Term::put(int x, int y, const std::string& text, const Style& st, int maxw) {
  int W = COLS, H = LINES;
  if (y < 0 || y >= H || text.empty()) return;
  std::string s = text;
  if (x < 0) {
    s = cact::u8substr(s, size_t(-x));
    x = 0;
  }
  int room = W - x;
  if (maxw >= 0 && maxw < room) room = maxw;
  if (room <= 0) return;
  if ((int)cact::u8len(s) > room) s = cact::u8substr(s, 0, size_t(room));
  apply(st);
  if (y == H - 1 && x + (int)cact::u8len(s) >= W) {
    // Writing the bottom-right cell can scroll; use insch-free approach.
    std::string head = cact::u8substr(s, 0, cact::u8len(s) - 1);
    mvaddstr(y, x, head.c_str());
    std::string last = cact::u8substr(s, cact::u8len(s) - 1);
    wchar_t wc[4] = {0};
    std::mbstowcs(wc, last.c_str(), 3);
    cchar_t cc;
    attr_t a = A_NORMAL;
    if (st.bold) a |= A_BOLD;
    setcchar(&cc, wc, a, pairFor(st.fg, st.bg), nullptr);
    mvins_wch(y, W - 1, &cc);
    return;
  }
  mvaddstr(y, x, s.c_str());
}

void Term::fill(const Rect& r, const Style& st, const std::string& ch) {
  if (r.w <= 0 || r.h <= 0) return;
  std::string row;
  for (int i = 0; i < r.w; ++i) row += ch;
  for (int yy = r.y; yy < r.y + r.h; ++yy) put(r.x, yy, row, st);
}

void Term::hrule(int x, int y, int w, const Style& st, const std::string& ch) {
  std::string row;
  for (int i = 0; i < w; ++i) row += ch;
  put(x, y, row, st);
}

void Term::vrule(int x, int y, int h, const Style& st, const std::string& ch) {
  for (int i = 0; i < h; ++i) put(x, y + i, ch, st);
}

void Term::frame(const Rect& r, const Style& st, bool rounded) {
  if (r.w < 2 || r.h < 2) return;
  std::string tl = ascii ? "+" : rounded ? "╭" : "┌", tr = ascii ? "+" : rounded ? "╮" : "┐";
  std::string bl = ascii ? "+" : rounded ? "╰" : "└", br = ascii ? "+" : rounded ? "╯" : "┘";
  std::string hz = ascii ? "-" : "─", vt = ascii ? "|" : "│";
  put(r.x, r.y, tl, st);
  hrule(r.x + 1, r.y, r.w - 2, st, hz);
  put(r.x + r.w - 1, r.y, tr, st);
  for (int yy = r.y + 1; yy < r.y + r.h - 1; ++yy) {
    put(r.x, yy, vt, st);
    put(r.x + r.w - 1, yy, vt, st);
  }
  put(r.x, r.y + r.h - 1, bl, st);
  hrule(r.x + 1, r.y + r.h - 1, r.w - 2, st, hz);
  put(r.x + r.w - 1, r.y + r.h - 1, br, st);
}

Event Term::next(int timeoutMs) {
  timeout(timeoutMs);
  wint_t ch = 0;
  int r = wget_wch(stdscr, &ch);
  Event e;
  if (r == ERR) return e;
  if (r == KEY_CODE_YES) {
    if (ch == KEY_RESIZE) {
      e.type = Event::Type::Resize;
      return e;
    }
    if (ch == KEY_MOUSE) {
      MEVENT me;
      if (getmouse(&me) != OK) return e;
      e.type = Event::Type::Mouse;
      e.mx = me.x;
      e.my = me.y;
      if (me.bstate & (BUTTON1_PRESSED | BUTTON1_CLICKED)) e.mouse = Event::Mouse::Press;
      else if (me.bstate & BUTTON1_RELEASED) e.mouse = Event::Mouse::Release;
      else if (me.bstate & BUTTON4_PRESSED) e.mouse = Event::Mouse::WheelUp;
#ifdef BUTTON5_PRESSED
      else if (me.bstate & BUTTON5_PRESSED) e.mouse = Event::Mouse::WheelDown;
#endif
      return e;
    }
    e.type = Event::Type::Key;
    e.key = int(ch);
    if (ch == KEY_ENTER) e.key = keys::Enter;
    const char* nm = keyname(int(ch));
    if (nm) {
      if (std::strcmp(nm, "kLFT5") == 0) e.key = keys::CtrlLeft;
      if (std::strcmp(nm, "kRIT5") == 0) e.key = keys::CtrlRight;
    }
    return e;
  }
  // A character.
  if (ch == 10 || ch == 13) {
    e.type = Event::Type::Key;
    e.key = keys::Enter;
    return e;
  }
  if (ch == 127 || ch == 8) {
    e.type = Event::Type::Key;
    e.key = keys::Backspace;
    return e;
  }
  if (ch < 32) {
    e.type = Event::Type::Key;
    e.key = int(ch);
    return e;
  }
  e.type = Event::Type::Text;
  e.text = toUtf8(ch);
  return e;
}

bool Term::interrupted() {
  timeout(0);
  wint_t ch = 0;
  int r = wget_wch(stdscr, &ch);
  bool hit = (r == OK && (ch == 27 || ch == 3));
  return hit;
}

}  // namespace ui
