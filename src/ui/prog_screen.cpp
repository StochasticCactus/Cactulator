// Copyright (C) 2026 StochasticCactus
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of Cactulator.
//
// Cactulator is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// Cactulator is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along
// with Cactulator; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

// Cactulator - PRGM app: list of your programs/functions and a code editor.
#include <cstdlib>
#include <memory>
#include <algorithm>

#include "app.hpp"
#include "core/format.hpp"
#include "core/parser.hpp"

namespace ui {

using cact::u8len;
using cact::u8substr;

namespace {

class ProgScreen : public Screen {
 public:
  explicit ProgScreen(App& a) : Screen(a) {}

  std::string title() const override { return editing_ ? "EDIT " + editName_ : "PROGRAMS"; }

  void enter(const std::string& arg) override {
    message_.clear();
    if (!arg.empty()) {
      auto it = app.s.in.funcs.find(arg);
      if (it != app.s.in.funcs.end()) openEditor(arg);
      else newProgram(arg);
    }
  }

  std::vector<SoftKey> softKeys() override {
    if (editing_)
      return {
          {"Save", [this] { save(false); }},
          {"Run", [this] { if (save(false)) run(editName_); }},
          {"if", [this] { snippet({"if x > 0 then", "  ", "else", "  ", "end"}, 1); }},
          {"while", [this] { snippet({"while x < 10 do", "  ", "end"}, 1); }},
          {"for", [this] { snippet({"for k = 1 to 10 do", "  ", "end"}, 1); }},
          {"print", [this] { snippet({"print \"x =\", x"}, 0); }},
          {"input", [this] { snippet({"x = input(\"Value of x?\")"}, 0); }},
          {"Done", [this] { if (save(false)) editing_ = false; }},
      };
    return {
        {"New", [this] { newProgram(""); }},
        {"Edit", [this] { auto n = selected(); if (!n.empty()) openEditor(n); }},
        {"Run", [this] { auto n = selected(); if (!n.empty()) run(n); }},
        {"Delete", [this] { remove(); }},
        {"Function", [this] { newFunction(); }},
        {"", nullptr},
        {"", nullptr},
        {"Home", [this] { app.back(); }},
    };
  }

  void draw(const Rect& r) override {
    rect_ = r;
    if (editing_) drawEditor(r);
    else drawList(r);
  }

  bool key(const Event& e) override { return editing_ ? keyEditor(e) : keyList(e); }

  void insert(const std::string& text) override {
    if (!editing_) return;
    if (text.size() == 1 && text[0] < 32) return;
    lines_[size_t(row_)] = u8substr(lines_[size_t(row_)], 0, size_t(col_)) + text + u8substr(lines_[size_t(row_)], size_t(col_));
    col_ += int(u8len(text));
    dirty_ = true;
  }

  bool mouse(const Event& e, const Rect& r) override {
    if (e.mouse == Event::Mouse::WheelUp) {
      if (editing_) top_ = std::max(0, top_ - 3);
      else sel_ = std::max(0, sel_ - 1);
      return true;
    }
    if (e.mouse == Event::Mouse::WheelDown) {
      if (editing_) top_ = std::min(int(lines_.size()) - 1, top_ + 3);
      else sel_ = std::min(int(app.s.programNames().size()) - 1, sel_ + 1);
      return true;
    }
    if (e.mouse != Event::Mouse::Press) return false;
    if (editing_) {
      int rr = top_ + (e.my - r.y);
      if (rr >= 0 && rr < (int)lines_.size()) {
        row_ = rr;
        col_ = std::clamp(e.mx - (r.x + gutter_), 0, int(u8len(lines_[size_t(row_)])));
      }
      return true;
    }
    int idx = listTop_ + (e.my - (r.y + 1));
    if (idx >= 0 && idx < (int)app.s.programNames().size()) {
      if (idx == sel_ && e.mx < r.x + listW_) openEditor(app.s.programNames()[size_t(idx)]);
      sel_ = idx;
    }
    return true;
  }

