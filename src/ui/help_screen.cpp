// Cactulator - HELP app: guides for beginners and a searchable catalog.
#include <memory>
#include <algorithm>
#include <cctype>

#include "app.hpp"
#include "core/format.hpp"

namespace ui {

using cact::u8len;

namespace {

const char* kStart = R"HELP(WELCOME TO CACTULATOR

Type a calculation on the bottom line and press Enter. The answer appears on the right. Nothing to memorize: Tab completes names, and the bottom line explains the function you're typing.

THE BASICS
  2 + 3 * 4            → 14    (multiplication first)
  (2 + 3) * 4          → 20
  2^10                 → 1024  (^ means "to the power of")
  sqrt(2)   or   √2    → 1.41421356237
  5!                   → 120   (factorial)
  1/3 + 1/6            → 0.5   (type  frac  to see fractions)

Press ↑ to bring back an earlier calculation and edit it. Click any result to paste it into the line. Start a line with an operator to keep going from the last answer:  *2  means  ans*2.

ANGLES
  The top line shows DEG or RAD. In DEG mode sin(30) = 0.5.
  Press F9 (or type  rad  /  deg) to switch.

MEMORY
  x = 7                store 7 in x (any name: price, rate, A …)
  3x + 1               → 22
  f(x) = x^2 - 1       make your own function
  f(4)                 → 15
  vars                 list everything you've stored

THE APPS (F-keys on HOME, or the blue keys on the keypad)
  F2 GRAPH    draw functions, trace them, find roots & intersections
  F3 MATRIX   type matrices in a grid: inverse, determinant, eigenvalues
  F4 STAT     enter data, get mean and SD, fit lines and curves
  F5 PRGM     write and run small programs
  F6 SOLVE    solve an equation for any variable; polynomial roots
  F7 UNITS    convert units (on HOME you can also type  5 km to mi)
  F8 MODE     angles, number format, fractions, RPN, colors
  Esc always takes you back to HOME. Everything is saved when you quit.

QUICK RECIPES
  solve(x^2 = 2, x)                 solve an equation
  integ(x^2, x, 0, 3)               area under a curve
  deriv(x^3, x, 2)                  slope at a point
  sum(1/k^2, k, 1, inf)             sum of a series
  inv([[1, 2], [3, 4]])             inverse of a matrix
  solve([[2, 1], [1, 3]], [3, 5])   solve a linear system
  polyroots([1, -3, 2])             roots of a polynomial
  mean([4, 8, 15])   sd([4, 8, 15])   statistics
  normcdf(-1.96, 1.96)   nCr(10, 3)   probability
  100 F to C                        unit conversion
  (2+3i)*(1-i)                      complex numbers
  hex(255)   0b1011 + 1             hex and binary
  _c   _h   _NA   _R                physical constants (see Catalog)

RPN — THE CLASSIC HP WAY
  Press F10 to switch. Numbers go on a stack and operators use them:
  3 Enter 4 +  → 7.   No parentheses needed: (2+3)*4 is 2 Enter 3 + 4 *
  Enter with nothing typed duplicates, ⌫ drops, → swaps, Ctrl-Z undoes.

THE KEYPAD
  When the window is big enough a keypad appears (Ctrl-K shows/hides it).
  Click keys with the mouse. The orange label above a key is its SHIFT
  function: click SHIFT first, then the key.)HELP";

const char* kGraphs = R"HELP(GRAPHING, STEP BY STEP

1. Open GRAPH (F2 on HOME). If nothing is set up yet you'll see the Y= list.
2. Type a function of x — for example  x^2 - 4 — and press Enter.
   Add more in the rows below; each gets its own color.
3. Press F1 (Graph) to draw them.

Even quicker, type on HOME:   plot sin(x), cos(x)

MOVING AROUND
  ← → ↑ ↓     pan
  + / −       zoom in / out (the mouse wheel zooms where you point)
  F2 Zoom     Std (−10…10), Trig, Fit (fit the y range), Square
  F6 Window   type exact x and y ranges
  F7 Grid     dotted grid on/off

TRACING
  F3 Trace, or just click on a curve. ← → move along it, ↑ ↓ switch
  curves, type a number to jump to that x. Coordinates appear at the
  bottom; Enter stores them (x and ans) for use on HOME.

ANALYZING (F4)
  Root     where the curve crosses zero, nearest the cursor
  Min/Max  the lowest / highest point nearby
  Isect    where two curves meet
  dy/dx    slope at the cursor
  ∫dx      area under the curve between two x values (it gets shaded)
  Value    the y value at any x
  Every result is also stored in ans.

TABLE (F5) lists x and y values in columns; ↑ ↓ scroll, F2/F3 set start and step.

OTHER CURVE TYPES (Y= list, F2 Type)
  Parametric   x(t), y(t)   e.g.  cos(3t), sin(2t)
  Polar        r(t)         e.g.  1 + cos(t)    (t is the angle θ)
  From HOME:   param cos(t), sin(t)     or     polar 2sin(4t)

Your functions are also real functions on HOME: after y1 = x^2 you can type y1(3).

Tip: in DEG mode trig graphs use −360…360 automatically; switch to RAD (F9) for the usual −2π…2π.)HELP";

