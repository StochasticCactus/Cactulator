# HOME Commands

Besides expressions, the HOME input line understands a few commands. They are not case-sensitive. In [RPN mode](RPN-Mode) they work the same way.

## Settings

| Command | Does |
|---|---|
| `deg` / `rad` | angles in degrees / radians (also **F9**) |
| `fix n` | show `n` decimals (0–15; `fix` alone means `fix 4`) |
| `sci n` | scientific notation with `n` decimals: `sci 2` shows 12345 as `1.23E4` |
| `eng n` | engineering notation (powers of 1000): `eng 2` shows `12.35E3` |
| `std` (or `float`) | standard format, up to 12 significant digits |
| `frac` | toggle exact fractions: `0.75` → `3/4`, `pi/2` → `π/2` (`frac on`, `frac off` also work) |
| `hex` / `bin` / `oct` / `dec` | show whole-number results in base 16 / 2 / 8 / 10 |
| `rpn` / `alg` | switch to RPN / algebraic entry (also **F10**) |
| `theme name` | `Classic`, `Cactus`, `Retro LCD`, `Midnight` or `Paper` (also **Ctrl-T** to cycle) |
| `keypad` | show or hide the on-screen keypad (also **Ctrl-K**) |

See [Modes and Settings](Modes-and-Settings) for what each setting affects.

## Memory

| Command | Does |
|---|---|
| `vars` | list all variables, functions and programs |
| `del name` (or `delete name`) | delete one variable, function or program |
| `clearvars` (or `clear vars`) | delete all variables (functions and programs stay) |
| `cls` (or `clear`) | clear the HOME history display (also **Ctrl-L**) |

## Graphing

| Command | Does |
|---|---|
| `plot f, g, …` (or `graph …`) | graph up to 6 functions of `x` and open GRAPH; a leading `y=` is ignored |
| `polar r` | graph one polar curve `r(t)`, e.g. `polar 1 + cos(t)` |
| `param x, y` | graph one parametric curve, e.g. `param cos(3t), sin(2t)` |
| `plot` / `graph` alone | open GRAPH as it was |

## Opening apps

| Command | Opens |
|---|---|
| `help` (or `?`), `help name` | HELP; with a name, the Catalog entry for it |
| `plot`, `graph` | GRAPH |
| `matrix` / `mat`, `matrix B` | MATRIX, optionally editing a given matrix |
| `stat` / `stats` / `data` | STAT |
| `prog` / `program`, `edit name` | PRGM, optionally editing a program |
| `solve` / `solver` | SOLVE |
| `units` / `convert` | UNITS |
| `mode` / `settings` | MODE |

## Other

| Command | Does |
|---|---|
| `solve x^2 = 2` | shorthand for `solve(x^2 = 2)` |
| `value unit to unit` | unit conversion, e.g. `5 km to mi` — see [Unit Conversion](Unit-Conversion) |
| `license` (or `about`, `version`, `warranty`) | copyright and warranty notice |
| `quit` (or `exit`, `bye`, `q`) | save and quit |

In the plain [REPL](Command-Line-and-REPL), commands that would open an app print *(that opens a screen in the full interface)* instead.