 private:
  // list
  int sel_ = 0, listTop_ = 0, listW_ = 30;
  // editor
  bool editing_ = false, dirty_ = false, pendingExit_ = false;
  std::string editName_;
  std::vector<std::string> lines_;
  int row_ = 0, col_ = 0, top_ = 0, gutter_ = 4;
  std::string message_;
  bool messageError_ = false;
  Rect rect_;

  Term& term() { return app.term; }

  std::string selected() {
    auto names = app.s.programNames();
    if (names.empty()) return "";
    sel_ = std::clamp(sel_, 0, int(names.size()) - 1);
    return names[size_t(sel_)];
  }

  std::string signature(const std::string& name) const {
    const auto& f = app.s.in.funcs.at(name);
    std::string params;
    for (size_t i = 0; i < f.params.size(); ++i) params += (i ? ", " : "") + f.params[i];
    return name + "(" + params + ")";
  }

  void drawList(const Rect& r) {
    const Theme& t = app.th();
    auto names = app.s.programNames();
    listW_ = std::min(34, std::max(20, r.w / 3));
    term().put(r.x, r.y, "Your programs & functions", app.lcdDim(), listW_);
    if (names.empty()) {
      term().put(r.x, r.y + 2, "Nothing here yet.", app.lcd());
      term().put(r.x, r.y + 3, "F1 New program · F5 New function", app.lcdAccent());
      return;
    }
    sel_ = std::clamp(sel_, 0, int(names.size()) - 1);
    int visible = r.h - 2;
    if (sel_ < listTop_) listTop_ = sel_;
    if (sel_ >= listTop_ + visible) listTop_ = sel_ - visible + 1;
    for (int i = 0; i < visible && listTop_ + i < (int)names.size(); ++i) {
      const std::string& n = names[size_t(listTop_ + i)];
      const auto& f = app.s.in.funcs.at(n);
      bool sel = listTop_ + i == sel_;
      Style st = sel ? app.lcdSel() : app.lcd();
      int y = r.y + 1 + i;
      if (sel) term().fill({r.x, y, listW_, 1}, st);
      std::string tag = f.isProg ? "▸ " : "ƒ ";
      if (app.term.ascii) tag = f.isProg ? "> " : "f ";
      term().put(r.x, y, tag, Style(f.isProg ? t.synKeyword : t.synFunc, st.bg, true));
      term().put(r.x + 2, y, signature(n), Style(st.fg, st.bg, sel), listW_ - 2);
    }
    // Preview
    std::string n = selected();
    int px = r.x + listW_ + 2, pw = r.w - listW_ - 2;
    if (pw < 16 || n.empty()) return;
    const auto& f = app.s.in.funcs.at(n);
    term().vrule(px - 1, r.y, r.h - 1, app.lcdDim(), app.term.ascii ? "|" : "│");
    std::vector<std::string> src;
    std::string s = f.source;
    size_t start = 0;
    while (start <= s.size()) {
      size_t nl = s.find('\n', start);
      src.push_back(s.substr(start, nl == std::string::npos ? std::string::npos : nl - start));
      if (nl == std::string::npos) break;
      start = nl + 1;
    }
    for (int i = 0; i < r.h - 2 && i < (int)src.size(); ++i)
      drawSpans(term(), px + 1, r.y + i, pw - 1, src[size_t(i)], highlight(src[size_t(i)], t, &app.s.in), app.lcd());
    term().put(px + 1, r.y + r.h - 2, "Enter edit · F3 run · F4 delete", app.lcdDim(), pw - 1);
    if (!message_.empty()) term().put(r.x, r.y + r.h - 1, message_, messageError_ ? app.lcdError() : app.lcdAccent(), r.w);
  }

  bool keyList(const Event& e) {
    auto names = app.s.programNames();
    if (e.is(keys::Up)) sel_ = std::max(0, sel_ - 1);
    else if (e.is(keys::Down)) sel_ = std::min(int(names.size()) - 1, sel_ + 1);
    else if (e.is(keys::Enter)) {
      auto n = selected();
      if (!n.empty()) openEditor(n);
    } else if (e.is(keys::Delete)) remove();
    else return false;
    return true;
  }

