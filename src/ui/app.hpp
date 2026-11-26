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

// Cactulator - the full-screen application: device frame, LCD, keypad and screens.
#pragma once

#include <algorithm>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "core/session.hpp"
#include "term.hpp"
#include "theme.hpp"
#include "widgets.hpp"

namespace ui {

class App;

struct SoftKey {
  std::string label;
  std::function<void()> action;
  bool active = false;  // shown highlighted (toggles, current tab)
};

class Screen {
 public:
  explicit Screen(App& a) : app(a) {}
  virtual ~Screen() = default;
  virtual std::string title() const = 0;
  virtual void enter(const std::string& /*arg*/) {}
  virtual void draw(const Rect& r) = 0;  // the LCD content area
  virtual bool key(const Event& e) = 0;  // true if handled
  virtual bool mouse(const Event& /*e*/, const Rect& /*r*/) { return false; }
  // Keypad presses: text to insert, or commands like "enter", "del", "clear".
  virtual void insert(const std::string& text) {
    Event e;
    e.type = Event::Type::Text;
    e.text = text;
    key(e);
  }
  virtual std::vector<SoftKey> softKeys() = 0;
  // Status bar override (e.g. "RPN"); empty = default mode line.
  virtual std::string hint() { return ""; }

 protected:
  App& app;
};

enum class ScreenId { Calc, Graph, Matrix, Stat, Prog, Solve, Units, Help, Mode, Count };

struct KeyDef {
  std::string label, shiftLabel;
  std::string alg, rpn;            // text inserted (algebraic) / word run (RPN)
  std::string salg, srpn;          // shifted versions
  std::string cmd, scmd;           // commands instead of text
  enum class Kind { Num, Fn, Op, App, Enter, Shift } kind = Kind::Fn;
  int span = 1;
};

struct Options {
  bool mouse = true;
  bool ascii = false;
  bool splash = true;
  bool loadState = true;
  std::string theme;
  std::string statePath;
};

class App {
 public:
  App(cact::Session& s, const Options& opt);
  ~App();
  int run();

  cact::Session& s;
  Term term;
  const Theme& th() const;
  Options opt;

  void open(ScreenId id, const std::string& arg = "");
  void back();
  ScreenId current() const { return cur_; }
  Screen& screen(ScreenId id);
  void toast(const std::string& msg, bool error = false);
  void cycleTheme();
  void setTheme(const std::string& name);
  void toggleKeypad();
  void setKeypadMode(const std::string& m) { keypadMode_ = m; }
  void toggleAngle();
  void toggleRpn();
  void quit() { running_ = false; }
  void save();
  void handleAction(const cact::SubmitResult& r);

  // Modal prompts (run their own little event loop).
  bool ask(const std::string& title, std::string& value, const std::string& help = "");
  bool confirm(const std::string& question);
  void busy(const std::string& what);  // show a busy note immediately

  // Styles used everywhere
  Style lcd() const;
  Style lcdDim() const;
  Style lcdStrong() const;
  Style lcdAccent() const;
  Style lcdError() const;
  Style lcdSel() const;
  Style body() const;

  bool shift = false;
  Rect lcdRect() const { return lcdContent_; }

 private:
  bool running_ = true;
  int theme_ = 0;
  ScreenId cur_ = ScreenId::Calc;
  std::vector<std::unique_ptr<Screen>> screens_;
  std::string toast_;
  bool toastError_ = false;
  std::chrono::steady_clock::time_point toastTime_;
  std::string keypadMode_ = "auto";  // auto / show / hide

  // layout results
  Rect frame_, lcdBox_, lcdContent_, statusRect_, menuRect_, keypadRect_;
  bool keypadCompact_ = false;
  std::vector<std::pair<Rect, int>> keyRects_;
  std::vector<std::pair<Rect, int>> softRects_;
  int pressedKey_ = -1;
  std::chrono::steady_clock::time_point lastCtrlC_{};
  std::chrono::steady_clock::time_point pressTime_;
  std::vector<KeyDef> keys_;
  int keyCols_ = 6;

  // modal overlay
  struct Modal {
    bool on = false;
    std::string title, help;
    LineEdit edit;
    bool yesNo = false;
  } modal_;

  void layout();
  void draw();
  void drawFrame();
  void drawStatus();
  void drawSoftKeys(const std::vector<SoftKey>& sk);
  void drawKeypad();
  void drawModal();
  void drawSplash();
  void handle(const Event& e);
  void pressKey(int index);
  void runKeypadCommand(const std::string& cmd);
  bool modalLoop();
};

// Factories for the screens (defined in their own files).
std::unique_ptr<Screen> makeCalcScreen(App& a);
std::unique_ptr<Screen> makeGraphScreen(App& a);
std::unique_ptr<Screen> makeMatrixScreen(App& a);
std::unique_ptr<Screen> makeStatScreen(App& a);
std::unique_ptr<Screen> makeProgScreen(App& a);
std::unique_ptr<Screen> makeSolveScreen(App& a);
std::unique_ptr<Screen> makeUnitsScreen(App& a);
std::unique_ptr<Screen> makeHelpScreen(App& a);
std::unique_ptr<Screen> makeModeScreen(App& a);

// Calc screen helper used by other screens ("send to calculator").
void calcInsert(App& a, const std::string& text);

int runTui(cact::Session& s, const Options& opt);

}  // namespace ui