const char* kPrograms = R"HELP(PROGRAMMING

One-line functions, typed on HOME:
  f(x) = x^2 + 1
  area(w, h) = w*h

Programs can do many steps. Open PRGM (F5 on HOME), press F1 New, and edit:

    prog collatz(n)
      steps = 0
      while n != 1 do
        if n % 2 == 0 then
          n = n / 2
        else
          n = 3*n + 1
        end
        steps = steps + 1
      end
      return steps
    end

Then run it from HOME:  collatz(27)  → 111

BUILDING BLOCKS (F3–F7 in the editor insert them)
  x = 5                          store a value
  if cond then … else … end      choose (elif for more cases)
  while cond do … end            repeat while true
  for k = 1 to 10 do … end       count (add  step -1  to count down)
  for v in [3, 5, 8] do … end    go through a list
  repeat … until cond            repeat until true
  return value                   finish and give back a value
  break   continue               leave / skip a loop turn
  print "x is", x                show text on HOME
  n = input("How many?")         ask the user while it runs
  global x                       change a HOME variable
  # note                         a comment for humans

  Comparisons   ==  !=  <  >  <=  >=     (a single = also works in if)
  Logic         and  or  not
  Lists         v = []   v[1] = 5   v[2]   len(v)   sum(v)
  Matrices      A = zeros(3, 3)   A[2, 3] = 7   inv(A)

Variables inside a program are private (local) unless declared global, so programs can't mess up your HOME variables by accident.

Press Esc to stop a program that runs too long.

Installed examples: collatz, quadratic, fib, primes, guess (a game) and compound — open them in PRGM to see how they work.)HELP";

const char* kKeys = R"HELP(KEYBOARD
  Enter            calculate / confirm
  Esc              clear the line, or go back to HOME
  ↑ ↓              earlier entries (HOME) · move around (apps)
  Tab              complete a function name
  PgUp PgDn        scroll
  Ctrl-A Ctrl-E    start / end of line     Ctrl-U  clear the line
  Ctrl-L           clear the HOME screen
  F1 … F8          the soft keys shown at the bottom of the display
  F9               degrees ⇄ radians
  F10              algebraic ⇄ RPN
  Ctrl-T           next color theme
  Ctrl-K           show / hide the keypad
  Ctrl-Z           undo (RPN)
  Ctrl-Q           quit (everything is saved)

TYPING MATH
  ^ power   * times   / divide   % remainder   ! factorial   ' transpose
  2E3 = 2000      pi or π      sqrt(…) or √      5 -> x  stores 5 in x
  [1, 2, 3]  a list         [[1, 2], [3, 4]]  or  [1, 2; 3, 4]  a matrix
  3+4i  complex             0xFF  0b101  0o17   hex / binary / octal
  30°  an angle in degrees even in RAD mode
  Implicit multiplication works: 2x, 3(4+1), 2pi

MOUSE
  Click keypad keys and soft keys · click a result on HOME to reuse it ·
  click a curve to trace it · the wheel scrolls and zooms.
  To select text with the mouse, hold Shift in most terminals.

COMMANDS (type on HOME)
  deg  rad          angle unit
  fix 4  sci 3  eng 2  std     number format
  frac              fractions on/off (0.75 → 3/4)
  hex bin oct dec   show whole numbers in another base
  rpn  alg          entry style
  vars              list variables and functions
  del name          delete a variable or function
  cls               clear the HOME screen
  plot f, g         graph functions       polar r · param x, y
  theme Midnight    pick a theme (Classic, Cactus, Retro LCD, Midnight, Paper)
  keypad            show / hide the keypad
  help name         look up a function
  quit              leave)HELP";

struct Entry {
  std::string name, category, signature, help, example, insert;
};

