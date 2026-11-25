// Cactulator - MATRIX app: spreadsheet-like editor for matrices A…F with
// one-key inverse, determinant, transpose, RREF, eigenvalues and more.
#include <cstdio>
#include <memory>
#include <algorithm>

#include "app.hpp"
#include "core/format.hpp"
#include "core/linalg.hpp"

namespace ui {

using cact::Matrix;
using cact::u8len;
using cact::Value;

namespace {

class MatrixScreen : public Screen {
 public:
  explicit MatrixScreen(App& a) : Screen(a) {}

  std::string title() const override { return "MATRIX " + name_; }

  void enter(const std::string& arg) override {
    if (!arg.empty()) name_ = arg;
    editing_ = false;
    clampSel();
  }

  std::vector<SoftKey> softKeys() override {
    if (more_)
      return {
          {"Rank", [this] { op("rank"); }},
          {"Trace", [this] { op("trace"); }},
          {"Ax=b", [this] { solveSystem(); }},
          {"Power", [this] { power(); }},
          {"Store", [this] { storeResult(); }},
          {"Name", [this] { pickName(); }},
          {"Ident", [this] { makeIdentity(); }},
          {"Back", [this] { more_ = false; }},
      };
    return {
        {"Size", [this] { resize(); }},
        {"Inverse", [this] { op("inv"); }},
        {"Det", [this] { op("det"); }},
        {"Transp", [this] { op("transpose"); }},
        {"RREF", [this] { op("rref"); }},
        {"Eigen", [this] { eigen(); }},
        {"More", [this] { more_ = true; }},
        {"Home", [this] { app.back(); }},
    };
  }