  void openEditor(const std::string& name) {
    const auto& f = app.s.in.funcs.at(name);
    editName_ = name;
    lines_.clear();
    std::string s = f.source;
    size_t start = 0;
    while (start <= s.size()) {
      size_t nl = s.find('\n', start);
      lines_.push_back(s.substr(start, nl == std::string::npos ? std::string::npos : nl - start));
      if (nl == std::string::npos) break;
      start = nl + 1;
    }
    if (lines_.empty()) lines_.push_back("");
    row_ = std::min(1, int(lines_.size()) - 1);
    col_ = int(u8len(lines_[size_t(row_)]));
    top_ = 0;
    editing_ = true;
    dirty_ = false;
    pendingExit_ = false;
    message_ = f.isProg ? "Ctrl-S saves · Esc saves and leaves · F3–F7 insert building blocks" : "A one-line function: name(x) = expression";
    messageError_ = false;
  }

  void newProgram(std::string name) {
    if (name.empty() && !app.ask("Name of the new program", name, "Letters and digits, e.g. area or solve2")) return;
    name = cact::trim(name);
    if (name.empty()) return;
    if (app.s.in.isReservedName(name)) {
      app.toast("'" + name + "' is a built-in name; pick another", true);
      return;
    }
    editName_ = name;
    lines_ = {"prog " + name + "(x)", "  # What does it do? Describe it here.", "  y = 2*x + 1", "  return y", "end"};
    row_ = 2;
    col_ = int(u8len(lines_[2]));
    top_ = 0;
    editing_ = true;
    dirty_ = true;
    pendingExit_ = false;
    message_ = "Edit the template · F8 Done saves · then run it from HOME: " + name + "(5)";
    messageError_ = false;
  }

  void newFunction() {
    std::string def = "g(x) = x^2 + 1";
    if (!app.ask("New one-line function", def, "Write it like  area(r) = pi*r^2")) return;
    try {
      auto r = app.s.in.execute(def);
      app.toast(r.message.empty() ? "Defined" : r.message);
      app.save();
    } catch (const cact::CalcError& e) {
      app.toast(e.what(), true);
    }
  }

  void remove() {
    auto n = selected();
    if (n.empty()) return;
    if (!app.confirm("Delete " + signature(n) + "?")) return;
    app.s.in.funcs.erase(n);
    app.save();
    message_ = "Deleted " + n;
  }

  void run(const std::string& name) {
    auto it = app.s.in.funcs.find(name);
    if (it == app.s.in.funcs.end()) return;
    std::string call = name + "()";
    if (!it->second.params.empty()) {
      std::string params;
      for (size_t i = 0; i < it->second.params.size(); ++i) params += (i ? ", " : "") + it->second.params[i];
      std::string args;
      if (!app.ask("Run " + name + " — values for " + params, args, "Separate several values with commas")) return;
      call = name + "(" + args + ")";
    }
    editing_ = false;
    app.open(ScreenId::Calc);
    app.busy("running " + name + "… (Esc stops)");
    app.handleAction(app.s.submit(call));
    app.toast("");
    app.save();
  }

  std::string source() const {
    std::string s;
    for (size_t i = 0; i < lines_.size(); ++i) s += (i ? "\n" : "") + lines_[i];
    return s;
  }