std::vector<Entry> buildCatalog() {
  std::vector<Entry> v;
  for (const auto& b : cact::builtinList())
    v.push_back({b.name, b.category, b.signature, b.help, b.example, b.name + "("});
  for (const auto& c : cact::constantList())
    v.push_back({c.name, "Constants (" + c.category + ")", c.name, c.help, "", c.name});
  const Entry cmds[] = {
      {"deg", "Commands", "deg", "Use degrees for angles.", "", "deg"},
      {"rad", "Commands", "rad", "Use radians for angles.", "", "rad"},
      {"fix", "Commands", "fix n", "Show n decimals.", "fix 2 → 3.14", "fix 4"},
      {"sci", "Commands", "sci n", "Scientific notation with n decimals.", "sci 3 → 1.235E4", "sci 3"},
      {"eng", "Commands", "eng n", "Engineering notation (powers of 1000).", "eng 2 → 12.35E3", "eng 2"},
      {"std", "Commands", "std", "Standard number format (up to 12 digits).", "", "std"},
      {"frac", "Commands", "frac", "Toggle showing results as fractions.", "0.75 → 3/4", "frac"},
      {"hex", "Commands", "hex · bin · oct · dec", "Show whole-number results in another base.", "", "hex"},
      {"rpn", "Commands", "rpn · alg", "Switch between RPN and algebraic entry (also F10).", "", "rpn"},
      {"vars", "Commands", "vars", "List your variables and functions.", "", "vars"},
      {"del", "Commands", "del name", "Delete a variable or function.", "del x", "del "},
      {"cls", "Commands", "cls", "Clear the HOME screen.", "", "cls"},
      {"plot", "Commands", "plot f, g, …", "Graph one or more functions of x.", "plot sin(x), x/100", "plot "},
      {"polar", "Commands", "polar r", "Graph a polar curve r(t).", "polar 1 + cos(t)", "polar "},
      {"param", "Commands", "param x, y", "Graph a parametric curve x(t), y(t).", "param cos(3t), sin(2t)", "param "},
      {"theme", "Commands", "theme name", "Change the color theme.", "theme Paper", "theme "},
      {"to", "Commands", "value unit to unit", "Convert units right on HOME.", "5 km to mi,  100 F to C", " to "},
  };
  for (const auto& c : cmds) v.push_back(c);
  const Entry ops[] = {
      {"+ - * /", "Operators", "a + b   a − b   a * b   a / b", "The four basic operations.", "7/2 = 3.5", ""},
      {"^", "Operators", "a ^ b", "Power. 2^3 = 8. Right to left: 2^3^2 = 2^9.", "", "^"},
      {"%", "Operators", "a % b", "Remainder after division (modulo).", "17 % 5 = 2", "%"},
      {"!", "Operators", "n!", "Factorial.", "5! = 120", "!"},
      {"'", "Operators", "A'", "Transpose of a matrix.", "", "'"},
      {"->", "Operators", "value -> name", "Store a value (like HP's STO→).", "5 -> x", "->"},
      {"=", "Operators", "name = value", "Store a value, define a function, or write an equation.", "f(x) = x^2", "="},
      {"==", "Operators", "a == b  a != b  a < b …", "Comparisons give 1 (true) or 0 (false).", "3 > 2 = 1", "=="},
      {"°", "Operators", "x°", "An angle in degrees, even in RAD mode.", "sin(30°) = 0.5", "°"},
      {"[ ]", "Operators", "[1, 2, 3]   [[1, 2], [3, 4]]", "Lists and matrices. v[2] picks an item.", "", "["},
  };
  for (const auto& o : ops) v.push_back(o);
  std::stable_sort(v.begin(), v.end(), [](const Entry& a, const Entry& b) { return a.category < b.category; });
  return v;
}

std::string lower(std::string s) {
  for (auto& c : s) c = char(std::tolower((unsigned char)c));
  return s;
}

class HelpScreen : public Screen {
 public:
  explicit HelpScreen(App& a) : Screen(a), catalog_(buildCatalog()) {}

  std::string title() const override { return "HELP"; }

  void enter(const std::string& arg) override {
    if (!arg.empty()) {
      tab_ = 1;
      filter_.set(arg);
      sel_ = 0;
    }
    scroll_ = 0;
  }

  std::vector<SoftKey> softKeys() override {
    return {
        {"Start", [this] { setTab(0); }, tab_ == 0},
        {"Catalog", [this] { setTab(1); }, tab_ == 1},
        {"Graphs", [this] { setTab(2); }, tab_ == 2},
        {"Programs", [this] { setTab(3); }, tab_ == 3},
        {"Keys", [this] { setTab(4); }, tab_ == 4},
        {"", nullptr},
        {"", nullptr},
        {"Home", [this] { app.back(); }},
    };
  }

