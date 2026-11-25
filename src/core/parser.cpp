#include "parser.hpp"

#include <set>

namespace cact {

static std::function<bool(const std::string&)> gIsFunction;

void setFunctionNamePredicate(std::function<bool(const std::string&)> pred) { gIsFunction = std::move(pred); }

void collectVars(const NodeP& n, std::vector<std::string>& out) {
  if (!n) return;
  if (n->k == NK::Var) {
    for (const auto& s : out)
      if (s == n->name) return;
    out.push_back(n->name);
    return;
  }
  for (const auto& a : n->args) collectVars(a, out);
  for (const auto& row : n->rows)
    for (const auto& e : row) collectVars(e, out);
}

namespace {

std::string describe(const Token& t) {
  switch (t.type) {
    case Tok::Num: return "number " + t.text;
    case Tok::Ident: return "'" + t.text + "'";
    case Tok::Str: return "text \"" + t.text + "\"";
    case Tok::Op: return "'" + t.text + "'";
    case Tok::LParen: return "'('";
    case Tok::RParen: return "')'";
    case Tok::LBrack: return "'['";
    case Tok::RBrack: return "']'";
    case Tok::Comma: return "','";
    case Tok::Semi: return "';'";
    case Tok::Newline: return "end of line";
    case Tok::End: return "end of input";
  }
  return "?";
}

class Parser {
 public:
  explicit Parser(const std::string& s) : src_(s), t_(tokenize(s)) {}

  NodeP parseWholeExpression() {
    skipNewlines();
    NodeP e = expr();
    skipNewlines();
    if (cur().type != Tok::End) fail("unexpected " + describe(cur()));
    return e;
  }

  Block parseAll() {
    Block b = block({});
    if (cur().type != Tok::End) {
      if (cur().type == Tok::Ident && isKeyword(cur().text))
        fail("'" + cur().text + "' without a matching block start");
      fail("unexpected " + describe(cur()));
    }
    return b;
  }

 private:
  const std::string& src_;
  std::vector<Token> t_;
  size_t p_ = 0;

  const Token& cur() const { return t_[p_]; }
  const Token& peek(size_t k = 1) const { return t_[std::min(p_ + k, t_.size() - 1)]; }
  bool isOp(const char* s) const { return cur().type == Tok::Op && cur().text == s; }
  bool isKw(const char* s) const { return cur().type == Tok::Ident && cur().text == s; }
  bool isKwIn(const std::set<std::string>& s) const { return cur().type == Tok::Ident && s.count(cur().text); }

  [[noreturn]] void fail(const std::string& msg) const {
    const Token& t = cur();
    std::string where;
    bool multiline = src_.find('\n') != std::string::npos;
    if (multiline) where = " (line " + std::to_string(t.line) + ")";
    else if (t.type != Tok::End) where = " (at column " + std::to_string(columnOf(src_, t.pos)) + ")";
    throw CalcError("Syntax error: " + msg + where);
  }

  void expect(Tok type, const char* what) {
    if (cur().type != type) fail(std::string("expected ") + what + " but found " + describe(cur()));
    ++p_;
  }
  void expectKw(const char* kw) {
    if (!isKw(kw)) fail(std::string("expected '") + kw + "' but found " + describe(cur()));
    ++p_;
  }
  std::string expectIdent(const char* what) {
    if (cur().type != Tok::Ident || isKeyword(cur().text))
      fail(std::string("expected ") + what + " but found " + describe(cur()));
    return t_[p_++].text;
  }

  void skipNewlines() {
    while (cur().type == Tok::Newline) ++p_;
  }
  void skipSeparators() {
    while (cur().type == Tok::Newline || cur().type == Tok::Semi) ++p_;
  }

  static NodeP mk(NK k, int pos, std::vector<NodeP> args = {}, std::string name = "") {
    auto n = std::make_shared<Node>();
    n->k = k;
    n->pos = pos;
    n->args = std::move(args);
    n->name = std::move(name);
    return n;
  }

  // ---------------------------------------------------------------- statements

  Block block(const std::set<std::string>& terminators) {
    Block b;
    for (;;) {
      skipSeparators();
      if (cur().type == Tok::End) return b;
      if (isKwIn(terminators)) return b;
      b.push_back(statement());
      if (cur().type == Tok::Newline || cur().type == Tok::Semi || cur().type == Tok::End) continue;
      if (isKwIn(terminators)) continue;
      fail("unexpected " + describe(cur()) + " after a complete statement");
    }
  }

  StmtP mkStmt(SK k, int line) {
    auto s = std::make_shared<Stmt>();
    s->k = k;
    s->line = line;
    return s;
  }

  bool atStatementEnd() const {
    return cur().type == Tok::Newline || cur().type == Tok::Semi || cur().type == Tok::End ||
           (cur().type == Tok::Ident && (cur().text == "end" || cur().text == "else" || cur().text == "elif" ||
                                          cur().text == "elseif" || cur().text == "until"));
  }

