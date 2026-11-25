#include "session.hpp"

#include <functional>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "lexer.hpp"
#include "parser.hpp"
#include "units.hpp"

namespace cact {

static const double kPi = 3.14159265358979323846;

std::string trim(const std::string& s) {
  size_t a = s.find_first_not_of(" \t\r\n");
  if (a == std::string::npos) return "";
  size_t b = s.find_last_not_of(" \t\r\n");
  return s.substr(a, b - a + 1);
}

static std::string lower(std::string s) {
  for (auto& c : s) c = char(std::tolower((unsigned char)c));
  return s;
}

static bool isGraphName(const std::string& n) {
  return n.size() == 2 && n[0] == 'y' && n[1] >= '1' && n[1] <= '9';
}

// Split on commas that are not inside brackets.
static std::vector<std::string> splitTopLevel(const std::string& s) {
  std::vector<std::string> out;
  int depth = 0;
  std::string cur;
  bool inStr = false;
  for (char c : s) {
    if (c == '"') inStr = !inStr;
    if (!inStr) {
      if (c == '(' || c == '[' || c == '{') ++depth;
      if (c == ')' || c == ']' || c == '}') --depth;
      if (c == ',' && depth == 0) {
        out.push_back(trim(cur));
        cur.clear();
        continue;
      }
    }
    cur += c;
  }
  if (!trim(cur).empty() || !out.empty()) out.push_back(trim(cur));
  return out;
}

Session::Session() {
  in.onPrint = [this](const std::string& s) {
    HistoryItem h;
    h.kind = HistoryItem::Kind::Print;
    h.lines = {s};
    addHistory(h);
  };
  graph.entries[0].text = "";
}

void Session::addHistory(HistoryItem item) {
  history.push_back(std::move(item));
  if (history.size() > maxHistory) history.erase(history.begin(), history.begin() + long(history.size() - maxHistory));
}

void Session::addMessage(const std::string& input, const std::string& msg, HistoryItem::Kind kind) {
  HistoryItem h;
  h.kind = kind;
  h.input = input;
  h.lines = {msg};
  addHistory(h);
}

std::string Session::modeLine() const {
  const auto& d = in.settings.display;
  std::string fmt = d.fmt == DisplaySettings::Fmt::STD ? "STD"
                    : d.fmt == DisplaySettings::Fmt::FIX ? "FIX " + std::to_string(d.digits)
                    : d.fmt == DisplaySettings::Fmt::SCI ? "SCI " + std::to_string(d.digits)
                                                         : "ENG " + std::to_string(d.digits);
  std::string s = std::string(in.settings.angle == AngleMode::DEG ? "DEG" : "RAD") + "  " + (rpnMode ? "RPN" : "ALG") +
                  "  " + fmt;
  if (d.frac) s += "  FRAC";
  if (d.base != 10) s += d.base == 16 ? "  HEX" : d.base == 2 ? "  BIN" : "  OCT";
  return s;
}

bool Session::unitConversion(const std::string& s, Value& value, std::string& text) {
  if (s.find('\n') != std::string::npos) return false;
  static const char* seps[] = {" to ", " in ", " -> ", "->", "→"};
  size_t best = std::string::npos, sepLen = 0;
  for (const char* sep : seps) {
    size_t p = s.rfind(sep);
    if (p != std::string::npos && (best == std::string::npos || p > best)) {
      best = p;
      sepLen = std::string(sep).size();
    }
  }
  if (best == std::string::npos) return false;
  std::string left = trim(s.substr(0, best)), right = trim(s.substr(best + sepLen));
  int c2, u2;
  if (right.empty() || !findUnit(right, c2, u2)) return false;
  std::string unit, expr;
  size_t sp = left.find_last_of(' ');
  int c1 = -1, u1 = -1;
  if (sp != std::string::npos) {
    unit = left.substr(sp + 1);
    expr = trim(left.substr(0, sp));
    if (!findUnit(unit, c1, u1)) unit.clear();
  }
  if (unit.empty()) {
    size_t k = left.size();
    while (k > 0) {
      unsigned char ch = left[k - 1];
      if (std::isalpha(ch) || ch == '/' || ch >= 0x80) --k;
      else break;
    }
    unit = left.substr(k);
    expr = trim(left.substr(0, k));
    if (unit.empty() || !findUnit(unit, c1, u1)) return false;
  }
  if (expr.empty()) return false;
  if (c1 != c2) {
    const auto& cats = unitCategories();
    throw CalcError("can't convert " + cats[c1].name + " (" + unit + ") to " + cats[c2].name + " (" + right + ")");
  }
  Value x = in.evalString(expr);
  double r = convertUnits(x.real("a number to convert"), c1, u1, u2);
  value = Value(r);
  DisplaySettings d = in.settings.display;
  d.frac = false;
  d.base = 10;
  text = formatReal(r, d) + " " + unitCategories()[c2].units[u2].name;
  return true;
}

bool Session::command(const std::string& line, SubmitResult& out) {
  std::string s = trim(line);
  if (s.find('\n') != std::string::npos) return false;
  std::string word = s, arg;
  size_t sp = s.find_first_of(" \t");
  if (sp != std::string::npos) {
    word = s.substr(0, sp);
    arg = trim(s.substr(sp + 1));
  }
  std::string w = lower(word);
  auto& st = in.settings;
  auto& d = st.display;
  auto note = [&](const std::string& msg) { addMessage(s, msg); };
  auto digitsArg = [&](int def) {
    if (arg.empty()) return def;
    for (char c : arg)
      if (!std::isdigit((unsigned char)c)) throw CalcError("expected a number of digits, like: fix 4");
    return std::clamp(std::atoi(arg.c_str()), 0, 15);
  };
  bool noArg = arg.empty();

  try {
    if (noArg && (w == "deg" || w == "degree" || w == "degrees")) {
      st.angle = AngleMode::DEG;
      note("Angles in degrees (DEG)");
      return true;
    }
    if (noArg && (w == "rad" || w == "radian" || w == "radians")) {
      st.angle = AngleMode::RAD;
      note("Angles in radians (RAD)");
      return true;
    }
    if (w == "fix" || w == "sci" || w == "eng") {
      if (!arg.empty() && !std::isdigit((unsigned char)arg[0])) return false;
      d.fmt = w == "fix" ? DisplaySettings::Fmt::FIX : w == "sci" ? DisplaySettings::Fmt::SCI : DisplaySettings::Fmt::ENG;
      d.digits = digitsArg(4);
      note("Number format: " + modeLine());
      return true;
    }
    if (noArg && (w == "std" || w == "float")) {
      d.fmt = DisplaySettings::Fmt::STD;
      note("Number format: standard");
      return true;
    }
    if (w == "frac" && (noArg || arg == "on" || arg == "off")) {
      d.frac = noArg ? !d.frac : arg == "on";
      note(d.frac ? "Fractions ON: results like 0.75 show as 3/4" : "Fractions OFF");
      return true;
    }
    if (noArg && (w == "dec" || w == "hex" || w == "bin" || w == "oct")) {
      d.base = w == "dec" ? 10 : w == "hex" ? 16 : w == "bin" ? 2 : 8;
      note(d.base == 10 ? "Whole numbers in decimal" : "Whole numbers shown in " + std::string(w == "hex" ? "hexadecimal" : w == "bin" ? "binary" : "octal") + " (type dec to go back)");
      return true;
    }
    if (noArg && w == "rpn") {
      rpnMode = true;
      note("RPN mode: type 3 ENTER 4 + (F10 or 'alg' switches back)");
      return true;
    }
    if (noArg && (w == "alg" || w == "algebraic")) {
      rpnMode = false;
      note("Algebraic mode: type 3+4 and press ENTER");
      return true;
    }
    if (noArg && (w == "cls" || w == "clear")) {
      out.action = UiAction::ClearHistory;
      history.clear();
      return true;
    }
    if ((w == "clearvars") || (w == "clear" && arg == "vars")) {
      in.globals.clear();
      note("All variables deleted");
      return true;
    }
    if (noArg && w == "vars") {
      HistoryItem h;
      h.kind = HistoryItem::Kind::Message;
      h.input = s;
      for (const auto& [k, v] : in.globals) {
        std::string one = formatInline(v, d);
        if (u8len(one) > 50) one = std::string(v.isMat() ? (v.vec ? "list of " + std::to_string(v.mat.a.size()) : std::to_string(v.mat.rows) + "×" + std::to_string(v.mat.cols) + " matrix") : "…");
        h.lines.push_back(k + " = " + one);
      }
      for (const auto& [k, f] : in.funcs) {
        std::string params;
        for (size_t i = 0; i < f.params.size(); ++i) params += (i ? ", " : "") + f.params[i];
        h.lines.push_back((f.isProg ? "prog " : "") + k + "(" + params + ")" + (f.isProg ? "" : " = " + (isGraphName(k) ? f.source : f.source.substr(f.source.find('=') == std::string::npos ? 0 : f.source.find('=') + 1))));
      }
      if (h.lines.empty()) h.lines.push_back("No variables yet. Try: a = 5");
      addHistory(h);
      return true;
    }
    if ((w == "del" || w == "delete") && !arg.empty()) {
      bool found = in.globals.erase(arg) + in.funcs.erase(arg) > 0;
      if (isGraphName(arg) && arg[1] - '1' < GraphState::kSlots) graph.entries[arg[1] - '1'].text.clear();
      note(found ? "Deleted " + arg : "Nothing called '" + arg + "'");
      return true;
    }
    if (w == "help" || w == "?") {
      out.action = UiAction::Help;
      out.arg = arg;
      return true;
    }
    if (noArg && (w == "graph" || w == "plot")) {
      out.action = UiAction::Graph;
      return true;
    }
    if (w == "plot" || w == "graph") {
      auto parts = splitTopLevel(arg);
      if (parts.size() > (size_t)GraphState::kSlots) throw CalcError("at most 6 functions at once");
      for (auto& p : parts) {
        std::string e = p;
        for (const char* prefix : {"y=", "y =", "f(x)=", "f(x) ="})
          if (e.rfind(prefix, 0) == 0) e = trim(e.substr(std::string(prefix).size()));
        parseExpression(e);  // syntax check
        p = e;
      }
      for (int k = 0; k < GraphState::kSlots; ++k) {
        if (k < (int)parts.size()) setGraphEntry(k, GraphEntry::Type::Func, parts[k], true);
        else graph.entries[k].enabled = false;
      }
      autoWindow(true);
      out.action = UiAction::Graph;
      return true;
    }
    if (w == "polar" && !arg.empty()) {
      parseExpression(arg);
      setGraphEntry(0, GraphEntry::Type::Polar, arg, true);
      for (int k = 1; k < GraphState::kSlots; ++k) graph.entries[k].enabled = false;
      autoWindow(true);
      out.action = UiAction::Graph;
      return true;
    }
    if ((w == "param" || w == "parametric") && !arg.empty()) {
      auto parts = splitTopLevel(arg);
      if (parts.size() != 2) throw CalcError("param needs two expressions in t: param cos(t), sin(t)");
      parseExpression(parts[0]);
      parseExpression(parts[1]);
      setGraphEntry(0, GraphEntry::Type::Param, parts[0] + ", " + parts[1], true);
      for (int k = 1; k < GraphState::kSlots; ++k) graph.entries[k].enabled = false;
      autoWindow(true);
      out.action = UiAction::Graph;
      return true;
    }
    if (w == "matrix" || w == "mat" || w == "matrices") {
      out.action = UiAction::Matrix;
      out.arg = arg;
      return true;
    }
    if (noArg && (w == "stat" || w == "stats" || w == "data")) {
      out.action = UiAction::Stat;
      return true;
    }
    if ((noArg && (w == "prog" || w == "program" || w == "programs")) || (w == "edit" && !arg.empty())) {
      out.action = UiAction::Prog;
      out.arg = arg;
      return true;
    }
    if (noArg && (w == "solve" || w == "solver")) {
      out.action = UiAction::Solve;
      return true;
    }
    if (noArg && (w == "units" || w == "convert" || w == "conversions")) {
      out.action = UiAction::Units;
      return true;
    }
    if (noArg && (w == "mode" || w == "settings" || w == "setup")) {
      out.action = UiAction::Mode;
      return true;
    }
    if (w == "theme") {
      out.action = UiAction::Theme;
      out.arg = arg;
      return true;
    }
    if (noArg && w == "keypad") {
      out.action = UiAction::Keypad;
      return true;
    }
    if (noArg && (w == "quit" || w == "exit" || w == "bye" || w == "q")) {
      out.action = UiAction::Quit;
      return true;
    }
    if (noArg && (w == "about" || w == "version")) {
      note("Cactulator 1.0 — a programmable graphing calculator for your terminal");
      return true;
    }
  } catch (const CalcError& e) {
    addMessage(s, e.what(), HistoryItem::Kind::Error);
    return true;
  }
  return false;
}

SubmitResult Session::submit(const std::string& raw) {
  SubmitResult r;
  std::string s = trim(raw);
  if (s.empty()) return r;
  if (command(s, r)) return r;

  // "solve x^2 = 2" without parentheses
  if (s.rfind("solve ", 0) == 0 && s.find('(') != 6) s = "solve(" + s.substr(6) + ")";

  // Starting with an operator continues from the last answer: "*2" -> "ans*2"
  if (in.globals.count("ans")) {
    static const char* ops[] = {"+", "*", "/", "^", "²", "³", "⁻¹", "!", "×", "÷"};
    for (const char* op : ops)
      if (s.rfind(op, 0) == 0) {
        s = "ans" + s;
        break;
      }
  }

  size_t placeholder = history.size();
  HistoryItem head;
  head.input = trim(raw);
  addHistory(head);
  placeholder = history.size() - 1;
  size_t before = history.size();

  HistoryItem result;
  try {
    Value v;
    std::string text;
    if (unitConversion(s, v, text)) {
      result.value = v;
      result.lines = {text};
      in.globals["ans"] = v;
    } else {
      ExecResult res = in.execute(s);
      if (res.hasValue) {
        result.value = res.value;
        result.lines = formatLines(res.value, in.settings.display);
      } else {
        result.kind = HistoryItem::Kind::Message;
        result.lines = {res.message};
      }
    }
  } catch (const CalcError& e) {
    result.kind = HistoryItem::Kind::Error;
    result.lines = {e.what()};
  }
  syncGraphFromFunctions();
  if (history.size() == before && placeholder < history.size()) {
    // No printed output in between: put the result next to the input.
    HistoryItem& h = history[placeholder];
    h.kind = result.kind;
    h.lines = result.lines;
    h.value = result.value;
  } else {
    addHistory(result);
  }
  return r;
}

SubmitResult Session::submitRpn(const std::string& raw) {
  SubmitResult r;
  std::string s = trim(raw);
  if (!s.empty() && command(s, r)) return r;
  try {
    rpn.enter(in, s);
    if (!rpn.stack.empty()) in.globals["ans"] = rpn.stack.back();
  } catch (const CalcError& e) {
    addMessage(s, e.what(), HistoryItem::Kind::Error);
  }
  return r;
}

void Session::setGraphEntry(int slot, GraphEntry::Type type, const std::string& text, bool enabled) {
  if (slot < 0 || slot >= GraphState::kSlots) return;
  GraphEntry& e = graph.entries[slot];
  e.type = type;
  e.text = trim(text);
  e.enabled = enabled;
  std::string name = "y" + std::to_string(slot + 1);
  if (type == GraphEntry::Type::Func && !e.text.empty()) {
    try {
      NodeP n = parseExpression(e.text);
      in.defineFunction(name, {"x"}, n, e.text);
    } catch (const CalcError&) {
      in.funcs.erase(name);
    }
  } else {
    in.funcs.erase(name);
  }
}

void Session::syncGraphFromFunctions() {
  for (int k = 0; k < GraphState::kSlots; ++k) {
    std::string name = "y" + std::to_string(k + 1);
    auto it = in.funcs.find(name);
    GraphEntry& e = graph.entries[k];
    if (it != in.funcs.end()) {
      if (e.type != GraphEntry::Type::Func || e.text != it->second.source) {
        e.type = GraphEntry::Type::Func;
        e.text = it->second.source;
        e.enabled = true;
      }
    } else if (e.type == GraphEntry::Type::Func && !e.text.empty()) {
      // Deleted from the calculator.
      try {
        NodeP n = parseExpression(e.text);
        in.defineFunction(name, {"x"}, n, e.text);
      } catch (const CalcError&) {
      }
    }
  }
}

static bool mentionsTrig(const std::string& s) {
  for (const char* f : {"sin", "cos", "tan", "sec", "csc", "cot"})
    if (s.find(f) != std::string::npos) return true;
  return false;
}

void Session::autoWindow(bool fitY) {
  bool deg = in.settings.angle == AngleMode::DEG;
  bool trig = false, onlyCurves = true;
  for (const auto& e : graph.entries) {
    if (!e.enabled || e.text.empty()) continue;
    if (e.type == GraphEntry::Type::Func) {
      onlyCurves = false;
      if (mentionsTrig(e.text)) trig = true;
    }
  }
  graph.tmin = 0;
  graph.tmax = deg ? 360 : 2 * kPi;
  graph.tstep = 0;
  if (trig) {
    graph.xmin = deg ? -360 : -2 * kPi;
    graph.xmax = deg ? 360 : 2 * kPi;
  } else {
    graph.xmin = -10;
    graph.xmax = 10;
  }
  graph.userWindow = false;
  if (!fitY) return;

  std::vector<double> ys, xs;
  for (int k = 0; k < GraphState::kSlots; ++k) {
    const auto& e = graph.entries[k];
    if (!e.enabled || e.text.empty()) continue;
    try {
      if (e.type == GraphEntry::Type::Func) {
        NodeP n = parseExpression(e.text);
        auto f = in.realFunction(n, "x");
        for (int i = 0; i <= 240; ++i) {
          double x = graph.xmin + (graph.xmax - graph.xmin) * i / 240.0;
          double y = f(x);
          if (std::isfinite(y)) ys.push_back(y);
        }
      } else {
        NodeP nx, ny;
        if (e.type == GraphEntry::Type::Param) {
          auto parts = splitTopLevel(e.text);
          if (parts.size() != 2) continue;
          nx = parseExpression(parts[0]);
          ny = parseExpression(parts[1]);
        } else {
          nx = parseExpression(e.text);
        }
        auto fx = in.realFunction(nx, "t");
        std::function<double(double)> fy = ny ? in.realFunction(ny, "t") : nullptr;
        for (int i = 0; i <= 400; ++i) {
          double t = graph.tmin + (graph.tmax - graph.tmin) * i / 400.0;
          double px, py;
          if (e.type == GraphEntry::Type::Param) {
            px = fx(t);
            py = fy(t);
          } else {
            double r = fx(t), th = in.toRadians(t);
            px = r * std::cos(th);
            py = r * std::sin(th);
          }
          if (std::isfinite(px) && std::isfinite(py)) {
            xs.push_back(px);
            ys.push_back(py);
          }
        }
      }
    } catch (const CalcError&) {
    }
  }
  auto robust = [](std::vector<double> v, double& lo, double& hi) {
    if (v.empty()) return false;
    std::sort(v.begin(), v.end());
    size_t n = v.size();
    lo = v[size_t(n * 0.02)];
    hi = v[std::min(n - 1, size_t(n * 0.98))];
    if (v.front() >= lo - 0.25 * (hi - lo)) lo = v.front();
    if (v.back() <= hi + 0.25 * (hi - lo)) hi = v.back();
    double pad = (hi - lo) * 0.1;
    if (hi - lo < 1e-9) pad = std::max(1.0, std::abs(lo) * 0.5);
    lo -= pad;
    hi += pad;
    return true;
  };
  double lo, hi;
  if (robust(ys, lo, hi)) {
    graph.ymin = lo;
    graph.ymax = hi;
  } else {
    graph.ymin = -10;
    graph.ymax = 10;
  }
  if (onlyCurves && robust(xs, lo, hi)) {
    graph.xmin = lo;
    graph.xmax = hi;
  }
}

std::vector<std::string> Session::programNames() const {
  std::vector<std::string> out;
  for (const auto& [k, f] : in.funcs)
    if (!isGraphName(k)) out.push_back(k);
  return out;
}

// ------------------------------------------------------------ persistence

static std::string escapeLine(const std::string& s) {
  std::string o;
  for (char c : s) {
    if (c == '\\') o += "\\\\";
    else if (c == '\n') o += "\\n";
    else if (c == '\t') o += "\\t";
    else o += c;
  }
  return o;
}

static std::string unescapeLine(const std::string& s) {
  std::string o;
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '\\' && i + 1 < s.size()) {
      char n = s[++i];
      o += n == 'n' ? '\n' : n == 't' ? '\t' : n;
    } else {
      o += s[i];
    }
  }
  return o;
}

