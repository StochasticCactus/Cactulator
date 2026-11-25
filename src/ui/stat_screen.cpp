// Cactulator - STAT app: data lists L1…L4, one-variable statistics,
// regressions (linear, quadratic, exponential …) and scatter plots.
#include <memory>
#include <algorithm>
#include <cmath>

#include "app.hpp"
#include "canvas.hpp"
#include "core/format.hpp"
#include "core/numeric.hpp"
#include "core/parser.hpp"

namespace ui {

using cact::u8len;
using cact::Value;

namespace {

const char* kModels[] = {"linear", "quadratic", "cubic", "exponential", "power", "logarithmic"};
const char* kModelNames[] = {"Linear", "Quadratic", "Cubic", "Exponential", "Power", "Logarithmic"};

class StatScreen : public Screen {
 public:
  explicit StatScreen(App& a) : Screen(a) {}

  std::string title() const override { return plot_ ? "STAT PLOT" : "STATISTICS"; }

  void enter(const std::string&) override {
    plot_ = false;
    editing_ = false;
  }

  std::vector<SoftKey> softKeys() override {
    if (plot_)
      return {{"Data", [this] { plot_ = false; }}, {"Model", [this] { nextModel(); }}, {"→ y1", [this] { toGraph(); }},
              {"", nullptr}, {"", nullptr}, {"", nullptr}, {"", nullptr}, {"Home", [this] { app.back(); }}};
    return {
        {"1-Var", [this] { oneVar(); }},
        {"Fit", [this] { regression(); }},
        {std::string(kModelNames[model_]).substr(0, 6), [this] { nextModel(); }},
        {"Plot", [this] { plot_ = true; }},
        {"→ y1", [this] { toGraph(); }},
        {"Sort", [this] { sortCol(); }},
        {"Clear", [this] { clearCol(); }},
        {"Home", [this] { app.back(); }},
    };
  }

  void draw(const Rect& r) override {
    if (plot_) {
      drawPlot(r);
      return;
    }
    const Theme& t = app.th();
    int cw = 10;
    int gridW = 4 + 4 * (cw + 1);
    bool side = r.w >= gridW + 26;
    int visRows = r.h - 3;
    int maxLen = 0;
    for (int c = 0; c < 4; ++c) maxLen = std::max(maxLen, int(list(c).size()));
    int rows = maxLen + 1;
    if (selR_ >= rows) selR_ = rows - 1;
    if (selR_ < scroll_) scroll_ = selR_;
    if (selR_ >= scroll_ + visRows) scroll_ = selR_ - visRows + 1;
    for (int c = 0; c < 4; ++c) {
      std::string h = "L" + std::to_string(c + 1);
      std::string role = c == 0 ? " (x)" : c == 1 ? " (y)" : "";
      term().put(r.x + 4 + c * (cw + 1), r.y, cact::padLeft(h + role, size_t(cw)),
                 Style(c == selC_ ? t.lcdAccent : t.lcdStrong, t.lcdBg, true));
    }
    term().hrule(r.x, r.y + 1, std::min(r.w, gridW), app.lcdDim(), app.term.ascii ? "-" : "─");
    cells_.clear();
    for (int i = 0; i < visRows; ++i) {
      int row = scroll_ + i;
      if (row >= rows) break;
      int y = r.y + 2 + i;
      term().put(r.x, y, cact::padLeft(std::to_string(row + 1), 3), app.lcdDim());
      for (int c = 0; c < 4; ++c) {
        auto l = list(c);
        bool sel = row == selR_ && c == selC_;
        Style st = sel ? app.lcdSel() : app.lcd();
        int x = r.x + 4 + c * (cw + 1);
        std::string s = row < (int)l.size() ? cact::formatShort(l[size_t(row)], cw - 1, app.s.in.settings.display) : "";
        if (sel && editing_) {
          term().fill({x, y, cw, 1}, st);
        } else {
          if (sel) term().fill({x, y, cw, 1}, st);
          term().put(x, y, cact::padLeft(s.empty() && sel ? "_" : s, size_t(cw)), st);
        }
        cells_.push_back({Rect{x, y, cw, 1}, {row, c}});
      }
    }
    int by = r.y + r.h - 1;
    if (editing_) {
      std::string label = "L" + std::to_string(selC_ + 1) + "(" + std::to_string(selR_ + 1) + ") = ";
      term().put(r.x, by, label, app.lcdAccent());
      ui::drawEdit(term(), r.x + int(u8len(label)), by, r.w - int(u8len(label)), edit_, t, app.lcd(), true,
                   highlight(edit_.text, t, &app.s.in, long(edit_.cur)));
    } else {
      term().put(r.x, by, "Type numbers · Enter next row · Del removes · x = L1, y = L2 for fits", app.lcdDim(), r.w);
    }
    // Results panel
    Rect res = side ? Rect{r.x + gridW + 2, r.y, r.w - gridW - 2, r.h - 1}
                    : Rect{r.x, r.y + 2 + std::min(visRows, rows) + 1, r.w, 0};
    if (!side) res.h = r.y + r.h - 1 - res.y;
    int y = res.y;
    if (!message_.empty()) {
      for (const auto& l : wrapText(message_, res.w))
        if (y < res.y + res.h) term().put(res.x, y++, l, messageError_ ? app.lcdError() : app.lcdAccent());
    }
    for (const auto& [k, v] : results_) {
      if (y >= res.y + res.h) break;
      if (k.empty()) {
        term().put(res.x, y++, v, app.lcdAccent(), res.w);
        continue;
      }
      term().put(res.x, y, k, app.lcdDim(), res.w);
      term().put(res.x + std::min(10, res.w / 2), y, v, app.lcdStrong(), res.w - std::min(10, res.w / 2));
      ++y;
    }
  }

