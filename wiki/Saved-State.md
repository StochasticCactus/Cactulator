# Saved State

Cactulator remembers your work between sessions: variables, functions, programs, the graph setup and window, the RPN stack, your settings and the last 200 history entries. The full-screen interface saves after every calculation on HOME, when you change settings in MODE, when you save or run a program, and when you quit. The REPL saves when it exits.

## Where it lives

| Platform | Default location |
|---|---|
| Linux, macOS, Termux | `~/.config/cactulator/state.txt` (or `$XDG_CONFIG_HOME/cactulator/state.txt`) |
| Windows | `%APPDATA%\Cactulator\state.txt` |

Override it with the `CACTULATOR_HOME` environment variable (the file is `$CACTULATOR_HOME/state.txt`) or for a single run with `--state FILE`. The full priority order is in [Command Line and REPL](Command-Line-and-REPL#choosing-the-state-file).

`--fresh` starts with an empty session and does not save anything — useful for testing and scripts.

## Starting over

Quit Cactulator and delete the file. On the next start the [example programs](Programming#bundled-examples) are installed again.

```bash
rm ~/.config/cactulator/state.txt
```

To clear only the variables, type `clearvars` on HOME. To delete one thing, `del name`.

## File format

It's a plain UTF-8 text file, one item per line, so you can read it, back it up, put it under version control, or edit it carefully while Cactulator is not running. A short example:

```text
# Cactulator state v1 - edit with care
set angle DEG
set fmt 0 4
set frac 0
set base 10
set rpn 0
pref theme Classic
pref keypad auto
var ans = 5
var v = [1,2]
var x = 7
func area(w,h)=w*h
prog prog collatz(n)\n  steps = 0\n  …\nend
graph 0 0 1 sin(x)
graph 1 0 0 cos(x)
window -360 360 -1.2 1.2 0 360 0 0 0
stack 42
hist 0	2+3	5
```

| Line | Meaning |
|---|---|
| `# …` | comment, ignored |
| `set angle DEG\|RAD` | angle mode |
| `set fmt F D` | number format (0 standard, 1 fixed, 2 scientific, 3 engineering) and number of decimals |
| `set frac 0\|1` | fractions off/on |
| `set base 10\|16\|2\|8` | display base for whole numbers |
| `set rpn 0\|1` | algebraic or RPN entry |
| `pref KEY VALUE` | interface preferences: `theme`, `keypad` (`auto`, `show`, `hide`), `splash` (`on`, `off`) |
| `var NAME = VALUE` | a variable; lists and matrices are written in bracket syntax |
| `func SOURCE` | a one-line function, exactly as you typed it |
| `prog SOURCE` | a program; new lines are written as `\n`, tabs as `\t`, backslashes as `\\` |
| `graph SLOT TYPE ON TEXT` | a Y= row: slot 0–5, type 0 function / 1 parametric / 2 polar, enabled 0/1, the expression |
| `window xmin xmax ymin ymax tmin tmax tstep grid user` | the graph window, grid on/off, and whether you set it by hand |
| `stack VALUE` | one RPN stack level, bottom first |
| `hist KIND⇥INPUT⇥OUTPUT` | a history entry (tab-separated; kind 0 result, 1 error, 2 message, 3 print) |

Saving is atomic: Cactulator writes `state.txt.tmp` and renames it over the old file, so a crash can't leave a half-written state.

Lines with unknown keys or invalid values are skipped when loading, so a small mistake in a hand-edited file only loses that line.