  StmtP statement() {
    const Token& first = cur();
    int line = first.line;
    int startPos = first.pos;

    if (isKw("if")) {
      ++p_;
      auto s = mkStmt(SK::If, line);
      NodeP cond = expr();
      if (isKw("then")) ++p_;
      Block body = block({"elif", "elseif", "else", "end"});
      s->branches.push_back({cond, body});
      for (;;) {
        if (isKw("elif") || isKw("elseif") || (isKw("else") && peek().type == Tok::Ident && peek().text == "if")) {
          if (isKw("else")) ++p_;
          ++p_;
          NodeP c = expr();
          if (isKw("then")) ++p_;
          Block bb = block({"elif", "elseif", "else", "end"});
          s->branches.push_back({c, bb});
          continue;
        }
        if (isKw("else")) {
          ++p_;
          s->elseBody = block({"end"});
        }
        break;
      }
      if (!isKw("end")) fail("this 'if' is missing its 'end'");
      ++p_;
      return s;
    }
    if (isKw("while")) {
      ++p_;
      auto s = mkStmt(SK::While, line);
      s->expr = expr();
      if (isKw("do")) ++p_;
      s->body = block({"end"});
      if (!isKw("end")) fail("this 'while' is missing its 'end'");
      ++p_;
      return s;
    }
    if (isKw("for")) {
      ++p_;
      std::string var = expectIdent("a loop variable name");
      if (isKw("in")) {
        ++p_;
        auto s = mkStmt(SK::ForIn, line);
        s->name = var;
        s->expr = orExpr();
        if (isKw("do")) ++p_;
        s->body = block({"end"});
        if (!isKw("end")) fail("this 'for' is missing its 'end'");
        ++p_;
        return s;
      }
      auto s = mkStmt(SK::For, line);
      s->name = var;
      if (!isOp("=")) fail("expected '=' after the loop variable (for i = 1 to 10)");
      ++p_;
      s->from = orExpr();
      expectKw("to");
      s->to = orExpr();
      if (isKw("step")) {
        ++p_;
        s->step = orExpr();
      }
      if (isKw("do")) ++p_;
      s->body = block({"end"});
      if (!isKw("end")) fail("this 'for' is missing its 'end'");
      ++p_;
      return s;
    }
    if (isKw("repeat")) {
      ++p_;
      auto s = mkStmt(SK::Repeat, line);
      s->body = block({"until"});
      expectKw("until");
      s->expr = expr();
      return s;
    }
    if (isKw("prog") || isKw("function")) {
      ++p_;
      auto s = mkStmt(SK::ProgDef, line);
      s->name = expectIdent("a program name");
      if (cur().type == Tok::LParen) {
        ++p_;
        if (cur().type != Tok::RParen) {
          for (;;) {
            s->params.push_back(expectIdent("a parameter name"));
            if (cur().type == Tok::Comma) {
              ++p_;
              continue;
            }
            break;
          }
        }
        expect(Tok::RParen, "')'");
      }
      s->body = block({"end"});
      if (!isKw("end")) fail("program '" + s->name + "' is missing its 'end'");
      s->source = src_.substr(startPos, cur().end - startPos);
      ++p_;
      return s;
    }
    if (isKw("return")) {
      ++p_;
      auto s = mkStmt(SK::Return, line);
      if (!atStatementEnd()) s->expr = expr();
      return s;
    }
    if (isKw("break")) {
      ++p_;
      return mkStmt(SK::Break, line);
    }
    if (isKw("continue")) {
      ++p_;
      return mkStmt(SK::Continue, line);
    }
    if (isKw("print")) {
      ++p_;
      auto s = mkStmt(SK::Print, line);
      if (!atStatementEnd()) {
        for (;;) {
          s->items.push_back(expr());
          if (cur().type == Tok::Comma) {
            ++p_;
            continue;
          }
          break;
        }
      }
      return s;
    }
    if (isKw("global")) {
      ++p_;
      auto s = mkStmt(SK::Global, line);
      for (;;) {
        s->params.push_back(expectIdent("a variable name"));
        if (cur().type == Tok::Comma) {
          ++p_;
          continue;
        }
        break;
      }
      return s;
    }
    if (cur().type == Tok::Ident && isKeyword(cur().text) && cur().text != "not")
      fail("unexpected '" + cur().text + "'");

    NodeP e = expr();
    // expr -> name  (HP-style store)
    if (isOp("->")) {
      ++p_;
      auto s = mkStmt(SK::Assign, line);
      s->name = expectIdent("a variable name after →");
      s->expr = e;
      return s;
    }
    if (e->k == NK::Eq) {
      NodeP lhs = e->args[0], rhs = e->args[1];
      if (lhs->k == NK::Var) {
        auto s = mkStmt(SK::Assign, line);
        s->name = lhs->name;
        s->expr = rhs;
        int endPos = t_[p_ > 0 ? p_ - 1 : 0].end;
        s->source = src_.substr(startPos, endPos - startPos);
        return s;
      }
      if (lhs->k == NK::Call) {
        bool allVars = true;
        for (const auto& a : lhs->args)
          if (a->k != NK::Var) allVars = false;
        if (allVars) {
          auto s = mkStmt(SK::FuncDef, line);
          s->name = lhs->name;
          for (const auto& a : lhs->args) s->params.push_back(a->name);
          s->expr = rhs;
          int endPos = t_[p_ > 0 ? p_ - 1 : 0].end;
          s->source = src_.substr(startPos, endPos - startPos);
          return s;
        }
      }
      if (lhs->k == NK::Index && lhs->args[0]->k == NK::Var) {
        auto s = mkStmt(SK::IndexAssign, line);
        s->name = lhs->args[0]->name;
        s->idx.assign(lhs->args.begin() + 1, lhs->args.end());
        s->expr = rhs;
        return s;
      }
    }
    auto s = mkStmt(SK::Expr, line);
    s->expr = e;
    return s;
  }