std::string Session::defaultStatePath() {
  namespace fs = std::filesystem;
  if (const char* p = std::getenv("CACTULATOR_HOME")) return (fs::path(p) / "state.txt").string();
#ifdef _WIN32
  if (const char* a = std::getenv("APPDATA")) return (fs::path(a) / "Cactulator" / "state.txt").string();
#endif
  if (const char* x = std::getenv("XDG_CONFIG_HOME")) return (fs::path(x) / "cactulator" / "state.txt").string();
  if (const char* h = std::getenv("HOME")) return (fs::path(h) / ".config" / "cactulator" / "state.txt").string();
  if (const char* u = std::getenv("USERPROFILE")) return (fs::path(u) / ".cactulator" / "state.txt").string();
  return "cactulator-state.txt";
}

bool Session::save(const std::string& path) const {
  namespace fs = std::filesystem;
  std::error_code ec;
  fs::create_directories(fs::path(path).parent_path(), ec);
  std::string tmp = path + ".tmp";
  {
    std::ofstream f(tmp, std::ios::binary);
    if (!f) return false;
    const auto& st = in.settings;
    f << "# Cactulator state v1 - edit with care\n";
    f << "set angle " << (st.angle == AngleMode::DEG ? "DEG" : "RAD") << "\n";
    f << "set fmt " << int(st.display.fmt) << " " << st.display.digits << "\n";
    f << "set frac " << (st.display.frac ? 1 : 0) << "\n";
    f << "set base " << st.display.base << "\n";
    f << "set rpn " << (rpnMode ? 1 : 0) << "\n";
    for (const auto& [k, v] : prefs) f << "pref " << k << " " << escapeLine(v) << "\n";
    for (const auto& [k, v] : in.globals) f << "var " << k << " = " << serialize(v) << "\n";
    for (const auto& [k, fn] : in.funcs) {
      if (isGraphName(k)) continue;
      if (fn.isProg) f << "prog " << escapeLine(fn.source) << "\n";
      else f << "func " << escapeLine(fn.source) << "\n";
    }
    const auto& g = graph;
    for (int k = 0; k < GraphState::kSlots; ++k) {
      const auto& e = g.entries[k];
      f << "graph " << k << " " << int(e.type) << " " << (e.enabled ? 1 : 0) << " " << escapeLine(e.text) << "\n";
    }
    f << "window " << serializeReal(g.xmin) << " " << serializeReal(g.xmax) << " " << serializeReal(g.ymin) << " "
      << serializeReal(g.ymax) << " " << serializeReal(g.tmin) << " " << serializeReal(g.tmax) << " "
      << serializeReal(g.tstep) << " " << (g.grid ? 1 : 0) << " " << (g.userWindow ? 1 : 0) << "\n";
    for (const auto& v : rpn.stack) f << "stack " << serialize(v) << "\n";
    size_t start = history.size() > 200 ? history.size() - 200 : 0;
    for (size_t i = start; i < history.size(); ++i) {
      const auto& h = history[i];
      std::string lines;
      for (size_t j = 0; j < h.lines.size(); ++j) lines += (j ? "\n" : "") + h.lines[j];
      f << "hist " << int(h.kind) << "\t" << escapeLine(h.input) << "\t" << escapeLine(lines) << "\n";
    }
    if (!f) return false;
  }
  fs::rename(tmp, path, ec);
  if (ec) {
    fs::remove(path, ec);
    fs::rename(tmp, path, ec);
  }
  return !ec;
}