  void draw(const Rect& r) override {
    const Theme& t = app.th();
    rect_ = r;
    // Tabs
    tabs_.clear();
    int x = r.x;
    for (const char* n : {"A", "B", "C", "D", "E", "F"}) {
      Value v;
      bool exists = getMatrix(n, v);
      std::string label = std::string(" ") + n + (exists ? " " + dims(v) : "") + " ";
      bool sel = name_ == n;
      Style st = sel ? Style(t.lcdBg, t.lcdFg, true) : (exists ? app.lcd() : app.lcdDim());
      term().put(x, r.y, label, st);
      tabs_.push_back({Rect{x, r.y, int(u8len(label)), 1}, n});
      x += int(u8len(label)) + 1;
    }
    if (std::string("ABCDEF").find(name_) == std::string::npos || name_.size() != 1)
      term().put(x, r.y, " " + name_ + " ", Style(t.lcdBg, t.lcdFg, true));

    Matrix m = current();
    int rows = m.rows, cols = m.cols;
    // Grid geometry
    int cw = 10;
    int gx = r.x + 4, gy = r.y + 2;
    int visCols = std::max(1, (r.w - 4) / (cw + 1));
    int resultsH = std::min(std::max(0, r.h - 6 - rows), 10);
    int visRows = std::max(1, r.h - 4 - (lastLines_.empty() ? 0 : std::min(resultsH, int(lastLines_.size()) + 1)));
    if (selC_ < scrollC_) scrollC_ = selC_;
    if (selC_ >= scrollC_ + visCols) scrollC_ = selC_ - visCols + 1;
    if (selR_ < scrollR_) scrollR_ = selR_;
    if (selR_ >= scrollR_ + visRows) scrollR_ = selR_ - visRows + 1;
    for (int c = 0; c < std::min(visCols, cols - scrollC_); ++c) {
      std::string h = std::to_string(scrollC_ + c + 1);
      term().put(gx + c * (cw + 1) + (cw - int(h.size())) / 2, r.y + 1, h, app.lcdDim());
    }
    cells_.clear();
    for (int rr = 0; rr < std::min(visRows, rows - scrollR_); ++rr) {
      int y = gy + rr;
      int row = scrollR_ + rr;
      term().put(r.x, y, cact::padLeft(std::to_string(row + 1), 2), app.lcdDim());
      std::string lb = app.term.ascii ? "[" : (rows == 1 ? "[" : row == 0 ? "⎡" : row == rows - 1 ? "⎣" : "⎢");
      std::string rb = app.term.ascii ? "]" : (rows == 1 ? "]" : row == 0 ? "⎤" : row == rows - 1 ? "⎦" : "⎥");
      term().put(gx - 1, y, lb, app.lcdDim());
      for (int c = 0; c < std::min(visCols, cols - scrollC_); ++c) {
        int col = scrollC_ + c;
        int cx = gx + c * (cw + 1);
        bool sel = row == selR_ && col == selC_;
        Style st = sel ? app.lcdSel() : app.lcd();
        if (sel && editing_) {
          term().fill({cx, y, cw, 1}, st);
        } else {
          std::string s = cact::formatShort(m(row, col), cw - 1, app.s.in.settings.display);
          term().put(cx, y, cact::padLeft(s, size_t(cw)), Style(st.fg, st.bg, sel));
        }
        cells_.push_back({Rect{cx, y, cw, 1}, {row, col}});
      }
      int last = std::min(visCols, cols - scrollC_);
      term().put(gx + last * (cw + 1), y, rb, app.lcdDim());
    }
    int y = gy + std::min(visRows, rows - scrollR_) + 1;
    // Editing line or results
    if (editing_) {
      std::string label = name_ + "[" + std::to_string(selR_ + 1) + "," + std::to_string(selC_ + 1) + "] = ";
      term().put(r.x, y, label, app.lcdAccent());
      ui::drawEdit(term(), r.x + int(u8len(label)), y, r.w - int(u8len(label)), edit_, t, app.lcd(), true,
                   highlight(edit_.text, t, &app.s.in, long(edit_.cur)));
      ++y;
    }
    if (!message_.empty()) {
      term().put(r.x, y++, message_, messageError_ ? app.lcdError() : app.lcdAccent(), r.w);
    }
    for (size_t i = 0; i < lastLines_.size() && y < r.y + r.h - 1; ++i, ++y) {
      std::string l = lastLines_[i];
      if ((int)u8len(l) > r.w) l = cact::u8substr(l, 0, size_t(r.w - 1)) + "…";
      term().put(r.x + 2, y, l, i == 0 ? app.lcdAccent() : app.lcdStrong());
    }
    term().put(r.x, r.y + r.h - 1,
               editing_ ? "Enter ✓ (fractions & expressions ok: 1/3, √2)  Esc ✗"
                        : "Arrows move · type a number · PgUp/PgDn switch matrix · results go to ans",
               app.lcdDim(), r.w);
  }

  bool key(const Event& e) override {
    if (editing_) {
      if (e.is(keys::Enter) || e.is(keys::Tab)) {
        commit(true);
        return true;
      }
      if (e.is(keys::Esc)) {
        editing_ = false;
        return true;
      }
      if (e.is(keys::Up) || e.is(keys::Down)) {
        if (commit(false)) selR_ = std::clamp(selR_ + (e.is(keys::Up) ? -1 : 1), 0, current().rows - 1);
        return true;
      }
      return edit_.handle(e);
    }
    if (e.type == Event::Type::Text) {
      editing_ = true;
      edit_.clear();
      edit_.insert(e.text);
      return true;
    }
    if (e.type != Event::Type::Key) return false;
    Matrix m = current();
    int k = e.key;
    if (k == keys::Up) selR_ = std::max(0, selR_ - 1);
    else if (k == keys::Down) selR_ = std::min(m.rows - 1, selR_ + 1);
    else if (k == keys::Left) selC_ = std::max(0, selC_ - 1);
    else if (k == keys::Right || k == keys::Tab) selC_ = std::min(m.cols - 1, selC_ + 1);
    else if (k == keys::Enter) {
      editing_ = true;
      edit_.set(cact::formatInline(Value(m(selR_, selC_)), app.s.in.settings.display));
    } else if (k == keys::Delete || k == keys::Backspace) setCell(selR_, selC_, 0.0);
    else if (k == keys::PageUp) switchTab(-1);
    else if (k == keys::PageDown) switchTab(1);
    else if (k == keys::Esc && more_) more_ = false;
    else return false;
    return true;
  }

