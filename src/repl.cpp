// Cactulator - plain line-based REPL (used when not on a terminal, or with --repl).
#include "repl.hpp"

#include <iostream>
#include <string>

#include "core/parser.hpp"

namespace cact {

static void printItem(const HistoryItem& h) {
  for (const auto& l : h.lines) {
    switch (h.kind) {
      case HistoryItem::Kind::Error: std::cout << "Error: " << l << "\n"; break;
      case HistoryItem::Kind::Print: std::cout << l << "\n"; break;
      default: std::cout << "  " << l << "\n";
    }
  }
}

int runRepl(Session& s, bool interactive) {
  s.in.onPrint = [](const std::string& line) { std::cout << line << "\n"; };
  s.in.onInput = [](const std::string& prompt, std::string& answer) {
    std::cout << prompt << " " << std::flush;
    return bool(std::getline(std::cin, answer));
  };
  if (interactive)
    std::cout << "Cactulator REPL — type expressions, 'help' for a hint, 'exit' to quit.\n";
  std::string buffer, line;
  for (;;) {
    if (interactive) std::cout << (buffer.empty() ? "> " : "… ") << std::flush;
    if (!std::getline(std::cin, line)) {
      if (buffer.empty()) break;
      line.clear();
      std::string pending = buffer;
      buffer.clear();
      size_t before = s.history.size();
      s.rpnMode ? s.submitRpn(pending) : s.submit(pending);
      for (size_t i = before; i < s.history.size(); ++i)
        if (s.history[i].kind != HistoryItem::Kind::Print) printItem(s.history[i]);
      break;
    }
    if (!buffer.empty()) buffer += "\n";
    buffer += line;
    if (needsMoreInput(buffer)) continue;
    std::string input = buffer;
    buffer.clear();
    std::string t = trim(input);
    if (t.empty()) continue;
    size_t before = s.history.size();
    SubmitResult r = s.rpnMode ? s.submitRpn(input) : s.submit(input);
    if (r.action == UiAction::Quit) break;
    if (r.action == UiAction::Help) {
      std::cout << "  Examples: 2+3*4   sin(30)   x = 5   f(x) = x^2+1   inv([[1,2],[3,4]])\n"
                   "            solve(x^2 = 2, x)   integ(x^2, x, 0, 3)   5 km to mi\n"
                   "  Run without --repl in a terminal for the full graphing interface.\n";
      continue;
    }
    if (r.action != UiAction::None && r.action != UiAction::ClearHistory) {
      std::cout << "  (that opens a screen in the full interface)\n";
      continue;
    }
    for (size_t i = before; i < s.history.size(); ++i)
      if (s.history[i].kind != HistoryItem::Kind::Print) printItem(s.history[i]);
    if (s.rpnMode) {
      DisplaySettings ds = s.in.settings.display;
      size_t n = s.rpn.stack.size();
      for (size_t i = 0; i < n; ++i)
        std::cout << "  " << (n - i) << ": " << formatInline(s.rpn.stack[i], ds) << "\n";
    }
  }
  return 0;
}

}  // namespace cact
