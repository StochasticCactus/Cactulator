// Cactulator - HOME screen: algebraic entry with history, or the RPN stack.
#include <algorithm>

#include "app.hpp"
#include "core/format.hpp"
#include "core/lexer.hpp"

namespace ui {

using cact::HistoryItem;
using cact::u8len;
using cact::u8substr;

namespace {

struct DLine {
  std::string text;
  Style style;
  bool right = false;
  bool syntax = false;  // apply syntax colors
  std::string click;    // text inserted when the line is clicked
};

bool isNumberEntry(const std::string& s) {
  if (s.empty()) return false;
  try {
    auto t = cact::tokenize(s);
    if (t.size() == 2 && t[0].type == cact::Tok::Num) return true;
    if (t.size() == 3 && t[0].type == cact::Tok::Op && t[0].text == "-" && t[1].type == cact::Tok::Num) return true;
  } catch (const cact::CalcError&) {
  }
  return false;
}

class CalcScreen : public Screen {
 public:
  explicit CalcScreen(App& a) : Screen(a) {}

  LineEdit edit;

  std::string title() const override { return app.s.rpnMode ? "RPN STACK" : "HOME"; }

  std::vector<SoftKey> softKeys() override {
    return {
        {"Help", [this] { app.open(ScreenId::Help); }},
        {"Graph", [this] { app.open(ScreenId::Graph); }},
        {"Matrix", [this] { app.open(ScreenId::Matrix); }},
        {"Stat", [this] { app.open(ScreenId::Stat); }},
        {"Prog", [this] { app.open(ScreenId::Prog); }},
        {"Solve", [this] { app.open(ScreenId::Solve); }},
        {"Units", [this] { app.open(ScreenId::Units); }},
        {"Mode", [this] { app.open(ScreenId::Mode); }},
    };
  }

  void insert(const std::string& text) override {
    const std::string tag = "\x01rpn:";
    if (text.rfind(tag, 0) == 0) {
      rpnWord(text.substr(tag.size()));
      return;
    }
    edit.insert(text);
    recall_ = -1;
  }

  void draw(const Rect& r) override {
    if (r.h < 3) return;
    clicks_.clear();
    int entryY = r.y + r.h - 2, hintY = r.y + r.h - 1;
    Rect hist{r.x, r.y, r.w, r.h - 2};
    if (app.s.rpnMode) drawStack(hist);
    else drawHistory(hist);
    drawEntry(r.x, entryY, r.w);
    drawHint(r.x, hintY, r.w);
  }

  bool key(const Event& e) override {
    auto& s = app.s;
    if (e.type == Event::Type::Text) {
      if (s.rpnMode && e.text.size() == 1 && std::string("+-*/^").find(e.text[0]) != std::string::npos) {
        std::string t = edit.text;
        bool exponentSign = e.text == "-" && !t.empty() && (t.back() == 'E' || t.back() == 'e');
        bool inQuote = std::count(t.begin(), t.end(), '\'') % 2 == 1 || std::count(t.begin(), t.end(), '[') >
                                                                              std::count(t.begin(), t.end(), ']');
        if (!exponentSign && !inQuote && (t.empty() || isNumberEntry(t))) {
          rpnWord(e.text);
          return true;
        }
      }
      edit.insert(e.text);
      recall_ = -1;
      return true;
    }
    if (e.type != Event::Type::Key) return false;
    int k = e.key;
    if (k == keys::Enter) {
      submit();
      return true;
    }
    if (k == keys::Tab) {
      auto [ghost, cands] = completion();
      if (!ghost.empty()) edit.insert(ghost);
      return true;
    }
    if (k == keys::Up) {
      recall(-1);
      return true;
    }
    if (k == keys::Down) {
      recall(+1);
      return true;
    }
    if (k == keys::PageUp) {
      scroll_ += 5;
      return true;
    }
    if (k == keys::PageDown) {
      scroll_ = std::max(0, scroll_ - 5);
      return true;
    }
    if (k == keys::Esc) {
      edit.clear();
      recall_ = -1;
      scroll_ = 0;
      return true;
    }
    if (k == keys::Ctrl('l')) {
      s.history.clear();
      scroll_ = 0;
      return true;
    }
    if (s.rpnMode && edit.empty()) {
      if (k == keys::Backspace || k == keys::Delete) {
        rpnWord("drop");
        return true;
      }
      if (k == keys::Right) {
        rpnWord("swap");
        return true;
      }
    }
    if (k == keys::Ctrl('z')) {
      if (s.rpnMode) app.toast(s.rpn.undo() ? "Undone" : "Nothing to undo");
      return true;
    }
    if (edit.handle(e)) {
      recall_ = -1;
      return true;
    }
    return false;
  }

