// Cactulator - tokenizer.
#pragma once

#include <string>
#include <vector>

namespace cact {

enum class Tok { Num, Ident, Str, Op, LParen, RParen, LBrack, RBrack, Comma, Semi, Newline, End };

struct Token {
  Tok type = Tok::End;
  std::string text;  // identifier / operator / string contents
  double num = 0.0;
  bool imag = false;  // numeric literal followed by i (4i)
  int pos = 0;        // byte offset
  int end = 0;        // byte offset one past the token
  int line = 1;
};

// Tokenize source text. Unicode operators (× ÷ − √ ² ³ ° ≤ ≥ ≠ → π ∞ ⁻¹) are
// normalized. Throws CalcError on bad input.
std::vector<Token> tokenize(const std::string& src);

bool isKeyword(const std::string& s);

// Column (in characters, 1-based) for a byte offset, for error messages.
int columnOf(const std::string& src, int pos);

}  // namespace cact
