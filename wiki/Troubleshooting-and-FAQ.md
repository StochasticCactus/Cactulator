# Troubleshooting and FAQ

## Building

**`fatal error: curses.h: No such file or directory`** (or `ncursesw/…`)
Install the ncurses development package: `libncurses-dev` (Debian/Ubuntu), `ncurses-devel` (Fedora), `ncurses` (Arch, Termux, MSYS2 `mingw-w64-ucrt-x86_64-ncurses`). See [Installation](Installation).

**Linker errors like `undefined reference to 'wget_wch'` or `'setcchar'`**
You are linking the narrow-character ncurses. Make sure the wide version (`ncursesw`) is installed. With make you can force the library: `make CURSES_LIBS=-lncursesw`.

**`error: 'optional'/'filesystem' is not a member of 'std'`**
Your compiler is too old or not in C++17 mode. Use a compiler with full C++17 support including `<filesystem>`, such as GCC 9 or newer, a recent Clang or Xcode, or a current MinGW from MSYS2.

## Display

**Boxes, braille graphs or symbols show as `?` or empty squares**
Your terminal font lacks those characters. Use a font with good Unicode coverage (e.g. DejaVu Sans Mono, Cascadia Code, JetBrains Mono, Fira Code), make sure the terminal uses UTF-8 (`echo $LANG` should end in `UTF-8`), or run `cactulator --ascii` to draw with plain ASCII only.

**Colors look wrong or washed out**
Cactulator uses 256 colors when available and falls back to 16 or 8. Check that `TERM` is something like `xterm-256color`. Try another theme with **Ctrl-T**; *Paper* works well on light terminals.

**The keypad doesn't appear**
It only appears automatically when the window is large enough. Enlarge the window, press **Ctrl-K**, or set *Keypad: Always* in MODE.

**F-keys don't work** (common in some terminals, tmux/screen, or on laptops)
Many terminals or laptops send F-keys only with **Fn** held. Inside tmux, make sure `TERM` inside tmux is `tmux-256color` or `screen-256color`. Everything is also reachable without F-keys: click the soft keys with the mouse, or type commands like `plot`, `matrix`, `stat`, `prog`, `solve`, `units`, `mode`, `rad`, `rpn` on HOME.

**On Windows, characters are garbled**
Run Cactulator inside **Windows Terminal** from the MSYS2 UCRT64 environment. The program switches the console to UTF-8 itself.

## Mouse

**I can't select text with the mouse**
Cactulator captures the mouse so you can click keys and curves. Hold **Shift** while selecting (in most terminals), or start with `cactulator --no-mouse`.

## Calculations

**`sin(90)` gives 0.893…, not 1**
You are in RAD mode. Press **F9** or type `deg`. The top line shows the current mode.

**`integ(sin(x), x, 0, 180)` gives 114.59 instead of 2**
In DEG mode the integration variable is measured in degrees. Switch to RAD for calculus. See [Solving and Calculus](Solving-and-Calculus#angles-and-calculus).

**`1/2x` gives x/2, not 1/(2x)**
Implicit multiplication has the same precedence as `/` and is evaluated left to right. Write `1/(2x)`. See [Expressions and Syntax](Expressions-and-Syntax#implicit-multiplication).

**`-2^2` is −4**
The power binds tighter than the minus sign, as in standard math notation. Write `(-2)^2` for 4.

**`solve` returns the "wrong" solution**
Equations can have several solutions; `solve` returns the one nearest its starting guess. Give a guess as the third argument: `solve(x^2 = 2, x, -1)`. If the variable already has a value, that value is used as the guess.

**`no solution found (try a different starting guess)`**
The solver didn't find a sign change or a minimum of |f| near the guess. Graph the expression to see where the roots are, then try a guess close to one.

**`the series does not seem to converge`** for a series that does converge
Infinite sums stop after about two million terms. Slowly converging series like `sum(1/k^2, k, 1, inf)` need more than that; use a large finite upper limit instead: `sum(1/k^2, k, 1, 1000000)`.

**`'sum' is a built-in function; pick another name`**
Variable names can't reuse built-in names or aliases (`sum`, `count`, `len`, `mean`, `max`…) or constants (`pi`, `e`, `i`). Choose another name.

**Results like `0.30000000000000004`?**
You won't see these: results are shown with 12 significant digits by default and tiny floating-point residues are cleaned up for display. Values are still IEEE double precision internally.

**A program runs forever**
Press **Esc** to stop it. In the plain REPL, press **Ctrl-C** (this ends the REPL).

## State

**My variables disappeared**
Did you start with `--fresh` or a different `--state` file, or change `CACTULATOR_HOME`? See [Saved State](Saved-State) for where the file lives.

**How do I reset everything?**
Quit and delete the state file (`~/.config/cactulator/state.txt` on Linux/macOS).

## Known issues

* **The bundled `primes` example fails** with *'count' is a built-in function; pick another name (line 4)*, because `count` is an alias of `len`. Fix your copy with `edit primes` and rename the variable `count` (for example to `found`) everywhere in the program.
* **`sum(…, k, 1, inf)` gives up on slowly converging series** — see above. The in-app *Quick recipes* and the README use `sum(1/k^2, k, 1, inf)` as an example, which currently hits this limit.
* **In RPN mode, typing `clear` clears the screen, not the stack.** `clear` is the HOME command for clearing the history; use `clr` to empty the stack.

Found something else? Please [open an issue](https://github.com/StochasticCactus/Cactulator/issues).
