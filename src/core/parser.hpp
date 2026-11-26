// Cactulator - parser for expressions and the program language.
#pragma once

#include <functional>
#include <string>

#include "ast.hpp"
#include "lexer.hpp"

namespace cact {

// Parse a single expression (the whole input must be consumed).
NodeP parseExpression(const std::string& src);

// Parse a sequence of statements (command line or program source).
Block parseStatements(const std::string& src);

// Used by the parser to allow "sin 30" (call without parentheses).
void setFunctionNamePredicate(std::function<bool(const std::string&)> pred);

// True if the source looks like an unfinished block (e.g. "prog f(x)" without
// "end"); used by the REPL to keep reading lines.
bool needsMoreInput(const std::string& src);

}  // namespace cact
