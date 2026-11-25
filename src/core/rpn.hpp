// Cactulator - HP-style RPN stack engine.
#pragma once

#include <string>
#include <vector>

#include "interp.hpp"

namespace cact {

class Rpn {
 public:
  std::vector<Value> stack;  // back() is level 1

  // Process a line of space-separated RPN words: numbers, operators,
  // function names, stack commands (dup drop swap over rot clear), sto name,
  // 'algebraic' and [lists]. Atomic: on error the stack is restored.
  void process(Interp& in, const std::string& line);

  // Single operations used by keys.
  void enter(Interp& in, const std::string& entry);  // push entry, or DUP if empty
  void apply(Interp& in, const std::string& word);    // apply one word
  bool undo();
  void snapshot();

  static bool isStackWord(const std::string& w);

 private:
  std::vector<std::vector<Value>> undo_;
  Value pop(const char* what);
  void need(size_t n, const std::string& what) const;
  void word(Interp& in, const std::string& w, std::vector<std::string>& rest);
};

}  // namespace cact
