#include "interp.hpp"

#include <functional>
#include <map>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <limits>

#include "lexer.hpp"
#include "linalg.hpp"
#include "parser.hpp"

namespace cact {

static const double kPi = 3.14159265358979323846;
static const double kNaN = std::numeric_limits<double>::quiet_NaN();

// Raised when the user presses Esc during a long computation.
struct Interrupted : CalcError {
  Interrupted() : CalcError("Interrupted") {}
};

// ------------------------------------------------------------ arithmetic

cplx powc(cplx a, cplx b) {
  if (b == cplx(0)) return 1.0;
  if (a.imag() == 0.0 && b.imag() == 0.0) {
    double x = a.real(), y = b.real();
    if (x >= 0 || y == std::floor(y)) return std::pow(x, y);
    // Negative base, fractional exponent: real odd roots (-8)^(1/3) = -2.
    long long p, q;
    if (toFraction(y, p, q, 1000, 1e-12) && q % 2 == 1) {
      double r = std::pow(-x, y);
      return (std::llabs(p) % 2 == 1) ? -r : r;
    }
    return std::pow(cplx(x, 0.0), cplx(y, 0.0));
  }
  if (b.imag() == 0.0 && b.real() == std::floor(b.real()) && std::abs(b.real()) <= 1024) {
    long long n = (long long)b.real();
    bool neg = n < 0;
    n = std::llabs(n);
    cplx result = 1.0, base = a;
    while (n) {
      if (n & 1) result *= base;
      base *= base;
      n >>= 1;
    }
    return neg ? 1.0 / result : result;
  }
  if (a == cplx(0)) return 0.0;
  return std::pow(a, b);
}

static cplx numOp(const std::string& op, cplx a, cplx b) {
  bool real = a.imag() == 0.0 && b.imag() == 0.0;
  double x = a.real(), y = b.real();
  switch (op[0]) {
    case '+': return real ? cplx(x + y) : a + b;
    case '-': return real ? cplx(x - y) : a - b;
    case '*': return real ? cplx(x * y) : a * b;
    case '/':
      if (b == cplx(0)) throw CalcError("division by zero");
      return real ? cplx(x / y) : a / b;
    case '\\':
      if (a == cplx(0)) throw CalcError("division by zero");
      return real ? cplx(y / x) : b / a;
    case '^': return powc(a, b);
    case '%': {
      if (!real) throw CalcError("mod needs real numbers");
      if (y == 0) throw CalcError("mod by zero");
      double r = x - y * std::floor(x / y);
      return r;
    }
  }
  throw CalcError("unknown operator " + op);
}

Value mapNumbers(const Value& v, const std::function<cplx(cplx)>& f) {
  if (v.isNum()) return Value(f(v.num));
  if (v.isMat()) {
    Value out = v;
    for (auto& z : out.mat.a) z = f(z);
    return out;
  }
  throw CalcError(std::string("expected a number, got ") + v.typeName());
}

static std::string dims(const Matrix& m) { return std::to_string(m.rows) + "×" + std::to_string(m.cols); }

static Value elementwise(const std::string& op, const Value& a, const Value& b) {
  const Matrix &A = a.mat, &B = b.mat;
  if (A.a.size() != B.a.size() || (!(a.vec || b.vec) && (A.rows != B.rows || A.cols != B.cols))) {
    if (a.vec && b.vec)
      throw CalcError("the lists have different lengths (" + std::to_string(A.a.size()) + " and " +
                      std::to_string(B.a.size()) + ")");
    throw CalcError("can't combine a " + dims(A) + " and a " + dims(B) + " matrix with '" + op +
                    "' (sizes must match)");
  }
  Value out = a.vec && !b.vec ? b : a;
  for (size_t i = 0; i < A.a.size(); ++i) out.mat.a[i] = numOp(op, A.a[i], B.a[i]);
  return out;
}

static std::string toText(const Value& v) {
  DisplaySettings ds;
  return formatInline(v, ds);
}

Value negate(const Value& a) {
  if (a.isStr()) throw CalcError("can't negate text");
  if (a.isNone()) throw CalcError("missing value");
  return mapNumbers(a, [](cplx z) { return -z; });
}

Value binaryOp(const std::string& op, const Value& a, const Value& b) {
  if (a.isNone() || b.isNone()) throw CalcError("missing value (did a program return nothing?)");
  if (a.isStr() || b.isStr()) {
    if (op == "+") return Value::string(toText(a) + toText(b));
    throw CalcError("can't use '" + op + "' with text");
  }
  if (a.isNum() && b.isNum()) return Value(numOp(op, a.num, b.num));
  if (a.isMat() && b.isNum()) {
    if (op == "^" && !a.vec) {
      if (!a.mat.square()) throw CalcError("only square matrices can be raised to a power");
      double e = b.real("an integer power");
      if (e != std::floor(e)) throw CalcError("matrix powers must be whole numbers");
      return Value::matrix(matrixPower(a.mat, (long long)e));
    }
    cplx s = b.num;
    return mapNumbers(a, [&](cplx z) { return numOp(op, z, s); });
  }
  if (a.isNum() && b.isMat()) {
    if (op == "/" && !b.vec) {
      Matrix inv = inverse(b.mat);
      for (auto& z : inv.a) z *= a.num;
      return Value::matrix(inv);
    }
    if (op == "^" && !b.vec) throw CalcError("can't raise a number to a matrix power");
    cplx s = a.num;
    return mapNumbers(b, [&](cplx z) { return numOp(op, s, z); });
  }
  // matrix (or list) with matrix (or list)
  const Matrix &A = a.mat, &B = b.mat;
  bool bothVec = a.vec && b.vec;
  if (op == "+" || op == "-" || op == "%" || (bothVec && (op == "*" || op == "/" || op == "^")))
    return elementwise(op, a, b);
  if (op == "*") {
    if (A.cols == B.rows) {
      Matrix m = multiply(A, B);
      return Value::matrix(m, b.vec && m.cols == 1);
    }
    if (a.vec && (int)A.a.size() == B.rows) {
      Matrix row(1, int(A.a.size()));
      row.a = A.a;
      Matrix m = multiply(row, B);
      Matrix col(m.cols, 1);
      col.a = m.a;
      return Value::matrix(col, true);
    }
    multiply(A, B);  // throws a helpful error
  }
  if (op == "/") {
    if (!B.square()) throw CalcError("can only divide by a square matrix");
    return Value::matrix(multiply(A, inverse(B)));
  }
  if (op == "\\") {
    Matrix x = solveLinear(A, B);
    return Value::matrix(x, b.vec);
  }
  throw CalcError("can't use '" + op + "' between two matrices");
}

static bool approxEqual(cplx a, cplx b) {
  double s = std::max(std::abs(a), std::abs(b));
  return std::abs(a - b) <= 1e-12 * s;
}

static bool valuesEqual(const Value& a, const Value& b) {
  if (a.isNum() && b.isNum()) return approxEqual(a.num, b.num);
  if (a.isStr() && b.isStr()) return a.str == b.str;
  if (a.isMat() && b.isMat()) {
    if (a.mat.a.size() != b.mat.a.size()) return false;
    if (!(a.vec || b.vec) && (a.mat.rows != b.mat.rows)) return false;
    for (size_t i = 0; i < a.mat.a.size(); ++i)
      if (!approxEqual(a.mat.a[i], b.mat.a[i])) return false;
    return true;
  }
  return false;
}

static Value compareValues(const std::string& op, const Value& a, const Value& b) {
  if (op == "==") return Value(valuesEqual(a, b) ? 1.0 : 0.0);
  if (op == "!=") return Value(valuesEqual(a, b) ? 0.0 : 1.0);
  if (a.isStr() && b.isStr()) {
    int c = a.str.compare(b.str);
    bool r = op == "<" ? c < 0 : op == ">" ? c > 0 : op == "<=" ? c <= 0 : c >= 0;
    return Value(r ? 1.0 : 0.0);
  }
  double x = a.real("a real number to compare"), y = b.real("a real number to compare");
  bool eq = approxEqual(x, y);
  bool r = op == "<" ? (x < y && !eq) : op == ">" ? (x > y && !eq) : op == "<=" ? (x < y || eq) : (x > y || eq);
  return Value(r ? 1.0 : 0.0);
}

static cplx factorial(cplx z) {
  if (z.imag() != 0.0) throw CalcError("factorial needs a real number");
  double x = z.real();
  if (x == std::floor(x)) {
    if (x < 0) throw CalcError("factorial of a negative integer is undefined");
    if (x > 170) return INFINITY;
    double r = 1;
    for (int i = 2; i <= (int)x; ++i) r *= i;
    return r;
  }
  return std::tgamma(x + 1);
}

// ---------------------------------------------------------------- Interp

Interp::Interp() {
  setFunctionNamePredicate([](const std::string& n) {
    const Builtin* b = findBuiltin(n);
    return b && !b->lazy && b->minArgs <= 1 && (b->maxArgs < 0 || b->maxArgs >= 1) && b->minArgs == 1;
  });
}

void Interp::tick() {
  if ((++steps_ & 4095) == 0 && checkInterrupt && checkInterrupt()) throw Interrupted();
}

double Interp::toRadians(double a) const { return settings.angle == AngleMode::DEG ? a * kPi / 180 : a; }
double Interp::fromRadians(double r) const { return settings.angle == AngleMode::DEG ? r * 180 / kPi : r; }
cplx Interp::toRadians(cplx a) const { return settings.angle == AngleMode::DEG ? a * (kPi / 180) : a; }
cplx Interp::fromRadians(cplx r) const { return settings.angle == AngleMode::DEG ? r * (180 / kPi) : r; }

bool Interp::isFunctionName(const std::string& name) const {
  return funcs.count(name) || findBuiltin(name) != nullptr;
}

bool Interp::isReservedName(const std::string& name) const {
  if (isKeyword(name) || findBuiltin(name)) return true;
  if (name == "i") return false;  // allowed as a loop variable
  return findConstant(name) != nullptr;
}

static int editDistance(const std::string& a, const std::string& b) {
  std::vector<int> prev(b.size() + 1), cur(b.size() + 1);
  for (size_t j = 0; j <= b.size(); ++j) prev[j] = int(j);
  for (size_t i = 1; i <= a.size(); ++i) {
    cur[0] = int(i);
    for (size_t j = 1; j <= b.size(); ++j)
      cur[j] = std::min({prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + (a[i - 1] == b[j - 1] ? 0 : 1)});
    std::swap(prev, cur);
  }
  return prev[b.size()];
}

std::vector<std::string> Interp::allNames() const {
  std::vector<std::string> out;
  for (const auto& b : builtinList()) out.push_back(b.name);
  for (const auto& c : constantList()) out.push_back(c.name);
  for (const auto& [k, v] : globals) out.push_back(k);
  for (const auto& [k, v] : funcs) out.push_back(k);
  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
  return out;
}

std::string Interp::suggest(const std::string& name) const {
  if (u8len(name) <= 1) return "";
  std::string best;
  int bestD = 3, bestPrefix = -1;
  for (const auto& n : allNames()) {
    if (n.size() < 2 && name.size() > 2) continue;
    int d = editDistance(name, n);
    int prefix = 0;
    while (prefix < (int)std::min(n.size(), name.size()) && n[prefix] == name[prefix]) ++prefix;
    if (d < bestD || (d == bestD && prefix > bestPrefix)) {
      bestD = d;
      bestPrefix = prefix;
      best = n;
    }
  }
  if (best.empty() || bestD > (name.size() <= 3 ? 1 : 2)) return "";
  return " — did you mean '" + best + "'?";
}

bool Interp::getVar(const std::string& name, Value& out) const {
  for (int i = int(frames_.size()) - 1; i >= 0; --i) {
    auto it = frames_[i].vars.find(name);
    if (it != frames_[i].vars.end()) {
      out = it->second;
      return true;
    }
    if (frames_[i].opaque) break;
  }
  auto g = globals.find(name);
  if (g != globals.end()) {
    out = g->second;
    return true;
  }
  if (name == "i") {
    out = Value(cplx(0.0, 1.0));
    return true;
  }
  if (const Constant* c = findConstant(name)) {
    out = Value(c->value);
    return true;
  }
  return false;
}

Value Interp::lookup(const std::string& name, int) const {
  Value v;
  if (getVar(name, v)) return v;
  if (findBuiltin(name)) throw CalcError("'" + name + "' is a function — use it like " + name + "(…)");
  auto f = funcs.find(name);
  if (f != funcs.end()) {
    std::string params;
    for (size_t k = 0; k < f->second.params.size(); ++k) params += (k ? ", " : "") + f->second.params[k];
    throw CalcError("'" + name + "' is a function — call it like " + name + "(" + params + ")");
  }
  throw CalcError("'" + name + "' is not defined" + suggest(name));
}

void Interp::setVar(const std::string& name, const Value& v) {
  if (isReservedName(name)) {
    if (findBuiltin(name)) throw CalcError("'" + name + "' is a built-in function; pick another name");
    if (isKeyword(name)) throw CalcError("'" + name + "' is a keyword; pick another name");
    throw CalcError("'" + name + "' is a built-in constant; pick another name");
  }
  funcs.erase(name);
  globals[name] = v;
}

void Interp::assign(const std::string& name, const Value& v) {
  if (!frames_.empty()) {
    Frame& f = frames_.back();
    if (!f.globalNames.count(name)) {
      if (isReservedName(name)) setVar(name, v);  // throws the friendly error
      f.vars[name] = v;
      return;
    }
  }
  setVar(name, v);
}

void Interp::defineFunction(const std::string& name, const std::vector<std::string>& params, const NodeP& body,
                            const std::string& source) {
  if (findBuiltin(name)) throw CalcError("'" + name + "' is a built-in function; pick another name");
  if (isKeyword(name) || (findConstant(name) && name != "i"))
    throw CalcError("'" + name + "' is reserved; pick another name");
  UserFunc f;
  f.name = name;
  f.params = params;
  f.expr = body;
  f.source = source;
  globals.erase(name);
  funcs[name] = f;
}

void Interp::defineProgram(const StmtP& def) {
  if (findBuiltin(def->name)) throw CalcError("'" + def->name + "' is a built-in function; pick another name");
  if (isKeyword(def->name)) throw CalcError("'" + def->name + "' is a keyword; pick another name");
  UserFunc f;
  f.name = def->name;
  f.params = def->params;
  f.isProg = true;
  f.body = def->body;
  f.source = def->source;
  globals.erase(def->name);
  funcs[def->name] = f;
}

Interp::ScopedBinding::ScopedBinding(Interp& in, const std::string& var, const Value& v) : in_(in), var_(var) {
  Frame f;
  f.opaque = false;
  f.vars[var] = v;
  in_.frames_.push_back(std::move(f));
  index_ = in_.frames_.size() - 1;
}

Interp::ScopedBinding::~ScopedBinding() {
  if (in_.frames_.size() > index_) in_.frames_.resize(index_);
}

void Interp::ScopedBinding::set(const Value& v) { in_.frames_[index_].vars[var_] = v; }

Value Interp::evalWith(const NodeP& expr, const std::map<std::string, Value>& binds) {
  Frame f;
  f.opaque = false;
  f.vars = binds;
  frames_.push_back(std::move(f));
  size_t idx = frames_.size() - 1;
  try {
    Value v = eval(expr);
    frames_.resize(idx);
    return v;
  } catch (...) {
    frames_.resize(idx);
    throw;
  }
}

std::function<double(double)> Interp::realFunction(const NodeP& expr, const std::string& var) {
  return [this, expr, var](double x) -> double {
    ScopedBinding b(*this, var, Value(x));
    try {
      Value v = eval(expr);
      if (!v.isNum()) return kNaN;
      cplx z = v.num;
      if (z.imag() != 0.0 && std::abs(z.imag()) > 1e-9 * std::max(1.0, std::abs(z.real()))) return kNaN;
      return z.real();
    } catch (const Interrupted&) {
      throw;
    } catch (const CalcError&) {
      return kNaN;
    }
  };
}

std::function<cplx(cplx)> Interp::complexFunction(const NodeP& expr, const std::string& var) {
  return [this, expr, var](cplx x) -> cplx {
    ScopedBinding b(*this, var, Value(x));
    try {
      Value v = eval(expr);
      if (!v.isNum()) return kNaN;
      return v.num;
    } catch (const Interrupted&) {
      throw;
    } catch (const CalcError&) {
      return kNaN;
    }
  };
}

void Interp::probe(const NodeP& expr, const std::string& var, double at) {
  ScopedBinding b(*this, var, Value(at));
  Value v = eval(expr);
  if (!v.isNum()) throw CalcError(std::string("the expression must give a number, not ") + v.typeName());
}

// ------------------------------------------------------------ evaluation

static long long indexOf(const Value& v, const char* what) {
  double d = v.real("an index");
  if (d != std::floor(d) || d < 1) throw CalcError(std::string(what) + " must be a whole number ≥ 1 (counting starts at 1)");
  return (long long)d;
}

static Value indexValue(const Value& base, const std::vector<Value>& idx) {
  if (base.isStr()) {
    if (idx.size() != 1) throw CalcError("text takes a single index");
    long long i = indexOf(idx[0], "index");
    if (i > (long long)base.str.size()) throw CalcError("index is past the end of the text");
    return Value::string(base.str.substr(size_t(i - 1), 1));
  }
  if (!base.isMat()) throw CalcError(std::string("can't index ") + base.typeName() + " with [ ]");
  const Matrix& m = base.mat;
  if (idx.size() == 1) {
    long long i = indexOf(idx[0], "index");
    if (base.vec || m.cols == 1 || m.rows == 1) {
      if (i > (long long)m.a.size())
        throw CalcError("index " + std::to_string(i) + " is out of range (the list has " + std::to_string(m.a.size()) +
                        " items)");
      return Value(m.a[size_t(i - 1)]);
    }
    if (i > m.rows) throw CalcError("row " + std::to_string(i) + " is out of range (the matrix has " + std::to_string(m.rows) + " rows)");
    std::vector<cplx> row(m.a.begin() + (i - 1) * m.cols, m.a.begin() + i * m.cols);
    return Value::vector(row);
  }
  if (idx.size() == 2) {
    long long r = indexOf(idx[0], "row"), c = indexOf(idx[1], "column");
    int rows = base.vec ? int(m.a.size()) : m.rows, cols = base.vec ? 1 : m.cols;
    if (r > rows || c > cols)
      throw CalcError("[" + std::to_string(r) + ", " + std::to_string(c) + "] is outside the " + std::to_string(rows) +
                      "×" + std::to_string(cols) + " matrix");
    return Value(m(int(r - 1), int(c - 1)));
  }
  throw CalcError("use one index for lists (v[2]) or two for matrices (A[1, 2])");
}

Value Interp::eval(const NodeP& n) {
  tick();
  switch (n->k) {
    case NK::Num: return Value(n->num);
    case NK::Str: return Value::string(n->name);
    case NK::Var: return lookup(n->name, n->pos);
    case NK::Call: return evalCall(n);
    case NK::Index: {
      Value base = eval(n->args[0]);
      std::vector<Value> idx;
      for (size_t i = 1; i < n->args.size(); ++i) idx.push_back(eval(n->args[i]));
      return indexValue(base, idx);
    }
    case NK::Neg: return negate(eval(n->args[0]));
    case NK::Not: return Value(eval(n->args[0]).truthy() ? 0.0 : 1.0);
    case NK::Binary: {
      Value a = eval(n->args[0]);
      Value b = eval(n->args[1]);
      return binaryOp(n->name, a, b);
    }
    case NK::Compare: return compareValues(n->name, eval(n->args[0]), eval(n->args[1]));
    case NK::Eq: return compareValues("==", eval(n->args[0]), eval(n->args[1]));
    case NK::And: return Value(eval(n->args[0]).truthy() && eval(n->args[1]).truthy() ? 1.0 : 0.0);
    case NK::Or: return Value(eval(n->args[0]).truthy() || eval(n->args[1]).truthy() ? 1.0 : 0.0);
    case NK::Postfix: {
      Value v = eval(n->args[0]);
      const std::string& op = n->name;
      if (op == "!") return mapNumbers(v, factorial);
      if (op == "²") return binaryOp("^", v, Value(2.0));
      if (op == "³") return binaryOp("^", v, Value(3.0));
      if (op == "°") {
        bool deg = settings.angle == AngleMode::DEG;
        return mapNumbers(v, [deg](cplx z) { return deg ? z : z * (kPi / 180); });
      }
      if (op == "'") {
        if (!v.isMat()) return v;
        return Value::matrix(transpose(v.mat));
      }
      if (op == "⁻¹") {
        if (v.isMat() && !v.vec) return Value::matrix(inverse(v.mat));
        return binaryOp("/", Value(1.0), v);
      }
      throw CalcError("unknown operator " + op);
    }
    case NK::MatLit: return evalMatLit(n);
  }
  throw CalcError("internal error: unknown node");
}

Value Interp::evalMatLit(const NodeP& n) {
  if (n->rows.empty()) return Value::vector({});
  if (n->rows.size() == 1) {
    std::vector<Value> vals;
    for (const auto& e : n->rows[0]) vals.push_back(eval(e));
    bool allNum = true, allRows = true;
    for (const auto& v : vals) {
      if (!v.isNum()) allNum = false;
      if (!(v.isMat() && (v.vec || v.mat.rows == 1))) allRows = false;
    }
    if (allNum) {
      std::vector<cplx> xs;
      for (const auto& v : vals) xs.push_back(v.num);
      return Value::vector(xs);
    }
    if (allRows) {
      size_t len = vals[0].mat.a.size();
      for (const auto& v : vals)
        if (v.mat.a.size() != len) throw CalcError("all rows of a matrix must have the same length");
      Matrix m(int(vals.size()), int(len));
      for (size_t r = 0; r < vals.size(); ++r)
        for (size_t c = 0; c < len; ++c) m(int(r), int(c)) = vals[r].mat.a[c];
      return Value::matrix(m);
    }
    throw CalcError("a list can only contain numbers, or rows of numbers for a matrix");
  }
  size_t cols = n->rows[0].size();
  Matrix m(int(n->rows.size()), int(cols));
  for (size_t r = 0; r < n->rows.size(); ++r) {
    if (n->rows[r].size() != cols) throw CalcError("all rows of a matrix must have the same length");
    for (size_t c = 0; c < cols; ++c) {
      Value v = eval(n->rows[r][c]);
      if (!v.isNum()) throw CalcError("matrix entries must be numbers");
      m(int(r), int(c)) = v.num;
    }
  }
  return Value::matrix(m);
}

Value Interp::evalCall(const NodeP& n) {
  const std::string& name = n->name;
  auto uf = funcs.find(name);
  if (uf != funcs.end()) {
    std::vector<Value> args;
    for (const auto& a : n->args) args.push_back(eval(a));
    UserFunc f = uf->second;  // copy: the call may redefine it
    return callUser(f, args);
  }
  if (const Builtin* b = findBuiltin(name)) {
    int argc = int(n->args.size());
    if (argc < b->minArgs || (b->maxArgs >= 0 && argc > b->maxArgs)) {
      std::string count = b->minArgs == b->maxArgs ? std::to_string(b->minArgs)
                          : b->maxArgs < 0         ? "at least " + std::to_string(b->minArgs)
                                                   : std::to_string(b->minArgs) + " to " + std::to_string(b->maxArgs);
      bool one = b->maxArgs == 1 || (b->maxArgs < 0 && b->minArgs == 1);
      throw CalcError(name + " takes " + count + (one ? " argument: " : " arguments: ") + b->signature);
    }
    if (b->lazy) return b->lazy(*this, n->args);
    std::vector<Value> args;
    for (const auto& a : n->args) args.push_back(eval(a));
    return b->fn(*this, args);
  }
  Value v;
  if (getVar(name, v) && n->args.size() == 1) return binaryOp("*", v, eval(n->args[0]));
  throw CalcError("unknown function '" + name + "'" + suggest(name));
}

Value Interp::call(const std::string& name, std::vector<Value> args) {
  auto uf = funcs.find(name);
  if (uf != funcs.end()) {
    UserFunc f = uf->second;
    return callUser(f, args);
  }
  if (const Builtin* b = findBuiltin(name)) {
    if (b->lazy) throw CalcError(name + " can't be used here");
    int argc = int(args.size());
    if (argc < b->minArgs || (b->maxArgs >= 0 && argc > b->maxArgs))
      throw CalcError(name + " needs different arguments: " + b->signature);
    return b->fn(*this, args);
  }
  throw CalcError("unknown function '" + name + "'" + suggest(name));
}

Value Interp::callUser(const UserFunc& f, std::vector<Value>& args) {
  if (args.size() != f.params.size()) {
    std::string params;
    for (size_t k = 0; k < f.params.size(); ++k) params += (k ? ", " : "") + f.params[k];
    throw CalcError(f.name + " expects " + std::to_string(f.params.size()) + " argument" +
                    (f.params.size() == 1 ? "" : "s") + ": " + f.name + "(" + params + ")");
  }
  if (callDepth_ > 250) throw CalcError("too many nested calls (is a function calling itself forever?)");
  Frame fr;
  fr.opaque = true;
  for (size_t k = 0; k < args.size(); ++k) fr.vars[f.params[k]] = args[k];
  frames_.push_back(std::move(fr));
  size_t idx = frames_.size() - 1;
  ++callDepth_;
  try {
    Value result;
    if (!f.isProg) {
      result = eval(f.expr);
    } else {
      Value ret, last;
      Flow fl = execBlock(f.body, ret, last);
      if (fl == Flow::Break || fl == Flow::Continue)
        throw CalcError("'break'/'continue' used outside a loop in " + f.name);
      result = fl == Flow::Return ? ret : last;
    }
    frames_.resize(idx);
    --callDepth_;
    return result;
  } catch (...) {
    frames_.resize(idx);
    --callDepth_;
    throw;
  }
}

void Interp::indexAssign(const std::string& name, const std::vector<NodeP>& idxNodes, const Value& v) {
  if (!v.isNum()) throw CalcError("only numbers can be stored in a list or matrix entry");
  std::vector<long long> idx;
  for (const auto& n : idxNodes) idx.push_back(indexOf(eval(n), "index"));
  if (idx.empty() || idx.size() > 2) throw CalcError("use v[i] or A[i, j]");

  Value* slot = nullptr;
  if (!frames_.empty() && !frames_.back().globalNames.count(name)) {
    auto it = frames_.back().vars.find(name);
    if (it != frames_.back().vars.end()) slot = &it->second;
  }
  if (!slot) {
    auto g = globals.find(name);
    if (g != globals.end()) slot = &g->second;
  }
  if (!slot) {
    Value fresh = idx.size() == 1 ? Value::vector({}) : Value::matrix(Matrix(0, 0));
    assign(name, fresh);
    if (!frames_.empty() && frames_.back().vars.count(name)) slot = &frames_.back().vars[name];
    else slot = &globals[name];
  }
  Value& cur = *slot;
  if (!cur.isMat()) throw CalcError("'" + name + "' is " + cur.typeName() + ", not a list or matrix");
  Matrix& m = cur.mat;
  if (idx.size() == 1) {
    long long i = idx[0];
    if (cur.vec || m.cols == 1) {
      if (i > (long long)m.a.size()) {
        if (i > 1000000) throw CalcError("list would be too large");
        m.a.resize(size_t(i), 0.0);
        m.rows = int(i);
        m.cols = 1;
      }
      m.a[size_t(i - 1)] = v.num;
      return;
    }
    if (m.rows == 1) {
      if (i > m.cols) throw CalcError("index out of range");
      m.a[size_t(i - 1)] = v.num;
      return;
    }
    throw CalcError("'" + name + "' is a matrix; use " + name + "[row, column]");
  }
  long long r = idx[0], c = idx[1];
  if (cur.vec) {
    if (c != 1) throw CalcError("'" + name + "' is a list; use " + name + "[i]");
    cur.vec = false;
  }
  if (r > m.rows || c > m.cols) {
    if (r > 2000 || c > 2000) throw CalcError("matrix would be too large");
    int nr = std::max<int>(m.rows, int(r)), nc = std::max<int>(m.cols, int(c));
    Matrix grown(nr, nc);
    for (int rr = 0; rr < m.rows; ++rr)
      for (int cc = 0; cc < m.cols; ++cc) grown(rr, cc) = m(rr, cc);
    m = grown;
  }
  m(int(r - 1), int(c - 1)) = v.num;
}

Interp::Flow Interp::execBlock(const Block& b, Value& ret, Value& last) {
  for (const auto& s : b) {
    Flow f = execStmt(s, ret, last);
    if (f != Flow::Normal) return f;
  }
  return Flow::Normal;
}

static bool isGraphName(const std::string& n) {
  return n.size() == 2 && n[0] == 'y' && n[1] >= '1' && n[1] <= '9';
}

Interp::Flow Interp::execStmt(const StmtP& s, Value& ret, Value& last) {
  tick();
  try {
    switch (s->k) {
      case SK::Expr: last = eval(s->expr); return Flow::Normal;
      case SK::Assign: {
        if (isGraphName(s->name) && frames_.empty()) {
          std::string src = s->source;
          size_t eq = src.find('=');
          std::string rhs = eq == std::string::npos ? src : src.substr(eq + 1);
          while (!rhs.empty() && rhs[0] == ' ') rhs.erase(0, 1);
          defineFunction(s->name, {"x"}, s->expr, rhs);
          last = Value();
          return Flow::Normal;
        }
        Value v = eval(s->expr);
        assign(s->name, v);
        last = v;
        return Flow::Normal;
      }
      case SK::IndexAssign: {
        Value v = eval(s->expr);
        indexAssign(s->name, s->idx, v);
        last = v;
        return Flow::Normal;
      }
      case SK::FuncDef: defineFunction(s->name, s->params, s->expr, s->source); return Flow::Normal;
      case SK::ProgDef: defineProgram(s); return Flow::Normal;
      case SK::If:
        for (const auto& br : s->branches)
          if (eval(br.first).truthy()) return execBlock(br.second, ret, last);
        return execBlock(s->elseBody, ret, last);
      case SK::While:
        while (eval(s->expr).truthy()) {
          tick();
          Flow f = execBlock(s->body, ret, last);
          if (f == Flow::Break) break;
          if (f == Flow::Return) return f;
        }
        return Flow::Normal;
      case SK::Repeat:
        for (;;) {
          tick();
          Flow f = execBlock(s->body, ret, last);
          if (f == Flow::Break) break;
          if (f == Flow::Return) return f;
          if (eval(s->expr).truthy()) break;
        }
        return Flow::Normal;
      case SK::For: {
        double from = eval(s->from).real("a number for the loop start");
        double to = eval(s->to).real("a number for the loop end");
        double step = s->step ? eval(s->step).real("a number for the loop step") : 1.0;
        if (step == 0) throw CalcError("the loop step can't be 0");
        double eps = 1e-12 * std::abs(step);
        for (long long k = 0;; ++k) {
          double v = from + double(k) * step;
          if (step > 0 ? v > to + eps : v < to - eps) break;
          tick();
          assign(s->name, Value(v));
          Flow f = execBlock(s->body, ret, last);
          if (f == Flow::Break) break;
          if (f == Flow::Return) return f;
        }
        return Flow::Normal;
      }
      case SK::ForIn: {
        Value list = eval(s->expr);
        std::vector<cplx> items = list.elements();
        for (const auto& z : items) {
          tick();
          assign(s->name, Value(z));
          Flow f = execBlock(s->body, ret, last);
          if (f == Flow::Break) break;
          if (f == Flow::Return) return f;
        }
        return Flow::Normal;
      }
      case SK::Return:
        if (frames_.empty()) throw CalcError("'return' can only be used inside a program");
        ret = s->expr ? eval(s->expr) : Value();
        return Flow::Return;
      case SK::Break: return Flow::Break;
      case SK::Continue: return Flow::Continue;
      case SK::Print: {
        std::string line;
        std::vector<std::string> extra;
        for (size_t k = 0; k < s->items.size(); ++k) {
          Value v = eval(s->items[k]);
          std::vector<std::string> lines = formatLines(v, settings.display);
          if (k) line += " ";
          line += lines[0];
          for (size_t j = 1; j < lines.size(); ++j) extra.push_back(lines[j]);
        }
        if (onPrint) {
          onPrint(line);
          for (const auto& e : extra) onPrint(e);
        }
        return Flow::Normal;
      }
      case SK::Global:
        if (!frames_.empty())
          for (const auto& nm : s->params) frames_.back().globalNames.insert(nm);
        return Flow::Normal;
    }
  } catch (const Interrupted&) {
    throw;
  } catch (const CalcError& e) {
    // Add the program line number for errors inside programs.
    std::string msg = e.what();
    if (!frames_.empty() && callDepth_ > 0 && msg.find("(line ") == std::string::npos && msg.find("Syntax") == std::string::npos)
      throw CalcError(msg + " (line " + std::to_string(s->line) + ")");
    throw;
  }
  return Flow::Normal;
}

ExecResult Interp::execute(const std::string& src) {
  Block b = parseStatements(src);
  ExecResult res;
  frames_.clear();
  callDepth_ = 0;
  steps_ = 0;
  Value ret, last;
  for (const auto& s : b) {
    last = Value();
    Flow f = execStmt(s, ret, last);
    if (f == Flow::Break || f == Flow::Continue) throw CalcError("'break'/'continue' can only be used inside a loop");
    switch (s->k) {
      case SK::FuncDef: {
        std::string params;
        for (size_t k = 0; k < s->params.size(); ++k) params += (k ? ", " : "") + s->params[k];
        res.message = "Defined " + s->name + "(" + params + ")";
        res.hasValue = false;
        break;
      }
      case SK::ProgDef: {
        std::string params;
        for (size_t k = 0; k < s->params.size(); ++k) params += (k ? ", " : "") + s->params[k];
        res.message = "Saved program " + s->name + "(" + params + ")";
        res.hasValue = false;
        break;
      }
      case SK::Assign:
        if (isGraphName(s->name)) {
          res.message = "Defined " + s->name + "(x) for graphing";
          res.hasValue = false;
          break;
        }
        [[fallthrough]];
      default:
        res.hasValue = !last.isNone();
        res.value = last;
        res.message = res.hasValue ? "" : "Done";
    }
  }
  if (res.hasValue) globals["ans"] = res.value;
  return res;
}

Value Interp::evalString(const std::string& src) {
  NodeP n = parseExpression(src);
  return eval(n);
}

}  // namespace cact
