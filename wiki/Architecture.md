# Architecture

Cactulator is about 14,000 lines of C++17 split into two layers: a **math engine** with no UI dependencies (`src/core`, built as the static library `cactcore`) and an **ncurses interface** on top of it (`src/ui`). The REPL and the `-e` mode use the engine directly.

```text
                 ┌─────────────────────────── src/main.cpp ───────────────────────────┐
                 │ parse options · load state · pick a front end                      │
                 └───────┬───────────────────────────┬────────────────────────┬───────┘
                         │                           │                        │
                 -e EXPR │                 piped / --repl                full screen
                         ▼                           ▼                        ▼
                  Session::submit          src/repl.cpp: runRepl     src/ui: runTui → App
                         │                           │                        │
                         └─────────────┬─────────────┴────────────────────────┘
                                       ▼
                         cact::Session  (src/core/session.*)
                commands · history · graph state · RPN stack · save/load
                                       │
                ┌──────────────────────┼─────────────────────────┐
                ▼                      ▼                         ▼
        Lexer → Parser → AST     Interp (evaluator)        Rpn (stack machine)
                                       │
          builtins · linalg · numeric · units · format · value
```

## Source map

### `src/core` — the engine

| File | Responsibility |
|---|---|
| `value.*` | `Value`: a tagged union of number (`std::complex<double>`), matrix (row-major `Matrix` of complex; lists are n×1 matrices with a `vec` flag) and string. `CalcError` is the single exception type for user-facing errors. |
| `lexer.*` | Turns text into tokens: numbers (decimal, `0x`/`0b`/`0o`, exponents, imaginary `4i`), strings, identifiers (including UTF-8 letters), and operators, mapping Unicode symbols (`×`, `÷`, `π`, `≤`, `→`, …) to their ASCII equivalents. |
| `ast.hpp` | Expression nodes (`Node`, kinds in `NK`) and statement nodes (`Stmt`, kinds in `SK`). |
| `parser.*` | Recursive-descent parser for statements (`if`, `while`, `for`, `repeat`, `prog`, `return`, `print`, `global`, assignments, definitions) and expressions (precedence climbing — see [Expressions and Syntax](Expressions-and-Syntax#operators)). Also `needsMoreInput()` for the REPL's multi-line blocks. |
| `interp.*` | The tree-walking evaluator: variable scopes (frames), user functions and programs, binary operators on numbers/matrices/strings, indexing, angle-mode helpers, interrupt polling (`tick()`), and "did you mean…?" suggestions. |
| `builtins.cpp` | Every built-in function and constant **with its help text, signature and example** — the single source of truth for the in-app Catalog. |
| `linalg.*` | Matrix algorithms: multiply, determinant, inverse and linear solve (Gaussian elimination with partial pivoting), RREF, rank, null space, eigenvalues (Householder → Hessenberg → shifted QR) and eigenvectors. |
| `numeric.*` | Root finding (Newton → bracketing + Brent → minimization), derivatives (Ridders), integration (adaptive Gauss–Kronrod 15 with infinite-range mapping), minimization, polynomial roots (Durand–Kerner), statistics, regressions and probability distributions. |
| `units.*` | The unit table (categories of units with factor and offset) and conversions. |
| `format.*` | Display formatting: STD/FIX/SCI/ENG, fractions and multiples of π, bases, complex numbers, multi-line matrix rendering, UTF-8 helpers. |
| `rpn.*` | The RPN stack machine with undo; each line is processed atomically. |
| `session.*` | Ties it together: HOME commands, the `value unit to unit` shortcut, `ans` continuation, history, graph state, RPN mode, example programs, and the state file. |

### `src/ui` — the interface

| File | Responsibility |
|---|---|
| `term.*` | A thin ncursesw wrapper: wide-character output, 256/16/8-color mapping, key and mouse events, ASCII fallback. |
| `theme.*` | The five color themes (body, LCD, keys, syntax highlighting, graph colors). |
| `canvas.*` | A braille-dot canvas (2×4 dots per cell) used by GRAPH and STAT plots, with an ASCII fallback. |
| `widgets.*` | `LineEdit` (single-line editor), syntax highlighting, text wrapping, drawing helpers. |
| `app.*` | `App`: the main loop, layout (display, soft keys, keypad), global keys, modal prompts, toasts, theme/keypad/angle/RPN toggles, saving. Defines the `Screen` interface. |
| `*_screen.cpp` | One file per app: `calc` (HOME), `graph`, `matrix`, `stat`, `prog`, `solve`, `units`, `help`, `mode`. |

Every app implements `ui::Screen`:

```cpp
class Screen {
  virtual std::string title() const = 0;
  virtual void enter(const std::string& arg);              // opened (arg from e.g. "matrix B")
  virtual void draw(const Rect& r) = 0;                    // draw the LCD content area
  virtual bool key(const Event& e) = 0;                    // true if handled
  virtual bool mouse(const Event& e, const Rect& r);
  virtual void insert(const std::string& text);            // keypad / catalog insertion
  virtual std::vector<SoftKey> softKeys() = 0;             // labels + actions for F1…F8
  virtual std::string hint();
};
```

Screens are created by `makeXxxScreen(App&)` factories and identified by `ScreenId`.

## Life of an input line

Typing `f(x) = x^2` and then `f(3) * 2` on HOME:

1. `CalcScreen` passes the text to `Session::submit`.
2. `Session::command` checks for a HOME command (`deg`, `plot`, `vars`, …). If one matches, it runs and may return a `UiAction` (e.g. *open GRAPH*) for the UI to perform.
3. Otherwise the line is pre-processed: a leading operator becomes `ans<op>…`, and `solve …` without parentheses is wrapped.
4. `unitConversion` tries the `value unit to unit` pattern.
5. Otherwise `Interp::execute` tokenizes and parses the line into statements. `f(x) = x^2` becomes a `FuncDef`; `f(3) * 2` an expression statement.
6. The evaluator walks the AST. Calls are resolved in order: user functions and programs, then built-ins (some built-ins such as `solve`, `integ`, `sum` are *lazy* and receive unevaluated arguments so they can bind their variable). Variables are looked up in local frames, then globals, then constants.
7. The result is stored in `ans`, formatted by `format.cpp` according to the display settings, and appended to the history as a `HistoryItem`.
8. The UI redraws; the session is saved.

## Design notes

* **The engine never touches the terminal.** Printing and input go through `Interp::onPrint` / `onInput` callbacks, and long computations poll `checkInterrupt` via `tick()`, so the same engine serves the TUI, the REPL and the tests.
* **Help lives next to the code.** Each built-in is registered with its category, signature, help and example, and the Catalog and the [Function Reference](Function-Reference) are generated from that table.
* **Everything is complex.** Numbers are `std::complex<double>`, so complex results appear naturally; real-only functions check their arguments and report clear errors.
* **Errors are values the user can read.** `CalcError` messages are written for end users ("this matrix is singular…", "did you mean 'sin'?"), and syntax errors carry a column and, inside programs, a line number.