  // --------------------------------------------------------------- expressions

  NodeP expr() {
    NodeP l = orExpr();
    if (isOp("=")) {
      int pos = cur().pos;
      ++p_;
      NodeP r = orExpr();
      return mk(NK::Eq, pos, {l, r});
    }
    return l;
  }

  NodeP orExpr() {
    NodeP l = andExpr();
    while (isKw("or") || isOp("or")) {
      int pos = cur().pos;
      ++p_;
      l = mk(NK::Or, pos, {l, andExpr()});
    }
    return l;
  }

  NodeP andExpr() {
    NodeP l = notExpr();
    while (isKw("and") || isOp("and")) {
      int pos = cur().pos;
      ++p_;
      l = mk(NK::And, pos, {l, notExpr()});
    }
    return l;
  }

  NodeP notExpr() {
    if (isKw("not")) {
      int pos = cur().pos;
      ++p_;
      return mk(NK::Not, pos, {notExpr()});
    }
    return compare();
  }

  NodeP compare() {
    NodeP l = additive();
    while (cur().type == Tok::Op &&
           (cur().text == "==" || cur().text == "!=" || cur().text == "<" || cur().text == ">" ||
            cur().text == "<=" || cur().text == ">=")) {
      std::string op = cur().text;
      int pos = cur().pos;
      ++p_;
      l = mk(NK::Compare, pos, {l, additive()}, op);
    }
    return l;
  }

  NodeP additive() {
    NodeP l = multiplicative();
    while (isOp("+") || isOp("-")) {
      std::string op = cur().text;
      int pos = cur().pos;
      ++p_;
      l = mk(NK::Binary, pos, {l, multiplicative()}, op);
    }
    return l;
  }

  bool startsImplicitFactor() const {
    const Token& t = cur();
    if (t.type == Tok::Ident) return !isKeyword(t.text);
    if (t.type == Tok::LParen) return true;
    if (t.type == Tok::Op && t.text == "√") return true;
    return false;
  }

  NodeP multiplicative() {
    NodeP l = unary();
    for (;;) {
      if (isOp("*") || isOp("/") || isOp("%") || isOp("\\")) {
        std::string op = cur().text;
        int pos = cur().pos;
        ++p_;
        l = mk(NK::Binary, pos, {l, unary()}, op);
        continue;
      }
      if (startsImplicitFactor()) {
        int pos = cur().pos;
        l = mk(NK::Binary, pos, {l, power()}, "*");
        continue;
      }
      return l;
    }
  }

  NodeP unary() {
    if (isOp("-")) {
      int pos = cur().pos;
      ++p_;
      return mk(NK::Neg, pos, {unary()});
    }
    if (isOp("+")) {
      ++p_;
      return unary();
    }
    return power();
  }

  NodeP power() {
    NodeP base = postfix();
    if (isOp("^")) {
      int pos = cur().pos;
      ++p_;
      return mk(NK::Binary, pos, {base, unary()}, "^");
    }
    return base;
  }

  NodeP postfix() {
    NodeP n = primary();
    for (;;) {
      if (cur().type == Tok::Op &&
          (cur().text == "!" || cur().text == "'" || cur().text == "²" || cur().text == "³" ||
           cur().text == "°" || cur().text == "⁻¹")) {
        n = mk(NK::Postfix, cur().pos, {n}, cur().text);
        ++p_;
        continue;
      }
      if (cur().type == Tok::LBrack) {
        int pos = cur().pos;
        ++p_;
        std::vector<NodeP> args{n};
        for (;;) {
          args.push_back(expr());
          if (cur().type == Tok::Comma) {
            ++p_;
            continue;
          }
          break;
        }
        expect(Tok::RBrack, "']'");
        n = mk(NK::Index, pos, args);
        continue;
      }
      return n;
    }
  }