bool Session::load(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return false;
  std::string line;
  auto& st = in.settings;
  history.clear();
  while (std::getline(f, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line.empty() || line[0] == '#') continue;
    size_t sp = line.find(' ');
    std::string key = line.substr(0, sp), rest = sp == std::string::npos ? "" : line.substr(sp + 1);
    try {
      if (key == "set") {
        std::istringstream is(rest);
        std::string what;
        is >> what;
        if (what == "angle") {
          std::string v;
          is >> v;
          st.angle = v == "RAD" ? AngleMode::RAD : AngleMode::DEG;
        } else if (what == "fmt") {
          int fmt = 0, digits = 4;
          is >> fmt >> digits;
          st.display.fmt = DisplaySettings::Fmt(std::clamp(fmt, 0, 3));
          st.display.digits = std::clamp(digits, 0, 15);
        } else if (what == "frac") {
          int v = 0;
          is >> v;
          st.display.frac = v != 0;
        } else if (what == "base") {
          int v = 10;
          is >> v;
          st.display.base = (v == 16 || v == 2 || v == 8) ? v : 10;
        } else if (what == "rpn") {
          int v = 0;
          is >> v;
          rpnMode = v != 0;
        }
      } else if (key == "pref") {
        size_t s2 = rest.find(' ');
        if (s2 != std::string::npos) prefs[rest.substr(0, s2)] = unescapeLine(rest.substr(s2 + 1));
      } else if (key == "var") {
        size_t eq = rest.find(" = ");
        if (eq == std::string::npos) continue;
        std::string name = rest.substr(0, eq);
        in.globals[name] = in.evalString(rest.substr(eq + 3));
      } else if (key == "func" || key == "prog") {
        in.execute(unescapeLine(rest));
      } else if (key == "graph") {
        std::istringstream is(rest);
        int slot, type, enabled;
        is >> slot >> type >> enabled;
        std::string text;
        std::getline(is, text);
        if (!text.empty() && text[0] == ' ') text.erase(0, 1);
        if (slot >= 0 && slot < GraphState::kSlots)
          setGraphEntry(slot, GraphEntry::Type(std::clamp(type, 0, 2)), unescapeLine(text), enabled != 0);
      } else if (key == "window") {
        std::istringstream is(rest);
        std::string a[7];
        int grid = 0, user = 0;
        for (auto& x : a) is >> x;
        is >> grid >> user;
        auto num = [](const std::string& t, double def) {
          char* end = nullptr;
          double v = std::strtod(t.c_str(), &end);
          return (end && *end == 0 && std::isfinite(v)) ? v : def;
        };
        graph.xmin = num(a[0], -10);
        graph.xmax = num(a[1], 10);
        graph.ymin = num(a[2], -10);
        graph.ymax = num(a[3], 10);
        graph.tmin = num(a[4], 0);
        graph.tmax = num(a[5], 360);
        graph.tstep = num(a[6], 0);
        graph.grid = grid != 0;
        graph.userWindow = user != 0;
        if (!(graph.xmin < graph.xmax)) graph.xmin = -10, graph.xmax = 10;
        if (!(graph.ymin < graph.ymax)) graph.ymin = -10, graph.ymax = 10;
      } else if (key == "stack") {
        rpn.stack.push_back(in.evalString(rest));
      } else if (key == "hist") {
        size_t t1 = rest.find('\t');
        size_t t2 = t1 == std::string::npos ? t1 : rest.find('\t', t1 + 1);
        if (t2 == std::string::npos) continue;
        HistoryItem h;
        h.kind = HistoryItem::Kind(std::clamp(std::atoi(rest.substr(0, t1).c_str()), 0, 3));
        h.input = unescapeLine(rest.substr(t1 + 1, t2 - t1 - 1));
        std::string lines = unescapeLine(rest.substr(t2 + 1));
        std::istringstream ls(lines);
        std::string l;
        while (std::getline(ls, l)) h.lines.push_back(l);
        history.push_back(h);
      }
    } catch (const CalcError&) {
      // Skip anything that no longer parses.
    }
  }
  return true;
}

