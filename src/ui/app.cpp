#include "app.hpp"

#include <algorithm>
#include <exception>
#include <thread>

#include "core/format.hpp"

namespace ui {

using cact::u8len;
using Clock = std::chrono::steady_clock;

static std::vector<KeyDef> buildKeys() {
  using K = KeyDef::Kind;
  auto app = [](const char* l, const char* sl, const char* c, const char* sc) {
    KeyDef k;
    k.label = l;
    k.shiftLabel = sl;
    k.cmd = c;
    k.scmd = sc;
    k.kind = K::App;
    return k;
  };
  auto fn = [](const char* l, const char* sl, const char* alg, const char* rpn, const char* salg, const char* srpn,
               K kind = K::Fn) {
    KeyDef k;
    k.label = l;
    k.shiftLabel = sl;
    k.alg = alg;
    k.rpn = rpn;
    k.salg = salg;
    k.srpn = srpn;
    k.kind = kind;
    return k;
  };
  auto num = [](const char* d) {
    KeyDef k;
    k.label = d;
    k.alg = d;
    k.kind = K::Num;
    return k;
  };
  auto cmd = [](const char* l, const char* sl, const char* c, const char* sc, K kind = K::Fn, int span = 1) {
    KeyDef k;
    k.label = l;
    k.shiftLabel = sl;
    k.cmd = c;
    k.scmd = sc;
    k.kind = kind;
    k.span = span;
    return k;
  };
  std::vector<KeyDef> v = {
      app("GRAPH", "Y=", "app:graph", "app:yedit"),
      app("MATRX", "UNITS", "app:matrix", "app:units"),
      app("STAT", "MODE", "app:stat", "app:mode"),
      app("PRGM", "VARS", "app:prog", "vars"),
      app("SOLVE", "POLY", "app:solve", "app:poly"),
      app("HELP", "THEME", "app:help", "theme"),

      cmd("SHIFT", "", "shift", "shift", K::Shift),
      fn("x²", "√x", "²", "sq", "√(", "sqrt"),
      fn("xʸ", "ʸ√x", "^", "^", "root(", "inv ^"),
      fn("SIN", "ASIN", "sin(", "sin", "asin(", "asin"),
      fn("COS", "ACOS", "cos(", "cos", "acos(", "acos"),
      fn("TAN", "ATAN", "tan(", "tan", "atan(", "atan"),

      fn("(", "[", "(", "", "[", "", K::Op),
      fn(")", "]", ")", "", "]", "", K::Op),
      fn(",", ";", ",", "", ";", "", K::Op),
      fn("LN", "eˣ", "ln(", "ln", "exp(", "exp"),
      fn("LOG", "10ˣ", "log(", "log", "10^(", "10 swap ^"),
      fn("1/x", "x!", "⁻¹", "inv", "!", "fact"),

      num("7"), num("8"), num("9"),
      fn("÷", "MOD", "÷", "/", "%", "%", K::Op),
      fn("x", "t", "x", "", "t", ""),
      cmd("DEL", "CLR", "del", "clear"),

      num("4"), num("5"), num("6"),
      fn("×", "nCr", "×", "*", "nCr(", "nCr", K::Op),
      fn("π", "e", "π", "", "e", ""),
      fn("ANS", "STO→", "ans", "", "→", ""),

      num("1"), num("2"), num("3"),
      fn("−", "|x|", "-", "-", "abs(", "abs", K::Op),
      fn("EE", "i", "E", "", "i", ""),
      fn("±", "°", "-", "neg", "°", ""),

      num("0"),
      fn(".", "=", ".", "", "=", "", K::Num),
      cmd("D↔R", "A↔R", "angle", "rpnmode"),
      fn("+", "<", "+", "+", "<", "", K::Op),
      cmd("ENTER", "ENTER", "enter", "enter", K::Enter, 2),
  };
  return v;
}

App::App(cact::Session& session, const Options& o) : s(session), opt(o) {
  keys_ = buildKeys();
  screens_.push_back(makeCalcScreen(*this));
  screens_.push_back(makeGraphScreen(*this));
  screens_.push_back(makeMatrixScreen(*this));
  screens_.push_back(makeStatScreen(*this));
  screens_.push_back(makeProgScreen(*this));
  screens_.push_back(makeSolveScreen(*this));
  screens_.push_back(makeUnitsScreen(*this));
  screens_.push_back(makeHelpScreen(*this));
  screens_.push_back(makeModeScreen(*this));
  std::string themeName = !opt.theme.empty() ? opt.theme : (s.prefs.count("theme") ? s.prefs["theme"] : "Classic");
  int ti = themeIndex(themeName);
  theme_ = ti < 0 ? 0 : ti;
  if (s.prefs.count("keypad")) keypadMode_ = s.prefs["keypad"];
}

App::~App() { term.shutdown(); }

const Theme& App::th() const { return themes()[size_t(theme_)]; }
Screen& App::screen(ScreenId id) { return *screens_[size_t(id)]; }

Style App::lcd() const { return Style(th().lcdFg, th().lcdBg); }
Style App::lcdDim() const { return Style(th().lcdDim, th().lcdBg); }
Style App::lcdStrong() const { return Style(th().lcdStrong, th().lcdBg, true); }
Style App::lcdAccent() const { return Style(th().lcdAccent, th().lcdBg); }
Style App::lcdError() const { return Style(th().lcdError, th().lcdBg, true); }
Style App::lcdSel() const { return Style(th().lcdSelFg, th().lcdSelBg); }
Style App::body() const { return Style(th().bodyFg, th().bodyBg); }

void App::open(ScreenId id, const std::string& arg) {
  cur_ = id;
  screen(id).enter(arg);
}

void App::back() { open(ScreenId::Calc); }

void App::toast(const std::string& msg, bool error) {
  toast_ = msg;
  toastError_ = error;
  toastTime_ = Clock::now();
}

void App::cycleTheme() {
  theme_ = (theme_ + 1) % int(themes().size());
  term.resetColors();
  toast("Theme: " + th().name + "  (Ctrl-T for the next one)");
}

void App::setTheme(const std::string& name) {
  int i = themeIndex(name);
  if (i < 0) {
    std::string list;
    for (const auto& t : themes()) list += (list.empty() ? "" : ", ") + t.name;
    toast("Themes: " + list, true);
    return;
  }
  theme_ = i;
  term.resetColors();
  toast("Theme: " + th().name);
}

void App::toggleKeypad() {
  bool visible = keypadRect_.w > 0;
  keypadMode_ = visible ? "hide" : "show";
  toast(visible ? "Keypad hidden — Ctrl-K brings it back" : "Keypad shown — Ctrl-K hides it");
}

void App::toggleAngle() {
  auto& a = s.in.settings.angle;
  a = a == cact::AngleMode::DEG ? cact::AngleMode::RAD : cact::AngleMode::DEG;
  toast(a == cact::AngleMode::DEG ? "Angles in degrees" : "Angles in radians");
}

void App::toggleRpn() {
  s.rpnMode = !s.rpnMode;
  toast(s.rpnMode ? "RPN mode: 3 ENTER 4 + — Enter=DUP ⌫=DROP →=SWAP" : "Algebraic mode: type 3+4 and press Enter");
  if (cur_ != ScreenId::Calc) open(ScreenId::Calc);
}

void App::save() {
  s.prefs["theme"] = th().name;
  s.prefs["keypad"] = keypadMode_;
  if (opt.loadState) s.save(opt.statePath);
}

void App::handleAction(const cact::SubmitResult& r) {
  using cact::UiAction;
  switch (r.action) {
    case UiAction::None: break;
    case UiAction::Quit: quit(); break;
    case UiAction::Graph: open(ScreenId::Graph, r.arg); break;
    case UiAction::Matrix: open(ScreenId::Matrix, r.arg); break;
    case UiAction::Stat: open(ScreenId::Stat, r.arg); break;
    case UiAction::Prog: open(ScreenId::Prog, r.arg); break;
    case UiAction::Solve: open(ScreenId::Solve, r.arg); break;
    case UiAction::Units: open(ScreenId::Units, r.arg); break;
    case UiAction::Help: open(ScreenId::Help, r.arg); break;
    case UiAction::Mode: open(ScreenId::Mode, r.arg); break;
    case UiAction::Theme:
      if (r.arg.empty()) cycleTheme();
      else setTheme(r.arg);
      break;
    case UiAction::Keypad: toggleKeypad(); break;
    case UiAction::ClearHistory: toast("Screen cleared"); break;
  }
}

// ------------------------------------------------------------------ layout

void App::layout() {
  int W = term.width(), H = term.height();
  frame_ = {0, 0, W, H};
  keyRects_.clear();
  const int kpWFull = 6 * 5 + 5 + 2;
  int innerW = W - 2, innerH = H - 2;
  bool show = keypadMode_ != "hide";
  bool forced = keypadMode_ == "show";
  keypadRect_ = {};
  keypadCompact_ = false;
  enum { None, Side, Stack } where = None;
  if (show) {
    if (W >= 100 && innerH >= 17) where = Side;
    else if (W >= 92 && innerH >= 9) where = Side, keypadCompact_ = true;
    else if (innerH >= 44) where = Stack;
    else if (innerH >= 34) where = Stack, keypadCompact_ = true;
    else if (forced && W >= 80 && innerH >= 9) where = Side, keypadCompact_ = true;
    else if (forced && innerH >= 20) where = Stack, keypadCompact_ = true;
  }
  int kpH = keypadCompact_ ? 8 : 16;
  if (where == Side) {
    keypadRect_ = {W - 1 - kpWFull, 1 + std::max(0, (innerH - kpH) / 2), kpWFull, kpH};
    lcdBox_ = {1, 1, innerW - kpWFull - 1, innerH};
  } else if (where == Stack) {
    keypadRect_ = {1, H - 1 - kpH, innerW, kpH};
    lcdBox_ = {1, 1, innerW, innerH - kpH - 1};
  } else {
    lcdBox_ = {1, 1, innerW, innerH};
  }
  lcdContent_ = {lcdBox_.x + 1, lcdBox_.y + 1, lcdBox_.w - 2, lcdBox_.h - 2};
  statusRect_ = {lcdContent_.x, lcdContent_.y, lcdContent_.w, 1};
  menuRect_ = {lcdContent_.x, lcdContent_.y + lcdContent_.h - 1, lcdContent_.w, 1};
}

// ------------------------------------------------------------------ drawing

void App::drawFrame() {
  const Theme& t = th();
  term.fill(frame_, body());
  int W = frame_.w;
  std::string brand = term.ascii ? " * CACTULATOR " : " Ψ CACTULATOR ";
  term.put(1, 0, brand, Style(t.brand, t.bodyBg, true));
  std::string sub = "scientific · graphing · programmable";
  if (W > 60) term.put(1 + int(u8len(brand)) + 1, 0, sub, Style(t.bodyDim, t.bodyBg));
  std::string right = th().name;
  if (W > 80) term.put(W - 2 - int(u8len(right)), 0, right, Style(t.bodyDim, t.bodyBg));

  std::string hints = "F9 DEG/RAD · F10 ALG/RPN · Ctrl-T theme · Ctrl-K keypad · Esc back · Ctrl-Q quit";
  if ((int)u8len(hints) > W - 2) hints = "F9 DEG/RAD · F10 RPN · Ctrl-T theme · Ctrl-Q quit";
  if ((int)u8len(hints) > W - 2) hints = "Ctrl-Q quit · Esc back";
  term.put(std::max(1, (W - int(u8len(hints))) / 2), frame_.h - 1, hints, Style(t.bodyDim, t.bodyBg));

  // LCD with its bezel
  term.fill(lcdBox_, Style(t.lcdFg, t.lcdBg));
  term.frame(lcdBox_, Style(t.lcdBorder, t.lcdBg), true);
}

void App::drawStatus() {
  const Theme& t = th();
  Rect r = statusRect_;
  std::string left = s.modeLine();
  if (shift) left = (term.ascii ? "[SHIFT] " : "⇧ SHIFT  ") + left;
  term.put(r.x, r.y, left, Style(shift ? t.shiftLabel : t.lcdDim, t.lcdBg, shift));
  std::string title = screen(cur_).title();
  int titleX = r.x + r.w - int(u8len(title));
  bool showToast = !toast_.empty() && Clock::now() - toastTime_ < std::chrono::milliseconds(3500);
  if (showToast) {
    int x = r.x + int(u8len(left)) + 3;
    int maxw = r.x + r.w - x;
    if (maxw > 10) {
      std::string msg = toast_;
      if ((int)u8len(msg) > maxw) msg = cact::u8substr(msg, 0, size_t(maxw - 1)) + "…";
      term.put(r.x + r.w - int(u8len(msg)), r.y, msg,
               Style(toastError_ ? t.lcdError : t.lcdAccent, t.lcdBg, true));
    }
  } else {
    if (titleX > r.x + (int)u8len(left) + 1) term.put(titleX, r.y, title, Style(t.lcdStrong, t.lcdBg, true));
  }
}

void App::drawSoftKeys(const std::vector<SoftKey>& sk) {
  const Theme& t = th();
  softRects_.clear();
  Rect r = menuRect_;
  int n = int(sk.size());
  if (n == 0 || r.w < 10) return;
  int slots = std::max(n, 6);
  int tabw = (r.w - (slots - 1)) / slots;
  if (tabw < 3) return;
  // Show "F1" prefixes only if every label fits with one (keeps tabs uniform).
  bool withF = true;
  for (int i = 0; i < n; ++i)
    if (!sk[size_t(i)].label.empty() && tabw < (int)u8len(sk[size_t(i)].label) + 3) withF = false;
  for (int i = 0; i < n; ++i) {
    int x = r.x + i * (tabw + 1);
    Rect tr{x, r.y, tabw, 1};
    if (sk[i].label.empty()) continue;
    Style st(sk[i].active ? t.menuBg : t.menuFg, sk[i].active ? t.menuFg : t.menuBg, true);
    term.fill(tr, st);
    std::string fnum = "F" + std::to_string(i + 1);
    std::string label = sk[i].label;
    if ((int)u8len(label) > tabw) label = cact::u8substr(label, 0, size_t(tabw));
    int total = int(u8len(label)) + (withF ? int(fnum.size()) + 1 : 0);
    int lx = x + (tabw - total) / 2;
    if (withF) {
      term.put(lx, r.y, fnum, Style(sk[i].active ? t.menuBg : t.menuKey, st.bg, false));
      lx += int(fnum.size()) + 1;
    }
    term.put(lx, r.y, label, st);
    softRects_.push_back({tr, i});
  }
}

void App::drawKeypad() {
  if (keypadRect_.w <= 0) return;
  const Theme& t = th();
  Rect r = keypadRect_;
  int cols = keyCols_;
  int gap = 1;
  int kw = std::min(9, (r.w - 2 - gap * (cols - 1)) / cols);
  int totalW = kw * cols + gap * (cols - 1);
  int x0 = r.x + (r.w - totalW) / 2;
  int lineH = keypadCompact_ ? 1 : 2;
  int row = 0, col = 0;
  bool pressedRecently = pressedKey_ >= 0 && Clock::now() - pressTime_ < std::chrono::milliseconds(140);
  for (size_t i = 0; i < keys_.size(); ++i) {
    const KeyDef& k = keys_[i];
    int span = k.span;
    int x = x0 + col * (kw + gap);
    int w = kw * span + gap * (span - 1);
    int y = r.y + row * lineH;
    Style cap;
    switch (k.kind) {
      case KeyDef::Kind::Num: cap = Style(t.numFg, t.numBg, true); break;
      case KeyDef::Kind::Op: cap = Style(t.opFg, t.opBg, true); break;
      case KeyDef::Kind::App: cap = Style(t.appFg, t.appBg, true); break;
      case KeyDef::Kind::Enter: cap = Style(t.enterFg, t.enterBg, true); break;
      case KeyDef::Kind::Shift: cap = Style(t.shiftFg, t.shiftBg, true); break;
      default: cap = Style(t.keyFg, t.keyBg, true);
    }
    if (k.kind == KeyDef::Kind::Shift && shift) cap = Style(t.shiftBg, t.shiftFg, true);
    if (pressedRecently && pressedKey_ == int(i)) cap = Style(t.bodyBg, t.pressBg, true);
    std::string label = k.label;
    if (keypadCompact_ && shift && !k.shiftLabel.empty() && k.kind != KeyDef::Kind::Shift) {
      label = k.shiftLabel;
      cap.fg = t.shiftLabel;
    }
    int keyY = keypadCompact_ ? y : y + 1;
    if (!keypadCompact_ && !k.shiftLabel.empty() && k.kind != KeyDef::Kind::Shift && k.kind != KeyDef::Kind::Enter) {
      std::string sl = k.shiftLabel;
      if ((int)u8len(sl) > w) sl = cact::u8substr(sl, 0, size_t(w));
      term.put(x + (w - int(u8len(sl)) + 1) / 2, y, sl, Style(t.shiftLabel, t.bodyBg, shift));
    }
    term.fill({x, keyY, w, 1}, cap);
    if ((int)u8len(label) > w) label = cact::u8substr(label, 0, size_t(w));
    term.put(x + (w - int(u8len(label)) + 1) / 2, keyY, label, cap);
    keyRects_.push_back({Rect{x, keyY, w, 1}, int(i)});
    col += span;
    if (col >= cols) {
      col = 0;
      ++row;
    }
  }
}

void App::drawModal() {
  if (!modal_.on) return;
  const Theme& t = th();
  Rect c = lcdContent_;
  int w = std::min(c.w - 2, std::max(40, int(u8len(modal_.title)) + 4));
  std::vector<std::string> help = wrapText(modal_.help, w - 4);
  if (modal_.help.empty()) help.clear();
  int h = 4 + int(help.size()) + (modal_.yesNo ? 0 : 1);
  Rect box{c.x + (c.w - w) / 2, c.y + std::max(1, (c.h - h) / 2), w, h};
  Style bg(t.lcdFg, t.lcdBg);
  term.fill(box, bg);
  term.frame(box, Style(t.lcdAccent, t.lcdBg, true), true);
  std::string title = " " + modal_.title + " ";
  if ((int)u8len(title) > w - 4) title = cact::u8substr(title, 0, size_t(w - 4));
  term.put(box.x + 2, box.y, title, Style(t.lcdAccent, t.lcdBg, true));
  int y = box.y + 1;
  for (const auto& l : help) term.put(box.x + 2, y++, l, Style(t.lcdDim, t.lcdBg));
  if (modal_.yesNo) {
    term.put(box.x + 2, y + 1, "[Y] Yes    [N] No", Style(t.lcdStrong, t.lcdBg, true));
    return;
  }
  ++y;
  Rect field{box.x + 2, y, w - 4, 1};
  term.fill(field, Style(t.lcdFg, t.lcdSelBg));
  drawEdit(term, field.x, field.y, field.w, modal_.edit, th(), Style(t.lcdSelFg, t.lcdSelBg), true,
           highlight(modal_.edit.text, th(), &s.in, long(modal_.edit.cur)));
  term.put(box.x + 2, y + 1, "Enter ✓   Esc ✗", Style(t.lcdDim, t.lcdBg));
}

void App::draw() {
  term.begin();
  layout();
  int W = term.width(), H = term.height();
  if (W < 40 || H < 12) {
    term.fill({0, 0, W, H}, Style(15, 0));
    term.put(0, 0, "Cactulator needs a bigger window", Style(15, 0, true));
    term.put(0, 1, "(at least 40×12; now " + std::to_string(W) + "×" + std::to_string(H) + ")", Style(7, 0));
    term.put(0, 2, "Ctrl-Q quits", Style(7, 0));
    term.end();
    return;
  }
  drawFrame();
  drawStatus();
  Screen& sc = screen(cur_);
  Rect content{lcdContent_.x, lcdContent_.y + 1, lcdContent_.w, lcdContent_.h - 2};
  sc.draw(content);
  drawSoftKeys(sc.softKeys());
  drawKeypad();
  drawModal();
  term.end();
}

void App::drawSplash() {
  static const char* art[] = {
      "         \\ \\ | | / /     ",
      "       _.-'''''''''-._    ",
      "    .'  /  /  |  \\  \\  '. ",
      "  \\/  \\/ \\/  \\|/  \\/ \\/  \\/",
      "  |   |   |   |   |   |   |",
      " \\|  \\|  \\|  \\|/  |/  |/  |/",
      "  |   |   |  \\|/  |   |   |",
      " \\|  \\|  \\|  \\|/  |/  |/  |/",
      "   \\   \\  \\   |   /  /   / ",
      "    '.  \\  \\  |  /  /  .'  ",
      "       '-._________.-'     ",
      "       [_____________]     ",
      "        \\___________/      ",
      "    ~~~~~~~~~~~~~~~~~~~~~  ",
  };
  const int n = int(sizeof(art) / sizeof(art[0]));
  term.begin();
  int W = term.width(), H = term.height();
  const Theme& t = th();
  term.fill({0, 0, W, H}, body());
  int artW = 28;
  int total = n + 5;
  int y0 = std::max(0, (H - total) / 2);
  int x0 = std::max(0, (W - artW) / 2);
  bool smallTerm = H < total + 1;
  if (!smallTerm) {
    for (int i = 0; i < n; ++i) {
      int col = i == 0 ? 220 : i <= 9 ? (theme_ == 2 ? 46 : 71) : i <= 12 ? 173 : 137;
      if (term.colors() < 256) col = i == 0 ? 3 : i <= 9 ? 2 : 1;
      term.put(x0, y0 + i, art[i], Style(col, t.bodyBg, i <= 9));
    }
  }
  int ty = smallTerm ? H / 2 - 1 : y0 + n + 1;
  std::string title = "C A C T U L A T O R";
  term.put((W - int(u8len(title))) / 2, ty, title, Style(t.brand, t.bodyBg, true));
  std::string sub = "a programmable graphing calculator";
  term.put((W - int(u8len(sub))) / 2, ty + 1, sub, Style(t.bodyFg, t.bodyBg));
  std::string hint = "press any key";
  term.put((W - int(u8len(hint))) / 2, ty + 3, hint, Style(t.bodyDim, t.bodyBg));
  term.end();
  term.next(1600);
}

// ------------------------------------------------------------------ input

void App::runKeypadCommand(const std::string& c) {
  if (c == "shift") {
    shift = !shift;
    return;
  }
  if (c == "app:graph") open(ScreenId::Graph);
  else if (c == "app:yedit") open(ScreenId::Graph, "edit");
  else if (c == "app:matrix") open(ScreenId::Matrix);
  else if (c == "app:units") open(ScreenId::Units);
  else if (c == "app:stat") open(ScreenId::Stat);
  else if (c == "app:mode") open(ScreenId::Mode);
  else if (c == "app:prog") open(ScreenId::Prog);
  else if (c == "app:solve") open(ScreenId::Solve);
  else if (c == "app:poly") open(ScreenId::Solve, "poly");
  else if (c == "app:help") open(ScreenId::Help);
  else if (c == "theme") cycleTheme();
  else if (c == "angle") toggleAngle();
  else if (c == "rpnmode") toggleRpn();
  else if (c == "vars") {
    open(ScreenId::Calc);
    handleAction(s.submit("vars"));
  } else {
    Event e;
    e.type = Event::Type::Key;
    e.key = c == "enter" ? keys::Enter : c == "del" ? keys::Backspace : keys::Esc;
    if (modal_.on) return;
    screen(cur_).key(e);
  }
}

void App::pressKey(int index) {
  if (index < 0 || index >= (int)keys_.size()) return;
  pressedKey_ = index;
  pressTime_ = Clock::now();
  const KeyDef& k = keys_[size_t(index)];
  bool sh = shift && k.kind != KeyDef::Kind::Shift;
  if (k.kind != KeyDef::Kind::Shift) shift = false;
  std::string c = sh && !k.scmd.empty() ? k.scmd : k.cmd;
  if (!c.empty()) {
    runKeypadCommand(c);
    return;
  }
  std::string text = sh && !k.salg.empty() ? k.salg : k.alg;
  std::string word = sh && !k.salg.empty() ? k.srpn : k.rpn;
  if (cur_ == ScreenId::Calc && s.rpnMode && !word.empty()) {
    screen(cur_).insert("\x01rpn:" + word);
    return;
  }
  screen(cur_).insert(text);
}

void App::handle(const Event& e) {
  if (e.type == Event::Type::None || e.type == Event::Type::Resize) return;
  if (e.type == Event::Type::Mouse) {
    if (e.mouse == Event::Mouse::Press) {
      for (const auto& [rect, idx] : keyRects_)
        if (rect.contains(e.mx, e.my)) {
          pressKey(idx);
          return;
        }
      for (const auto& [rect, idx] : softRects_)
        if (rect.contains(e.mx, e.my)) {
          auto sk = screen(cur_).softKeys();
          if (idx < (int)sk.size() && sk[size_t(idx)].action) sk[size_t(idx)].action();
          return;
        }
    }
    Rect content{lcdContent_.x, lcdContent_.y + 1, lcdContent_.w, lcdContent_.h - 2};
    screen(cur_).mouse(e, content);
    return;
  }
  if (e.type == Event::Type::Key) {
    int k = e.key;
    if (k == keys::Ctrl('q')) {
      quit();
      return;
    }
    if (k == keys::Ctrl('c')) {
      if (Clock::now() - lastCtrlC_ < std::chrono::seconds(2)) {
        quit();
        return;
      }
      lastCtrlC_ = Clock::now();
      toast("Press Ctrl-C again to quit (Ctrl-Q also quits)");
      return;
    }
    if (k == keys::Ctrl('t')) {
      cycleTheme();
      return;
    }
    if (k == keys::Ctrl('k')) {
      toggleKeypad();
      return;
    }
    if (k == keys::F(9)) {
      toggleAngle();
      return;
    }
    if (k == keys::F(10)) {
      toggleRpn();
      return;
    }
    for (int n = 1; n <= 8; ++n)
      if (k == keys::F(n)) {
        auto sk = screen(cur_).softKeys();
        if (n - 1 < (int)sk.size() && sk[size_t(n - 1)].action) sk[size_t(n - 1)].action();
        return;
      }
  }
  bool handled = screen(cur_).key(e);
  if (!handled && e.type == Event::Type::Key && e.key == keys::Esc && cur_ != ScreenId::Calc) back();
}

bool App::modalLoop() {
  for (;;) {
    draw();
    Event e = term.next(-1);
    if (e.type == Event::Type::Key) {
      if (e.key == keys::Esc || e.key == keys::Ctrl('c')) return false;
      if (modal_.yesNo) {
        if (e.key == keys::Enter) return true;
        continue;
      }
      if (e.key == keys::Enter) return true;
      if (e.key == keys::Ctrl('q')) {
        running_ = false;
        return false;
      }
      modal_.edit.handle(e);
      continue;
    }
    if (e.type == Event::Type::Text) {
      if (modal_.yesNo) {
        if (e.text == "y" || e.text == "Y") return true;
        if (e.text == "n" || e.text == "N") return false;
        continue;
      }
      modal_.edit.handle(e);
      continue;
    }
    if (e.type == Event::Type::Mouse && e.mouse == Event::Mouse::Press) {
      for (const auto& [rect, idx] : keyRects_)
        if (rect.contains(e.mx, e.my)) {
          const KeyDef& k = keys_[size_t(idx)];
          pressedKey_ = idx;
          pressTime_ = Clock::now();
          if (k.kind == KeyDef::Kind::Shift) {
            shift = !shift;
            break;
          }
          bool sh = shift;
          shift = false;
          std::string c = sh && !k.scmd.empty() ? k.scmd : k.cmd;
          if (c == "enter") return true;
          if (c == "del") modal_.edit.backspace();
          else if (c == "clear") modal_.edit.clear();
          else if (c.empty()) modal_.edit.insert(sh && !k.salg.empty() ? k.salg : k.alg);
          break;
        }
    }
  }
}

bool App::ask(const std::string& title, std::string& value, const std::string& help) {
  modal_.on = true;
  modal_.yesNo = false;
  modal_.title = title;
  modal_.help = help;
  modal_.edit.set(value);
  bool ok = modalLoop();
  modal_.on = false;
  if (ok) value = modal_.edit.text;
  return ok;
}

bool App::confirm(const std::string& question) {
  modal_.on = true;
  modal_.yesNo = true;
  modal_.title = question;
  modal_.help = "";
  bool ok = modalLoop();
  modal_.on = false;
  return ok;
}

void App::busy(const std::string& what) {
  toast(what, false);
  draw();
}

int App::run() {
  term.ascii = opt.ascii;
  if (!term.init(opt.mouse)) return 1;
  s.in.settings.display.unicode = !term.ascii;
  s.in.checkInterrupt = [this]() { return term.interrupted(); };
  s.in.onInput = [this](const std::string& prompt, std::string& answer) {
    answer.clear();
    return ask(prompt, answer, "Type a value (a number, list, or expression).");
  };
  if (opt.splash) drawSplash();
  open(ScreenId::Calc);
  while (running_) {
    try {
      draw();
      int timeout = -1;
      auto now = Clock::now();
      if (pressedKey_ >= 0 && now - pressTime_ < std::chrono::milliseconds(150)) timeout = 60;
      else if (!toast_.empty() && now - toastTime_ < std::chrono::milliseconds(3600)) timeout = 400;
      Event e = term.next(timeout);
      handle(e);
    } catch (const cact::CalcError& err) {
      toast(err.what(), true);
    } catch (const std::exception& err) {
      toast(std::string("Internal error: ") + err.what(), true);
    }
  }
  save();
  term.shutdown();
  return 0;
}

int runTui(cact::Session& s, const Options& opt) {
  App app(s, opt);
  return app.run();
}

}  // namespace ui