  bool startsOperand() const {
    const Token& t = cur();
    if (t.type == Tok::Num) return true;
    if (t.type == Tok::Ident) return !isKeyword(t.text);
    if (t.type == Tok::Op && t.text == "√") return true;
    return false;
  }

  NodeP primary() {
    const Token& t = cur();
    switch (t.type) {
      case Tok::Num: {
        auto n = mk(NK::Num, t.pos);
        n->num = t.imag ? cplx(0.0, t.num) : cplx(t.num, 0.0);
        ++p_;
        return n;
      }
      case Tok::Str: {
        auto n = mk(NK::Str, t.pos);
        n->name = t.text;
        ++p_;
        return n;
      }
      case Tok::Ident: {
        if (isKeyword(t.text)) fail("unexpected '" + t.text + "'");
        std::string name = t.text;
        int pos = t.pos;
        ++p_;
        if (cur().type == Tok::LParen) {
          ++p_;
          std::vector<NodeP> args;
          if (cur().type != Tok::RParen) {
            for (;;) {
              args.push_back(expr());
              if (cur().type == Tok::Comma) {
                ++p_;
                continue;
              }
              break;
            }
          }
          if (cur().type != Tok::RParen) {
            if (cur().type == Tok::End) fail("missing ')' to close " + name + "(");
            fail("expected ',' or ')' in " + name + "(...) but found " + describe(cur()));
          }
          ++p_;
          return mk(NK::Call, pos, args, name);
        }
        if (gIsFunction && gIsFunction(name) && startsOperand()) {
          // sin 30  ->  sin(30)
          return mk(NK::Call, pos, {power()}, name);
        }
        return mk(NK::Var, pos, {}, name);
      }
      case Tok::LParen: {
        ++p_;
        NodeP e = expr();
        if (cur().type != Tok::RParen) {
          if (cur().type == Tok::End) fail("missing ')'");
          fail("expected ')' but found " + describe(cur()));
        }
        ++p_;
        return e;
      }
      case Tok::LBrack: return matrixLiteral();
      case Tok::Op:
        if (t.text == "√") {
          int pos = t.pos;
          ++p_;
          return mk(NK::Call, pos, {postfix()}, "sqrt");
        }
        if (t.text == "*" || t.text == "/" || t.text == "^")
          fail("'" + t.text + "' needs a number on its left");
        fail("unexpected " + describe(t));
      case Tok::End: fail("the expression is incomplete");
      default: fail("unexpected " + describe(t));
    }
  }

  NodeP matrixLiteral() {
    auto n = mk(NK::MatLit, cur().pos);
    ++p_;  // [
    if (cur().type == Tok::RBrack) {
      ++p_;
      return n;
    }
    std::vector<NodeP> row;
    for (;;) {
      row.push_back(expr());
      if (cur().type == Tok::Comma) {
        ++p_;
        continue;
      }
      if (cur().type == Tok::Semi) {
        ++p_;
        n->rows.push_back(std::move(row));
        row.clear();
        continue;
      }
      if (cur().type == Tok::RBrack) {
        ++p_;
        break;
      }
      if (cur().type == Tok::End) fail("missing ']'");
      fail("expected ',' or ']' in a list but found " + describe(cur()) + " (separate items with commas)");
    }
    n->rows.push_back(std::move(row));
    return n;
  }
};

}  // namespace

NodeP parseExpression(const std::string& src) {
  Parser p(src);
  return p.parseWholeExpression();
}

Block parseStatements(const std::string& src) {
  Parser p(src);
  return p.parseAll();
}

bool needsMoreInput(const std::string& src) {
  std::vector<Token> toks;
  try {
    toks = tokenize(src);
  } catch (const CalcError& e) {
    std::string m = e.what();
    return m.find("closing \"") != std::string::npos;
  }
  int blocks = 0, parens = 0;
  for (size_t i = 0; i < toks.size(); ++i) {
    const Token& t = toks[i];
    if (t.type == Tok::LParen || t.type == Tok::LBrack) ++parens;
    if (t.type == Tok::RParen || t.type == Tok::RBrack) --parens;
    if (t.type != Tok::Ident) continue;
    const std::string& w = t.text;
    bool afterElse = i > 0 && toks[i - 1].type == Tok::Ident && toks[i - 1].text == "else";
    if ((w == "if" && !afterElse) || w == "while" || w == "for" || w == "prog" || w == "function" ||
        w == "repeat")
      ++blocks;
    if (w == "end" || w == "until") --blocks;
  }
  return blocks > 0 || parens > 0;
}

}  // namespace cact