  bool mouse(const Event& e, const Rect&) override {
    if (e.mouse != Event::Mouse::Press) return false;
    for (const auto& [rc, n] : tabs_)
      if (rc.contains(e.mx, e.my)) {
        name_ = n;
        editing_ = false;
        clampSel();
        return true;
      }
    for (const auto& [rc, pos] : cells_)
      if (rc.contains(e.mx, e.my)) {
        if (editing_) commit(false);
        selR_ = pos.first;
        selC_ = pos.second;
        return true;
      }
    return false;
  }

 private:
  std::string name_ = "A";
  int selR_ = 0, selC_ = 0, scrollR_ = 0, scrollC_ = 0;
  bool editing_ = false, more_ = false;
  LineEdit edit_;
  Rect rect_;
  std::string message_;
  bool messageError_ = false;
  std::vector<std::string> lastLines_;
  Value lastValue_;
  std::vector<std::pair<Rect, std::string>> tabs_;
  std::vector<std::pair<Rect, std::pair<int, int>>> cells_;

  Term& term() { return app.term; }

  static std::string dims(const Value& v) {
    return std::to_string(v.mat.rows) + "×" + std::to_string(v.mat.cols);
  }

  bool getMatrix(const std::string& n, Value& v) const {
    auto it = app.s.in.globals.find(n);
    if (it == app.s.in.globals.end() || !it->second.isMat()) return false;
    v = it->second;
    return true;
  }

  Matrix current() const {
    Value v;
    if (getMatrix(name_, v) && v.mat.rows > 0 && v.mat.cols > 0) return v.mat;
    return Matrix(2, 2);
  }

  void store(const Matrix& m) {
    Value old;
    bool vec = getMatrix(name_, old) && old.vec && m.cols == 1;
    app.s.in.setVar(name_, Value::matrix(m, vec));
  }

  void clampSel() {
    Matrix m = current();
    selR_ = std::clamp(selR_, 0, m.rows - 1);
    selC_ = std::clamp(selC_, 0, m.cols - 1);
  }

  void switchTab(int d) {
    static const std::string names = "ABCDEF";
    size_t i = names.find(name_);
    if (i == std::string::npos || name_.size() != 1) i = 0;
    i = (i + names.size() + size_t(d)) % names.size();
    name_ = std::string(1, names[i]);
    clampSel();
  }

  void setCell(int r, int c, cact::cplx v) {
    Matrix m = current();
    m(r, c) = v;
    store(m);
  }

  bool commit(bool advance) {
    try {
      Value v = app.s.in.evalString(edit_.text);
      if (!v.isNum()) throw cact::CalcError("a matrix entry must be a single number");
      setCell(selR_, selC_, v.num);
      editing_ = false;
      message_.clear();
      if (advance) {
        Matrix m = current();
        if (selC_ < m.cols - 1) ++selC_;
        else if (selR_ < m.rows - 1) {
          selC_ = 0;
          ++selR_;
        }
      }
      return true;
    } catch (const cact::CalcError& e) {
      message_ = e.what();
      messageError_ = true;
      return false;
    }
  }

  void resize() {
    Matrix m = current();
    std::string s = std::to_string(m.rows) + " " + std::to_string(m.cols);
    if (!app.ask("Size of " + name_ + " (rows columns)", s, "For example: 3 3  or  2x4")) return;
    for (auto& ch : s)
      if (ch == 'x' || ch == 'X' || ch == ',' || ch == '*') ch = ' ';
    int r = 0, c = 0;
    if (std::sscanf(s.c_str(), "%d %d", &r, &c) < 1) return;
    if (c == 0) c = r;
    if (r < 1 || c < 1 || r > 50 || c > 50) {
      message_ = "Size must be between 1 and 50";
      messageError_ = true;
      return;
    }
    Matrix n(r, c);
    for (int i = 0; i < std::min(r, m.rows); ++i)
      for (int j = 0; j < std::min(c, m.cols); ++j) n(i, j) = m(i, j);
    app.s.in.setVar(name_, Value::matrix(n));
    clampSel();
    message_ = name_ + " is now " + std::to_string(r) + "×" + std::to_string(c);
    messageError_ = false;
  }