  bool key(const Event& e) override {
    if (plot_) {
      if (e.is(keys::Esc)) {
        plot_ = false;
        return true;
      }
      return false;
    }
    if (editing_) {
      if (e.is(keys::Enter) || e.is(keys::Down)) {
        if (commit()) ++selR_;
        return true;
      }
      if (e.is(keys::Up)) {
        if (commit() && selR_ > 0) --selR_;
        return true;
      }
      if (e.is(keys::Esc)) {
        editing_ = false;
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
    int k = e.key;
    int len = int(list(selC_).size());
    if (k == keys::Up) selR_ = std::max(0, selR_ - 1);
    else if (k == keys::Down) selR_ = std::min(selR_ + 1, len);
    else if (k == keys::Left) selC_ = std::max(0, selC_ - 1);
    else if (k == keys::Right || k == keys::Tab) selC_ = std::min(3, selC_ + 1);
    else if (k == keys::PageUp) selR_ = std::max(0, selR_ - 10);
    else if (k == keys::PageDown) selR_ = std::min(len, selR_ + 10);
    else if (k == keys::Enter) {
      editing_ = true;
      auto l = list(selC_);
      if (selR_ < (int)l.size()) edit_.set(cact::formatInline(Value(l[size_t(selR_)]), app.s.in.settings.display));
      else edit_.clear();
    } else if (k == keys::Delete || k == keys::Backspace) {
      auto l = list(selC_);
      if (selR_ < (int)l.size()) {
        l.erase(l.begin() + selR_);
        setList(selC_, l);
      }
    } else return false;
    return true;
  }

  bool mouse(const Event& e, const Rect&) override {
    if (e.mouse != Event::Mouse::Press || plot_) return false;
    for (const auto& [rc, pos] : cells_)
      if (rc.contains(e.mx, e.my)) {
        if (editing_) commit();
        selR_ = std::min(pos.first, int(list(pos.second).size()));
        selC_ = pos.second;
        return true;
      }
    return false;
  }

 private:
  int selR_ = 0, selC_ = 0, scroll_ = 0, model_ = 0;
  bool editing_ = false, plot_ = false;
  LineEdit edit_;
  std::string message_;
  bool messageError_ = false;
  std::vector<std::pair<std::string, std::string>> results_;
  std::vector<std::pair<Rect, std::pair<int, int>>> cells_;

  Term& term() { return app.term; }

  std::string listName(int c) const { return "L" + std::to_string(c + 1); }

  std::vector<double> list(int c) const {
    auto it = app.s.in.globals.find(listName(c));
    if (it == app.s.in.globals.end() || !it->second.isMat()) return {};
    std::vector<double> out;
    for (const auto& z : it->second.mat.a) out.push_back(z.real());
    return out;
  }

  void setList(int c, const std::vector<double>& v) { app.s.in.globals[listName(c)] = Value::vectorReal(v); }

  bool commit() {
    try {
      Value v = app.s.in.evalString(edit_.text);
      auto l = list(selC_);
      if (v.isMat()) {
        // Paste a whole list: L1 = [1, 2, 3]
        auto e = v.realElements();
        l.erase(l.begin() + std::min<long>(selR_, long(l.size())), l.end());
        l.insert(l.end(), e.begin(), e.end());
      } else {
        double x = v.real();
        if (selR_ < (int)l.size()) l[size_t(selR_)] = x;
        else l.push_back(x);
      }
      setList(selC_, l);
      editing_ = false;
      message_.clear();
      return true;
    } catch (const cact::CalcError& e) {
      message_ = e.what();
      messageError_ = true;
      return false;
    }
  }

  std::string f(double v) const {
    cact::DisplaySettings d = app.s.in.settings.display;
    d.frac = false;
    d.base = 10;
    return cact::formatReal(v, d);
  }

  void oneVar() {
    results_.clear();
    message_.clear();
    auto xs = list(selC_);
    if (xs.empty()) {
      message_ = "L" + std::to_string(selC_ + 1) + " is empty — type some numbers first";
      messageError_ = true;
      return;
    }
    std::vector<double> s = xs;
    std::sort(s.begin(), s.end());
    size_t n = s.size();
    auto med = [](const std::vector<double>& v, size_t a, size_t b) {  // median of v[a, b)
      size_t m = b - a;
      if (m == 0) return std::nan("");
      return m % 2 ? v[a + m / 2] : 0.5 * (v[a + m / 2 - 1] + v[a + m / 2]);
    };
    double sum = 0, sum2 = 0;
    for (double x : xs) {
      sum += x;
      sum2 += x * x;
    }
    results_.push_back({"", "1-variable stats of " + listName(selC_)});
    results_.push_back({"n", f(double(n))});
    results_.push_back({"mean x̄", f(sum / n)});
    results_.push_back({"Σx", f(sum)});
    results_.push_back({"Σx²", f(sum2)});
    if (n > 1) results_.push_back({"Sx", f(std::sqrt(cact::variance(xs, true)))});
    results_.push_back({"σx", f(std::sqrt(cact::variance(xs, false)))});
    results_.push_back({"min", f(s.front())});
    if (n >= 4) results_.push_back({"Q1", f(med(s, 0, n / 2))});
    results_.push_back({"median", f(med(s, 0, n))});
    if (n >= 4) results_.push_back({"Q3", f(med(s, (n + 1) / 2, n))});
    results_.push_back({"max", f(s.back())});
    app.s.in.globals["ans"] = Value(sum / n);
  }

  bool fit(cact::Fit& out) {
    auto xs = list(0), ys = list(1);
    if (xs.size() != ys.size()) {
      message_ = "L1 has " + std::to_string(xs.size()) + " values but L2 has " + std::to_string(ys.size());
      messageError_ = true;
      return false;
    }
    try {
      out = cact::fitModel(kModels[model_], xs, ys);
      return true;
    } catch (const cact::CalcError& e) {
      message_ = e.what();
      messageError_ = true;
      return false;
    }
  }

  void regression() {
    results_.clear();
    message_.clear();
    cact::Fit ft;
    if (!fit(ft)) return;
    results_.push_back({"", std::string(kModelNames[model_]) + " fit, x = L1, y = L2"});
    results_.push_back({"", ft.equation});
    const char* names[] = {"a", "b", "c", "d"};
    for (size_t i = 0; i < ft.coef.size() && i < 4; ++i) results_.push_back({names[i], f(ft.coef[i])});
    results_.push_back({"R²", f(ft.r2)});
    if (std::string(kModels[model_]) == "linear") results_.push_back({"r", f(ft.r)});
    app.s.in.globals["ans"] = Value::vectorReal(ft.coef);
  }

  void nextModel() {
    model_ = (model_ + 1) % 6;
    if (!results_.empty() && results_[0].second.find("fit") != std::string::npos) regression();
    else app.toast(std::string("Fit model: ") + kModelNames[model_]);
  }

  void toGraph() {
    cact::Fit ft;
    if (!fit(ft)) return;
    app.s.setGraphEntry(0, cact::GraphEntry::Type::Func, ft.expr, true);
    app.toast("Stored the fit as y1 — press F2 on HOME to graph it");
    message_ = "y1 = " + ft.equation.substr(4);
    messageError_ = false;
  }

  void sortCol() {
    auto l = list(selC_);
    std::sort(l.begin(), l.end());
    setList(selC_, l);
  }

  void clearCol() {
    if (list(selC_).empty()) return;
    if (!app.confirm("Clear all of " + listName(selC_) + "?")) return;
    setList(selC_, {});
    selR_ = 0;
  }

  void drawPlot(const Rect& r) {
    const Theme& t = app.th();
    auto xs = list(0), ys = list(1);
    size_t n = std::min(xs.size(), ys.size());
    if (n == 0) {
      term().put(r.x, r.y, "Enter x values in L1 and y values in L2 first.", app.lcdError(), r.w);
      return;
    }
    double xmin = *std::min_element(xs.begin(), xs.begin() + long(n)), xmax = *std::max_element(xs.begin(), xs.begin() + long(n));
    double ymin = *std::min_element(ys.begin(), ys.begin() + long(n)), ymax = *std::max_element(ys.begin(), ys.begin() + long(n));
    double px = (xmax - xmin) * 0.1 + (xmax == xmin ? 1 : 0), py = (ymax - ymin) * 0.1 + (ymax == ymin ? 1 : 0);
    xmin -= px;
    xmax += px;
    ymin -= py;
    ymax += py;
    cact::Fit ft;
    bool haveFit = false;
    try {
      ft = cact::fitModel(kModels[model_], std::vector<double>(xs.begin(), xs.begin() + long(n)),
                          std::vector<double>(ys.begin(), ys.begin() + long(n)));
      haveFit = true;
    } catch (const cact::CalcError&) {
    }
    std::string head = haveFit ? ft.equation + "   R² = " + f(ft.r2) : std::string(kModelNames[model_]) + " fit not possible";
    term().put(r.x, r.y, head, Style(t.plot[1], t.lcdBg, true), r.w);
    Canvas cv(r.w, r.h - 2);
    int PW = cv.pw(), PH = cv.ph();
    auto X = [&](double x) { return (x - xmin) / (xmax - xmin) * (PW - 1); };
    auto Y = [&](double y) { return (ymax - y) / (ymax - ymin) * (PH - 1); };
    if (ymin <= 0 && ymax >= 0)
      for (int i = 0; i < PW; ++i) cv.set(i, int(std::lround(Y(0))), t.axis);
    if (xmin <= 0 && xmax >= 0)
      for (int i = 0; i < PH; ++i) cv.set(int(std::lround(X(0))), i, t.axis);
    if (haveFit) {
      auto node = cact::parseExpression(ft.expr);
      auto fn = app.s.in.realFunction(node, "x");
      double prevX = NAN, prevY = NAN;
      for (int i = 0; i < PW * 2; ++i) {
        double x = xmin + (xmax - xmin) * i / (PW * 2 - 1);
        double y = fn(x);
        if (std::isfinite(y) && std::isfinite(prevY)) cv.line(prevX, prevY, X(x), Y(y), t.plot[1]);
        prevX = X(x);
        prevY = std::isfinite(y) ? Y(y) : NAN;
      }
    }
    for (size_t i = 0; i < n; ++i) cv.marker(int(std::lround(X(xs[i]))), int(std::lround(Y(ys[i]))), app.term.ascii ? "o" : "●", t.plot[0]);
    cv.text(0, cv.rows() - 1, "x " + f(xmin) + "…" + f(xmax), t.axis);
    cv.text(0, 0, "y " + f(ymin) + "…" + f(ymax), t.axis);
    cv.render(term(), r.x, r.y + 1, t.lcdBg);
    term().put(r.x, r.y + r.h - 1, std::to_string(n) + " points · F2 changes the model · Esc back to the data", app.lcdDim(), r.w);
  }
};

}  // namespace

std::unique_ptr<Screen> makeStatScreen(App& a) { return std::make_unique<StatScreen>(a); }

}  // namespace ui
