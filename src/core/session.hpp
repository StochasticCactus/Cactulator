// Cactulator - a calculator session: commands, history, graph setup, RPN
// stack and saving/loading everything between runs.
#pragma once

#include <array>
#include <map>
#include <string>
#include <vector>

#include "interp.hpp"
#include "rpn.hpp"

namespace cact {

struct HistoryItem {
  enum class Kind { Result, Error, Message, Print };
  Kind kind = Kind::Result;
  std::string input;
  std::vector<std::string> lines;
  Value value;
};

struct GraphEntry {
  enum class Type { Func, Param, Polar };
  Type type = Type::Func;
  std::string text;  // FUNC: expr in x; PARAM: "xexpr, yexpr" in t; POLAR: expr in t (θ)
  bool enabled = true;
};

struct GraphState {
  static const int kSlots = 6;
  std::array<GraphEntry, kSlots> entries;
  double xmin = -10, xmax = 10, ymin = -10, ymax = 10;
  double tmin = 0, tmax = 360, tstep = 0;  // tstep 0 = automatic
  bool grid = false;
  bool userWindow = false;  // the user changed the window by hand
};

enum class UiAction {
  None, Quit, Graph, Matrix, Stat, Prog, Solve, Units, Help, Mode, Theme, Keypad, ClearHistory
};

struct SubmitResult {
  UiAction action = UiAction::None;
  std::string arg;
};

class Session {
 public:
  Session();

  Interp in;
  Rpn rpn;
  bool rpnMode = false;
  GraphState graph;
  std::vector<HistoryItem> history;
  std::map<std::string, std::string> prefs;  // UI preferences (theme, keypad ...)

  // Handle one line typed at the calculator prompt (algebraic mode).
  SubmitResult submit(const std::string& line);
  // Handle an RPN entry line (commands are recognized too).
  SubmitResult submitRpn(const std::string& line);

  // Recognize setting/app commands such as "deg", "fix 4", "graph".
  bool command(const std::string& line, SubmitResult& out);
  // "5 km to mi" style conversions. Returns false if the line isn't one.
  bool unitConversion(const std::string& line, Value& value, std::string& text);

  void addHistory(HistoryItem item);
  void addMessage(const std::string& input, const std::string& msg, HistoryItem::Kind kind = HistoryItem::Kind::Message);

  // Graph helpers
  void setGraphEntry(int slot, GraphEntry::Type type, const std::string& text, bool enabled);
  void syncGraphFromFunctions();
  void autoWindow(bool fitY = true);
  std::string modeLine() const;

  // Programs & functions defined by the user, in name order.
  std::vector<std::string> programNames() const;

  bool load(const std::string& path);
  bool save(const std::string& path) const;
  static std::string defaultStatePath();
  void installExamples();

  size_t maxHistory = 400;
};

std::string trim(const std::string& s);

}  // namespace cact
