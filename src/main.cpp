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

// Cactulator - a programmable graphing calculator for the terminal.
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <io.h>
#define CACT_ISATTY _isatty
#define CACT_STDIN 0
#define CACT_STDOUT 1
#else
#include <unistd.h>
#define CACT_ISATTY isatty
#define CACT_STDIN STDIN_FILENO
#define CACT_STDOUT STDOUT_FILENO
#endif

#include "core/session.hpp"
#include "repl.hpp"
#include "ui/app.hpp"

static const char* kUsage = R"USAGE(Cactulator - a programmable graphing calculator for your terminal

Usage: cactulator [options]

  (no options)        start the full-screen calculator
  -e, --eval EXPR     calculate EXPR, print the answer and exit (repeatable)
  --repl              plain line-by-line mode (automatic when input is piped)
  --ascii             plain ASCII drawing (for terminals without Unicode fonts)
  --no-mouse          don't capture the mouse (keeps normal text selection)
  --no-splash         skip the cactus welcome screen
  --theme NAME        Classic, Cactus, Retro LCD, Midnight or Paper
  --fresh             start empty: don't load or save your saved state
  --state FILE        keep variables, programs and history in FILE
  -h, --help          show this help
  -v, --version       show the version

Examples:
  cactulator
  cactulator -e "inv([[1,2],[3,4]])" -e "solve(x^2 = 2, x)"
  echo "5 km to mi" | cactulator

Copyright (C) 2026 StochasticCactus. Free software under the GNU GPL v2 or
later, with ABSOLUTELY NO WARRANTY. Run with --version for details.
)USAGE";

int main(int argc, char** argv) {
#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
#endif
  ui::Options opt;
  std::vector<std::string> evals;
  bool repl = false, fresh = false, noSplash = false;
  std::string statePath;

  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    auto need = [&](const char* what) -> std::string {
      if (i + 1 >= argc) {
        std::cerr << "cactulator: " << a << " needs " << what << "\n";
        std::exit(2);
      }
      return argv[++i];
    };
    if (a == "-h" || a == "--help") {
      std::cout << kUsage;
      return 0;
    } else if (a == "-v" || a == "--version") {
      for (const auto& l : cact::licenseNotice()) std::cout << l << "\n";
      return 0;
    } else if (a == "-e" || a == "--eval") {
      evals.push_back(need("an expression"));
    } else if (a == "--repl") {
      repl = true;
    } else if (a == "--ascii") {
      opt.ascii = true;
    } else if (a == "--no-mouse") {
      opt.mouse = false;
    } else if (a == "--no-splash") {
      noSplash = true;
    } else if (a == "--theme") {
      opt.theme = need("a theme name");
    } else if (a == "--fresh") {
      fresh = true;
    } else if (a == "--state") {
      statePath = need("a file name");
    } else {
      std::cerr << "cactulator: unknown option '" << a << "' (try --help)\n";
      return 2;
    }
  }

  cact::Session session;
  if (statePath.empty()) statePath = cact::Session::defaultStatePath();
  bool loaded = false;
  if (!fresh) loaded = session.load(statePath);
  if (!loaded) session.installExamples();

  if (!evals.empty()) {
    int status = 0;
    session.in.onPrint = [](const std::string& line) { std::cout << line << "\n"; };
    for (const auto& e : evals) {
      size_t before = session.history.size();
      session.submit(e);
      for (size_t i = before; i < session.history.size(); ++i) {
        const auto& h = session.history[i];
        for (const auto& l : h.lines) {
          if (h.kind == cact::HistoryItem::Kind::Error) {
            std::cerr << "Error: " << l << "\n";
            status = 1;
          } else {
            std::cout << l << "\n";
          }
        }
      }
    }
    return status;
  }

  bool tty = CACT_ISATTY(CACT_STDIN) && CACT_ISATTY(CACT_STDOUT);
  if (repl || !tty) {
    int rc = cact::runRepl(session, tty);
    if (!fresh) session.save(statePath);
    return rc;
  }

  opt.loadState = !fresh;
  opt.statePath = statePath;
  opt.splash = !noSplash && session.prefs["splash"] != "off";
  return ui::runTui(session, opt);
}