  bool save(bool quiet) {
    std::string src = cact::trim(source());
    try {
      cact::Block b = cact::parseStatements(src);
      if (b.size() != 1 || (b[0]->k != cact::SK::ProgDef && b[0]->k != cact::SK::FuncDef))
        throw cact::CalcError("a program must start with 'prog name(…)' and finish with 'end'");
      app.s.in.execute(src);
      std::string newName = b[0]->name;
      if (newName != editName_ && app.s.in.funcs.count(editName_) && app.s.in.funcs.count(newName)) {
        app.s.in.funcs.erase(editName_);
      }
      editName_ = newName;
      auto names = app.s.programNames();
      auto it = std::find(names.begin(), names.end(), newName);
      if (it != names.end()) sel_ = int(it - names.begin());
      dirty_ = false;
      pendingExit_ = false;
      if (!quiet) {
        message_ = "Saved ✓  Run it from HOME: " + signature(newName);
        messageError_ = false;
      }
      app.save();
      return true;
    } catch (const cact::CalcError& e) {
      message_ = e.what();
      messageError_ = true;
      // Jump to the line of the error.
      std::string m = e.what();
      size_t p = m.find("(line ");
      if (p != std::string::npos) {
        int ln = std::atoi(m.c_str() + p + 6);
        if (ln >= 1 && ln <= (int)lines_.size()) {
          row_ = ln - 1;
          col_ = std::min(col_, int(u8len(lines_[size_t(row_)])));
        }
      }
      return false;
    }
  }

  void snippet(const std::vector<std::string>& block, int cursorLine) {
    std::string indent;
    const std::string& cur = lines_[size_t(row_)];
    size_t sp = cur.find_first_not_of(' ');
    indent = std::string(sp == std::string::npos ? cur.size() : sp, ' ');
    bool blank = cact::trim(cur).empty();
    int at = blank ? row_ : row_ + 1;
    if (blank) lines_.erase(lines_.begin() + row_);
    for (size_t i = 0; i < block.size(); ++i) lines_.insert(lines_.begin() + at + long(i), indent + block[i]);
    row_ = at + cursorLine;
    col_ = int(u8len(lines_[size_t(row_)]));
    dirty_ = true;
  }

  void drawEditor(const Rect& r) {
    const Theme& t = app.th();
    gutter_ = 4;
    int textW = r.w - gutter_;
    int visible = r.h - 1;
    if (row_ < top_) top_ = row_;
    if (row_ >= top_ + visible) top_ = row_ - visible + 1;
    int hscroll = std::max(0, col_ - textW + 2);
    for (int i = 0; i < visible; ++i) {
      int ln = top_ + i;
      int y = r.y + i;
      if (ln >= (int)lines_.size()) {
        term().put(r.x, y, "  ~", app.lcdDim());
        continue;
      }
      term().put(r.x, y, cact::padLeft(std::to_string(ln + 1), 3), Style(ln == row_ ? t.lcdAccent : t.lcdDim, t.lcdBg, ln == row_));
      const std::string& line = lines_[size_t(ln)];
      if (ln == row_) {
        LineEdit le;
        le.text = line;
        le.cur = size_t(col_);
        ui::drawEdit(term(), r.x + gutter_, y, textW, le, t, app.lcd(), true, highlight(line, t, &app.s.in, col_));
      } else {
        drawSpans(term(), r.x + gutter_, y, textW, line, highlight(line, t, &app.s.in), app.lcd(), size_t(hscroll > 0 && ln == row_ ? hscroll : 0));
      }
    }
    std::string status = message_;
    if (status.empty()) status = std::string(dirty_ ? "● modified · " : "") + "Ctrl-S save · Esc done · Tab indent";
    term().put(r.x, r.y + r.h - 1, status, messageError_ ? app.lcdError() : app.lcdDim(), r.w);
  }