  bool mouse(const Event& e, const Rect&) override {
    if (e.mouse == Event::Mouse::WheelUp) {
      scroll_ += 3;
      return true;
    }
    if (e.mouse == Event::Mouse::WheelDown) {
      scroll_ = std::max(0, scroll_ - 3);
      return true;
    }
    if (e.mouse == Event::Mouse::Press)
      for (const auto& [y, text] : clicks_)
        if (y == e.my && !text.empty()) {
          edit.insert(text);
          app.toast("Inserted — press Enter to use it");
          return true;
        }
    return false;
  }

 private:
  int scroll_ = 0;
  int recall_ = -1;
  std::string draft_;
  std::vector<std::pair<int, std::string>> clicks_;

  std::vector<std::string> inputs() const {
    std::vector<std::string> v;
    for (const auto& h : app.s.history)
      if (!h.input.empty() && (v.empty() || v.back() != h.input)) v.push_back(h.input);
    return v;
  }

  void recall(int dir) {
    auto in = inputs();
    if (in.empty()) return;
    if (dir < 0) {
      if (recall_ == -1) {
        draft_ = edit.text;
        recall_ = int(in.size()) - 1;
      } else if (recall_ > 0) {
        --recall_;
      }
      edit.set(in[size_t(recall_)]);
    } else {
      if (recall_ == -1) return;
      ++recall_;
      if (recall_ >= (int)in.size()) {
        recall_ = -1;
        edit.set(draft_);
      } else {
        edit.set(in[size_t(recall_)]);
      }
    }
  }

  void submit() {
    auto& s = app.s;
    if (s.rpnMode) {
      size_t before = s.history.size();
      cact::SubmitResult r = s.submitRpn(edit.text);
      reportRpnErrors(before);
      edit.clear();
      recall_ = -1;
      app.handleAction(r);
      return;
    }
    std::string line = balanceBrackets(edit.text);
    if (cact::trim(line).empty()) return;
    app.busy("working… (Esc stops)");
    cact::SubmitResult r = s.submit(line);
    app.toast("");
    edit.clear();
    recall_ = -1;
    scroll_ = 0;
    app.handleAction(r);
    app.save();
  }

  void reportRpnErrors(size_t before) {
    auto& h = app.s.history;
    for (size_t i = before; i < h.size(); ++i)
      if (h[i].kind == HistoryItem::Kind::Error && !h[i].lines.empty()) app.toast(h[i].lines[0], true);
  }

  void rpnWord(const std::string& w) {
    auto& s = app.s;
    size_t before = s.history.size();
    std::string entry = cact::trim(edit.text);
    try {
      s.rpn.process(s.in, entry.empty() ? w : entry + " " + w);
      if (!s.rpn.stack.empty()) s.in.globals["ans"] = s.rpn.stack.back();
      edit.clear();
    } catch (const cact::CalcError& err) {
      app.toast(err.what(), true);
    }
    reportRpnErrors(before);
  }

  // ----- completion

  std::pair<std::string, std::vector<std::string>> completion() const {
    size_t start;
    std::string prefix = identBeforeCursor(edit, &start);
    if (cact::u8len(prefix) < 2) return {"", {}};
    const auto& in = app.s.in;
    std::vector<std::string> cands;
    for (const auto& b : cact::builtinList())
      if (b.name.rfind(prefix, 0) == 0) cands.push_back(b.name);
    for (const auto& [k, f] : in.funcs)
      if (k.rfind(prefix, 0) == 0) cands.push_back(k);
    for (const auto& [k, v] : in.globals)
      if (k.rfind(prefix, 0) == 0 && k != "ans") cands.push_back(k);
    for (const auto& c : cact::constantList())
      if (c.name.rfind(prefix, 0) == 0) cands.push_back(c.name);
    std::stable_sort(cands.begin(), cands.end(),
                     [](const std::string& a, const std::string& b) { return a.size() < b.size(); });
    cands.erase(std::unique(cands.begin(), cands.end()), cands.end());
    if (cands.empty()) return {"", {}};
    const std::string& best = cands[0];
    std::string ghost = best.substr(prefix.size());
    bool isFn = in.isFunctionName(best);
    std::string after = edit.afterCursor();
    if (isFn && (after.empty() || after[0] != '(')) ghost += "(";
    if (ghost.empty()) return {"", cands};
    return {ghost, cands};
  }

