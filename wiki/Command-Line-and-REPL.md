# Command Line and REPL

Cactulator has three ways to run: the full-screen interface, one-shot evaluation with `-e`, and a plain line-by-line REPL.

## Options

```text
cactulator [options]

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
  -v, --version       show the version and license notice
```

## One-shot evaluation: `-e`

```bash
cactulator -e "inv([[1,2],[3,4]])" -e "solve(x^2 = 2, x)"
```

Each `-e` is evaluated in order in the same session, so later expressions can use earlier definitions:

```bash
cactulator -e "r = 0.05" -e "1000*(1 + r)^10"
```

* Results go to standard output; errors go to standard error prefixed with `Error:`.
* The exit status is `0` if everything succeeded and `1` if any expression failed — handy in scripts.
* Your saved variables and functions are loaded (unless `--fresh`), but nothing is saved back.

## The REPL

The REPL starts automatically when standard input or output is not a terminal, or explicitly with `--repl`:

```bash
echo "5 km to mi" | cactulator
```

```bash
cactulator --repl
```

* Each line is evaluated like the HOME input line; results are printed indented, `print` output unindented, errors as `Error: …`.
* Multi-line blocks (`prog … end`, `if … end`, `while`, `for`, `repeat … until`) and unclosed brackets or strings continue on the next line (the interactive prompt changes from `>` to `…`).
* All [HOME commands](HOME-Commands) that change settings work; commands that would open an app print *(that opens a screen in the full interface)*.
* `rpn` switches to [RPN mode](RPN-Mode); the stack is printed after each line.
* `input()` reads the answer from the next input line.
* State is saved on exit unless `--fresh` was given.

### Scripting examples

Evaluate a file of calculations:

```bash
cactulator --fresh < calculations.txt
```

Use a separate state file for a project, so its variables and programs don't mix with your personal ones:

```bash
cactulator --state ./project-calc.txt
```

Generate a table in a shell pipeline:

```bash
printf 'for k = 1 to 5 do print k, k^2 end\n' | cactulator --fresh
```

## Choosing the state file

From highest to lowest priority:

1. `--state FILE`
2. `$CACTULATOR_HOME/state.txt`
3. `%APPDATA%\Cactulator\state.txt` (Windows)
4. `$XDG_CONFIG_HOME/cactulator/state.txt`
5. `~/.config/cactulator/state.txt`
6. `%USERPROFILE%\.cactulator\state.txt`

See [Saved State](Saved-State) for the file format.