  void draw(const Rect& r) override {
    rect_ = r;
    if (tab_ == 1) drawCatalog(r);
    else drawText(r, tab_ == 0 ? kStart : tab_ == 2 ? kGraphs : tab_ == 3 ? kPrograms : kKeys);
  }

  bool key(const Event& e) override {
    if (tab_ == 1) return keyCatalog(e);
    int page = std::max(1, rect_.h - 2);
    if (e.is(keys::Up)) scroll_ = std::max(0, scroll_ - 1);
    else if (e.is(keys::Down)) scroll_ += 1;
    else if (e.is(keys::PageUp)) scroll_ = std::max(0, scroll_ - page);
    else if (e.is(keys::PageDown) || e.isChar(' ')) scroll_ += page;
    else if (e.is(keys::Home)) scroll_ = 0;
    else if (e.is(keys::Tab) || e.is(keys::Right)) setTab((tab_ + 1) % 5);
    else if (e.is(keys::Left)) setTab((tab_ + 4) % 5);
    else if (e.type == Event::Type::Text) {
      setTab(1);
      filter_.set(e.text);
    } else return false;
    return true;
  }

  bool mouse(const Event& e, const Rect&) override {
    if (e.mouse == Event::Mouse::WheelUp) {
      if (tab_ == 1) sel_ = std::max(0, sel_ - 1);
      else scroll_ = std::max(0, scroll_ - 3);
      return true;
    }
    if (e.mouse == Event::Mouse::WheelDown) {
      if (tab_ == 1) ++sel_;
      else scroll_ += 3;
      return true;
    }
    if (e.mouse == Event::Mouse::Press && tab_ == 1)
      for (const auto& [y, idx] : rows_)
        if (y == e.my) {
          if (sel_ == idx) use();
          sel_ = idx;
          return true;
        }
    return false;
  }

 private:
  std::vector<Entry> catalog_;
  int tab_ = 0, scroll_ = 0, sel_ = 0, listTop_ = 0;
  LineEdit filter_;
  Rect rect_;
  std::vector<std::pair<int, int>> rows_;

  Term& term() { return app.term; }

  void setTab(int t) {
    tab_ = t;
    scroll_ = 0;
  }

  void drawText(const Rect& r, const char* text) {
    const Theme& t = app.th();
    auto lines = wrapText(text, r.w - 1);
    int maxScroll = std::max(0, int(lines.size()) - r.h);
    scroll_ = std::min(scroll_, maxScroll);
    for (int i = 0; i < r.h && scroll_ + i < (int)lines.size(); ++i) {
      const std::string& l = lines[size_t(scroll_ + i)];
      size_t firstSpace = l.find(' ');
      std::string firstWord = l.substr(0, firstSpace);
      bool heading = !l.empty() && l[0] != ' ' && firstWord.size() >= 2 &&
                     firstWord.find_first_of("abcdefghijklmnopqrstuvwxyz") == std::string::npos &&
                     firstWord.find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ") != std::string::npos;
      int y = r.y + i;
      if (heading) {
        term().put(r.x, y, l, Style(t.lcdAccent, t.lcdBg, true), r.w);
        continue;
      }
      size_t indent = l.find_first_not_of(' ');
      if (indent == std::string::npos || indent < 2) {
        term().put(r.x, y, l, app.lcd(), r.w);
        continue;
      }
      // "  code      explanation": color the code, keep the explanation plain.
      size_t gap = l.find("   ", indent);
      if (gap != std::string::npos) {
        std::string code = l.substr(0, gap);
        drawSpans(term(), r.x, y, r.w, code, highlight(code, t, &app.s.in), Style(t.lcdStrong, t.lcdBg));
        int cx = r.x + int(u8len(code));
        if (cx < r.x + r.w) term().put(cx, y, l.substr(gap), app.lcd(), r.x + r.w - cx);
      } else if (indent >= 4) {
        drawSpans(term(), r.x, y, r.w, l, highlight(l, t, &app.s.in), Style(t.lcdStrong, t.lcdBg));
      } else {
        term().put(r.x, y, l, app.lcd(), r.w);
      }
    }
    if (maxScroll > 0) {
      std::string pos = " " + std::to_string(std::min(100, (scroll_ * 100) / std::max(1, maxScroll))) + "% ↓ ";
      term().put(r.x + r.w - int(u8len(pos)), r.y + r.h - 1, pos, app.lcdSel());
    }
  }