  // ----- drawing

  void addWrapped(std::vector<DLine>& out, const std::string& text, const Style& st, bool right, bool syntax,
                  const std::string& click, int w, bool clip = false) {
    if ((int)u8len(text) <= w) {
      out.push_back({text, st, right, syntax, click});
      return;
    }
    if (clip) {
      out.push_back({u8substr(text, 0, size_t(w - 1)) + "…", st, right, syntax, click});
      return;
    }
    for (size_t i = 0; i < u8len(text); i += size_t(w)) out.push_back({u8substr(text, i, size_t(w)), st, right, syntax, click});
  }

  void drawHistory(const Rect& r) {
    const Theme& t = app.th();
    const auto& hist = app.s.history;
    int w = r.w;
    if (hist.empty()) {
      std::vector<std::pair<std::string, Style>> welcome = {
          {"Welcome to Cactulator!", app.lcdStrong()},
          {"", app.lcd()},
          {"Type a calculation and press Enter:", app.lcd()},
          {"   2+3*4      sin(30)      √2      5!", app.lcdAccent()},
          {"   x = 7      f(x) = x^2 - 1      f(3)", app.lcdAccent()},
          {"   inv([[1,2],[3,4]])      5 km to mi", app.lcdAccent()},
          {"   plot sin(x), cos(x)", app.lcdAccent()},
          {"", app.lcd()},
          {"F1 opens the guide. Tab completes names.", app.lcdDim()},
      };
      int y = r.y + std::max(0, (r.h - int(welcome.size())) / 2);
      for (const auto& [text, st] : welcome) {
        if (y >= r.y + r.h) break;
        term().put(r.x + 1, y++, text, st, w - 1);
      }
      return;
    }
    std::vector<DLine> lines;
    std::string warn = app.term.ascii ? "! " : "⚠ ";
    for (const auto& h : hist) {
      if (!h.input.empty()) addWrapped(lines, h.input, app.lcd(), false, true, h.input, w);
      std::string click = h.value.isNone() ? "" : cact::serialize(h.value);
      if (h.value.isNum() || (h.value.isMat() && h.value.mat.a.size() <= 16))
        click = cact::formatInline(h.value, app.s.in.settings.display);
      if (click == "undefined") click.clear();
      for (const auto& l : h.lines) {
        switch (h.kind) {
          case HistoryItem::Kind::Result:
            addWrapped(lines, l, app.lcdStrong(), true, false, click, w, !l.empty() && l.rfind("[", 0) != 0 ? true : false);
            break;
          case HistoryItem::Kind::Error: addWrapped(lines, warn + l, app.lcdError(), false, false, "", w); break;
          case HistoryItem::Kind::Message: addWrapped(lines, l, app.lcdAccent(), true, false, "", w); break;
          case HistoryItem::Kind::Print: addWrapped(lines, l, Style(t.lcdAccent, t.lcdBg), false, false, "", w); break;
        }
      }
    }
    int total = int(lines.size());
    int maxScroll = std::max(0, total - r.h);
    scroll_ = std::min(scroll_, maxScroll);
    int first = std::max(0, total - r.h - scroll_);
    int y = r.y + std::max(0, r.h - (total - first));
    for (int i = first; i < total && y < r.y + r.h; ++i, ++y) {
      const DLine& d = lines[size_t(i)];
      int len = int(u8len(d.text));
      int x = d.right ? r.x + r.w - len : r.x;
      if (d.syntax) drawSpans(term(), x, y, r.w, d.text, highlight(d.text, t, &app.s.in), d.style);
      else term().put(x, y, d.text, d.style, r.w);
      if (!d.click.empty()) clicks_.push_back({y, d.click});
    }
    if (scroll_ > 0) {
      std::string more = " ↓ " + std::to_string(scroll_) + " more ";
      term().put(r.x + r.w - int(u8len(more)), r.y + r.h - 1, more, app.lcdSel());
    }
  }