  void show(const std::string& label, const Value& v) {
    lastValue_ = v;
    app.s.in.globals["ans"] = v;
    lastLines_ = cact::formatLines(v, app.s.in.settings.display);
    lastLines_.insert(lastLines_.begin(), label + " =");
    message_.clear();
  }

  void op(const std::string& fn) {
    try {
      Value r = app.s.in.call(fn, {Value::matrix(current())});
      std::string label = fn == "transpose" ? name_ + "ᵀ" : fn == "inv" ? name_ + "⁻¹" : fn + "(" + name_ + ")";
      show(label, r);
    } catch (const cact::CalcError& e) {
      lastLines_.clear();
      message_ = e.what();
      messageError_ = true;
    }
    more_ = false;
  }

  void eigen() {
    try {
      std::vector<cact::cplx> vals;
      Matrix vecs = cact::eigenvectors(current(), &vals);
      std::vector<std::string> lines = {"Eigenvalues of " + name_ + " =", cact::formatInline(Value::vector(vals), app.s.in.settings.display),
                                        "Eigenvectors (columns, same order) ="};
      for (const auto& l : cact::formatLines(Value::matrix(vecs), app.s.in.settings.display)) lines.push_back(l);
      lastLines_ = lines;
      lastValue_ = Value::vector(vals);
      app.s.in.globals["ans"] = lastValue_;
      message_.clear();
    } catch (const cact::CalcError& e) {
      lastLines_.clear();
      message_ = e.what();
      messageError_ = true;
    }
  }

  void solveSystem() {
    std::string b = "B";
    if (!app.ask("Solve " + name_ + "·x = b.  b is (a matrix name or a list):", b,
                 "Examples: B   or   [1, 2, 3]")) return;
    try {
      Value bv = app.s.in.evalString(b);
      if (bv.isNum()) bv = Value::vector({bv.num});
      if (!bv.isMat()) throw cact::CalcError("b must be a list or matrix");
      Matrix x = cact::solveLinear(current(), bv.mat);
      show("x", Value::matrix(x, bv.vec));
    } catch (const cact::CalcError& e) {
      lastLines_.clear();
      message_ = e.what();
      messageError_ = true;
    }
    more_ = false;
  }

  void power() {
    std::string n = "2";
    if (!app.ask(name_ + " to the power n =", n)) return;
    try {
      Value r = cact::binaryOp("^", Value::matrix(current()), app.s.in.evalString(n));
      show(name_ + "^" + n, r);
    } catch (const cact::CalcError& e) {
      message_ = e.what();
      messageError_ = true;
    }
    more_ = false;
  }

  void storeResult() {
    if (lastValue_.isNone()) {
      message_ = "Nothing to store yet — run an operation first";
      messageError_ = true;
      return;
    }
    std::string n = "B";
    if (!app.ask("Store the last result as", n)) return;
    try {
      app.s.in.setVar(cact::trim(n), lastValue_);
      message_ = "Stored in " + cact::trim(n);
      messageError_ = false;
    } catch (const cact::CalcError& e) {
      message_ = e.what();
      messageError_ = true;
    }
    more_ = false;
  }

  void pickName() {
    std::string n = name_;
    if (!app.ask("Edit which matrix? (any variable name)", n)) return;
    n = cact::trim(n);
    if (n.empty()) return;
    name_ = n;
    clampSel();
    more_ = false;
  }

  void makeIdentity() {
    Matrix m = current();
    int n = std::max(m.rows, m.cols);
    app.s.in.setVar(name_, Value::matrix(Matrix::identity(n)));
    clampSel();
    more_ = false;
  }
};

}  // namespace

std::unique_ptr<Screen> makeMatrixScreen(App& a) { return std::make_unique<MatrixScreen>(a); }

}  // namespace ui