  bool keyEditor(const Event& e) {
    std::string& line = lines_[size_t(row_)];
    int len = int(u8len(line));
    if (e.type == Event::Type::Text) {
      insert(e.text);
      pendingExit_ = false;
      return true;
    }
    if (e.type != Event::Type::Key) return false;
    int k = e.key;
    if (k != keys::Esc) pendingExit_ = false;
    if (k == keys::Esc) {
      if (pendingExit_ || !dirty_) {
        editing_ = false;
        return true;
      }
      if (save(true)) {
        editing_ = false;
        app.toast("Saved " + editName_);
      } else {
        pendingExit_ = true;
        message_ += "  — fix it, or Esc again to discard changes";
      }
      return true;
    }
    if (k == keys::Ctrl('s')) {
      save(false);
      return true;
    }
    if (k == keys::Left) {
      if (col_ > 0) --col_;
      else if (row_ > 0) {
        --row_;
        col_ = int(u8len(lines_[size_t(row_)]));
      }
    } else if (k == keys::Right) {
      if (col_ < len) ++col_;
      else if (row_ + 1 < (int)lines_.size()) {
        ++row_;
        col_ = 0;
      }
    } else if (k == keys::Up) {
      if (row_ > 0) --row_;
      col_ = std::min(col_, int(u8len(lines_[size_t(row_)])));
    } else if (k == keys::Down) {
      if (row_ + 1 < (int)lines_.size()) ++row_;
      col_ = std::min(col_, int(u8len(lines_[size_t(row_)])));
    } else if (k == keys::Home || k == keys::Ctrl('a')) {
      size_t sp = line.find_first_not_of(' ');
      int ind = sp == std::string::npos ? 0 : int(sp);
      col_ = col_ == ind ? 0 : ind;
    } else if (k == keys::End || k == keys::Ctrl('e')) {
      col_ = len;
    } else if (k == keys::PageUp) {
      row_ = std::max(0, row_ - (rect_.h - 2));
      col_ = std::min(col_, int(u8len(lines_[size_t(row_)])));
    } else if (k == keys::PageDown) {
      row_ = std::min(int(lines_.size()) - 1, row_ + (rect_.h - 2));
      col_ = std::min(col_, int(u8len(lines_[size_t(row_)])));
    } else if (k == keys::Enter) {
      std::string before = u8substr(line, 0, size_t(col_)), after = u8substr(line, size_t(col_));
      size_t sp = before.find_first_not_of(' ');
      std::string indent(sp == std::string::npos ? before.size() : sp, ' ');
      std::string tb = cact::trim(before);
      auto endsWith = [&](const char* w) {
        std::string s(w);
        return tb.size() >= s.size() && tb.compare(tb.size() - s.size(), s.size(), s) == 0;
      };
      if (endsWith(" then") || endsWith(" do") || tb == "else" || tb == "repeat" || tb.rfind("prog ", 0) == 0)
        indent += "  ";
      line = before;
      lines_.insert(lines_.begin() + row_ + 1, indent + cact::trim(after));
      ++row_;
      col_ = int(indent.size());
      dirty_ = true;
    } else if (k == keys::Backspace) {
      if (col_ > 0) {
        // Remove a whole indent step when only spaces precede the cursor.
        std::string before = u8substr(line, 0, size_t(col_));
        int n = (cact::trim(before).empty() && col_ >= 2) ? 2 : 1;
        line = u8substr(line, 0, size_t(col_ - n)) + u8substr(line, size_t(col_));
        col_ -= n;
      } else if (row_ > 0) {
        int prevLen = int(u8len(lines_[size_t(row_ - 1)]));
        lines_[size_t(row_ - 1)] += line;
        lines_.erase(lines_.begin() + row_);
        --row_;
        col_ = prevLen;
      }
      dirty_ = true;
    } else if (k == keys::Delete) {
      if (col_ < len) line = u8substr(line, 0, size_t(col_)) + u8substr(line, size_t(col_ + 1));
      else if (row_ + 1 < (int)lines_.size()) {
        line += lines_[size_t(row_ + 1)];
        lines_.erase(lines_.begin() + row_ + 1);
      }
      dirty_ = true;
    } else if (k == keys::Tab) {
      insert("  ");
    } else if (k == keys::Ctrl('d')) {
      lines_.insert(lines_.begin() + row_, line);
      dirty_ = true;
    } else if (k == keys::Ctrl('x')) {
      if (lines_.size() > 1) {
        lines_.erase(lines_.begin() + row_);
        row_ = std::min(row_, int(lines_.size()) - 1);
      } else {
        lines_[0].clear();
      }
      col_ = std::min(col_, int(u8len(lines_[size_t(row_)])));
      dirty_ = true;
    } else {
      return false;
    }
    return true;
  }
};

}  // namespace

std::unique_ptr<Screen> makeProgScreen(App& a) { return std::make_unique<ProgScreen>(a); }

}  // namespace ui