  void drawStack(const Rect& r) {
    const Theme& t = app.th();
    const auto& st = app.s.rpn.stack;
    int n = int(st.size());
    int y = r.y + r.h - 1;
    int labelW = 3;
    // Always show at least levels 1..4 like an HP 48.
    int shown = std::max(n, std::min(4, r.h));
    for (int level = 1; level <= shown && y >= r.y; ++level) {
      int idx = n - level;
      std::vector<std::string> lines;
      if (idx >= 0) lines = cact::formatLines(st[size_t(idx)], app.s.in.settings.display);
      else lines = {""};
      int h = int(lines.size());
      int top = y - h + 1;
      for (int j = 0; j < h; ++j) {
        int yy = top + j;
        if (yy < r.y) continue;
        if (j == 0 || h == 1) {
          std::string lab = std::to_string(level) + ":";
          term().put(r.x, yy, lab, Style(t.lcdDim, t.lcdBg));
        }
        std::string l = lines[size_t(j)];
        int room = r.w - labelW - 1;
        if ((int)u8len(l) > room) l = u8substr(l, 0, size_t(std::max(1, room - 1))) + "…";
        term().put(r.x + r.w - int(u8len(l)), yy, l, Style(t.lcdStrong, t.lcdBg, level == 1));
        if (idx >= 0) clicks_.push_back({yy, cact::formatInline(st[size_t(idx)], app.s.in.settings.display)});
      }
      y = top - 1;
    }
  }

  void drawEntry(int x, int y, int w) {
    const Theme& t = app.th();
    std::string prompt = app.term.ascii ? "> " : "› ";
    term().put(x, y, prompt, Style(t.lcdAccent, t.lcdBg, true));
    auto [ghost, cands] = completion();
    drawEdit(term(), x + 2, y, w - 2, edit, t, app.lcd(), true, highlight(edit.text, t, &app.s.in, long(edit.cur)),
             ghost);
  }

  void drawHint(int x, int y, int w) {
    const Theme& t = app.th();
    Style dim(t.lcdDim, t.lcdBg);
    auto [ghost, cands] = completion();
    std::string text;
    auto describe = [&](const std::string& name) -> std::string {
      if (const cact::Builtin* b = cact::findBuiltin(name)) return b->signature + " — " + b->help;
      auto f = app.s.in.funcs.find(name);
      if (f != app.s.in.funcs.end()) {
        std::string params;
        for (size_t i = 0; i < f->second.params.size(); ++i) params += (i ? ", " : "") + f->second.params[i];
        return name + "(" + params + ") — your " + (f->second.isProg ? "program" : "function");
      }
      if (const cact::Constant* c = cact::findConstant(name)) return name + " — " + c->help;
      return "";
    };
    if (!cands.empty()) {
      text = "Tab ⇥ " + describe(cands[0]);
      if (cands.size() > 1) {
        std::string more = "   also:";
        for (size_t i = 1; i < cands.size() && i < 5; ++i) more += " " + cands[i];
        if ((int)u8len(text + more) <= w) text += more;
      }
    } else {
      std::string fn = enclosingCall(edit);
      if (!fn.empty()) text = describe(fn);
      if (text.empty()) {
        std::string id = identAtCursor(edit);
        if (!id.empty()) text = describe(id);
      }
    }
    if (text.empty()) {
      if (app.s.rpnMode)
        text = "3 Enter 4 + · Enter=DUP · ⌫=DROP · →=SWAP · Ctrl-Z undo · 'x^2+1' for algebra";
      else
        text = "Enter = calculate · ↑↓ = earlier entries · Tab = complete · click a result to reuse it";
    }
    if ((int)u8len(text) > w) text = u8substr(text, 0, size_t(w - 1)) + "…";
    term().put(x, y, text, dim);
  }

  Term& term() { return app.term; }
};

}  // namespace

std::unique_ptr<Screen> makeCalcScreen(App& a) { return std::make_unique<CalcScreen>(a); }

void calcInsert(App& a, const std::string& text) {
  a.open(ScreenId::Calc);
  auto& cs = static_cast<CalcScreen&>(a.screen(ScreenId::Calc));
  cs.edit.insert(text);
}

}  // namespace ui