void Session::installExamples() {
  static const char* examples[] = {
      R"(prog collatz(n)
  # How many steps does the Collatz sequence
  # starting at n take to reach 1?
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
end)",
      R"(prog quadratic(a, b, c)
  # Solve a·x² + b·x + c = 0 with the formula
  d = b^2 - 4*a*c
  print "discriminant:", d
  x1 = (-b + sqrt(d)) / (2*a)
  x2 = (-b - sqrt(d)) / (2*a)
  return [x1, x2]
end)",
      R"(prog fib(n)
  # The first n Fibonacci numbers, as a list
  out = []
  a = 0
  b = 1
  for k = 1 to n do
    out[k] = a
    t = a + b
    a = b
    b = t
  end
  return out
end)",
      R"(prog primes(n)
  # All prime numbers up to n
  out = []
  count = 0
  for k = 2 to n do
    if isprime(k) then
      count = count + 1
      out[count] = k
    end
  end
  return out
end)",
      R"(prog guess()
  # A tiny number-guessing game (uses input)
  secret = randint(1, 100)
  tries = 0
  repeat
    g = input("Guess a number from 1 to 100:")
    tries = tries + 1
    if g < secret then print g, "is too low" end
    if g > secret then print g, "is too high" end
  until g == secret
  print "You got it in", tries, "tries!"
  return tries
end)",
      R"(prog compound(p, rate, years)
  # Savings: principal p at rate % per year
  for y = 1 to years do
    p = p * (1 + rate/100)
    print "year", y, ":", round(p, 2)
  end
  return round(p, 2)
end)",
  };
  for (const char* src : examples) {
    try {
      in.execute(src);
    } catch (const CalcError&) {
    }
  }
  try {
    in.execute("f(x) = x^2 - 2x - 3");
  } catch (const CalcError&) {
  }
  setGraphEntry(0, GraphEntry::Type::Func, "sin(x)", true);
  setGraphEntry(1, GraphEntry::Type::Func, "cos(x)", false);
  autoWindow(true);
}

}  // namespace cact