  std::vector<int> filtered() {
    std::vector<int> out;
    std::string f = lower(cact::trim(filter_.text));
    for (size_t i = 0; i < catalog_.size(); ++i) {
      const auto& e = catalog_[i];
      if (f.empty() || lower(e.name).find(f) != std::string::npos || lower(e.help).find(f) != std::string::npos ||
          lower(e.category).find(f) != std::string::npos)
        out.push_back(int(i));
    }
    // Exact / prefix matches first
    std::stable_sort(out.begin(), out.end(), [&](int a, int b) {
      auto rank = [&](int i) {
        std::string n = lower(catalog_[size_t(i)].name);
        return n == f ? 0 : n.rfind(f, 0) == 0 ? 1 : 2;
      };
      return rank(a) < rank(b);
    });
    return out;
  }

  void drawCatalog(const Rect& r) {
    const Theme& t = app.th();
    rows_.clear();
    term().put(r.x, r.y, "Search:", app.lcdAccent());
    term().fill({r.x + 8, r.y, std::min(30, r.w - 8), 1}, app.lcdSel());
    ui::drawEdit(term(), r.x + 8, r.y, std::min(30, r.w - 8), filter_, t, app.lcdSel(), true, {});
    auto items = filtered();
    term().put(r.x + 40, r.y, std::to_string(items.size()) + " matches · Enter inserts on HOME", app.lcdDim(), r.w - 40);
    if (items.empty()) {
      term().put(r.x, r.y + 2, "Nothing found. Try another word, like  matrix  or  angle.", app.lcdDim(), r.w);
      return;
    }
    sel_ = std::clamp(sel_, 0, int(items.size()) - 1);
    int listW = std::min(24, r.w / 3);
    int visible = r.h - 2;
    if (sel_ < listTop_) listTop_ = sel_;
    if (sel_ >= listTop_ + visible) listTop_ = sel_ - visible + 1;
    std::string lastCat;
    for (int i = 0; i < visible && listTop_ + i < (int)items.size(); ++i) {
      const auto& e = catalog_[size_t(items[size_t(listTop_ + i)])];
      bool sel = listTop_ + i == sel_;
      int y = r.y + 2 + i;
      Style st = sel ? app.lcdSel() : app.lcd();
      if (sel) term().fill({r.x, y, listW, 1}, st);
      term().put(r.x + 1, y, e.name, Style(sel ? st.fg : t.synFunc, st.bg, sel), listW - 1);
      rows_.push_back({y, listTop_ + i});
    }
    const auto& e = catalog_[size_t(items[size_t(sel_)])];
    int dx = r.x + listW + 2, dw = r.w - listW - 2;
    if (dw < 16) return;
    term().vrule(dx - 1, r.y + 2, r.h - 2, app.lcdDim(), app.term.ascii ? "|" : "│");
    int y = r.y + 2;
    term().put(dx + 1, y++, e.category, app.lcdDim(), dw - 1);
    ++y;
    drawSpans(term(), dx + 1, y++, dw - 1, e.signature, highlight(e.signature, t, &app.s.in), Style(t.lcdStrong, t.lcdBg, true));
    ++y;
    for (const auto& l : wrapText(e.help, dw - 2))
      if (y < r.y + r.h) term().put(dx + 1, y++, l, app.lcd());
    if (!e.example.empty() && y + 1 < r.y + r.h) {
      ++y;
      term().put(dx + 1, y++, "Example", app.lcdAccent());
      for (const auto& l : wrapText(e.example, dw - 4))
        if (y < r.y + r.h) drawSpans(term(), dx + 3, y++, dw - 4, l, highlight(l, t, &app.s.in), app.lcd());
    }
  }

  void use() {
    auto items = filtered();
    if (items.empty()) return;
    const auto& e = catalog_[size_t(items[size_t(sel_)])];
    if (e.insert.empty()) return;
    calcInsert(app, e.insert);
    app.toast("Inserted " + e.insert + " — finish typing and press Enter");
  }

  bool keyCatalog(const Event& e) {
    if (e.is(keys::Up)) sel_ = std::max(0, sel_ - 1);
    else if (e.is(keys::Down)) ++sel_;
    else if (e.is(keys::PageUp)) sel_ = std::max(0, sel_ - 10);
    else if (e.is(keys::PageDown)) sel_ += 10;
    else if (e.is(keys::Enter)) use();
    else if (e.is(keys::Tab)) setTab(2);
    else if (e.is(keys::Esc)) {
      if (filter_.empty()) return false;
      filter_.clear();
    } else if (filter_.handle(e)) sel_ = 0;
    else return false;
    return true;
  }
};

}  // namespace

std::unique_ptr<Screen> makeHelpScreen(App& a) { return std::make_unique<HelpScreen>(a); }

}  // namespace ui
