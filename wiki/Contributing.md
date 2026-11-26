# Contributing

Contributions are welcome: bug reports, new functions and units, documentation, and fixes. This page explains how the code is organized for the most common changes. Read [Architecture](Architecture) first for the big picture.

## Development setup

```bash
git clone https://github.com/StochasticCactus/Cactulator.git
```

```bash
cd Cactulator && make -j4 && make test
```

`make` puts object files in `build-make/` and tracks header dependencies, so incremental rebuilds are fast. A debug build:

```bash
make clean && make CXXFLAGS="-g -O0 -fsanitize=address,undefined"
```

Useful while developing:

* `./cactulator --fresh` — test without touching your own saved state.
* `./cactulator --fresh -e "expr"` or `printf '…' | ./cactulator --fresh` — check engine behavior without the UI.
* `./cactulator --state /tmp/test-state.txt` — work against a throwaway state file.

## Code style

* C++17, no dependencies beyond the standard library and ncursesw.
* Builds warning-free with `-Wall -Wextra` on GCC, Clang and MinGW.
* Two-space indentation, `camelCase` functions and variables, `PascalCase` types, `trailing_` underscores for private members — follow the surrounding code.
* `src/core` must not include anything from `src/ui` or ncurses.
* Errors meant for the user are thrown as `cact::CalcError` with a plain-language message that says what to do.
* New source files start with the GPL-2.0-or-later header used by every existing file.
* New core `.cpp` files must be added to `CORE_SOURCES` in `CMakeLists.txt` (the Makefile picks them up automatically); new UI files to the `cactulator` target.

## Adding a built-in function

All built-ins are registered in `src/core/builtins.cpp`, together with the help text the Catalog shows. Pick the right helper:

```cpp
// A scalar function, automatically applied element-wise to lists and matrices:
R.scalar("sinc", TRIG, "sinc(x)", "sin(x)/x, with sinc(0) = 1 (x in radians).", "sinc(0) = 1",
         [](Interp&, cplx z) { return z == cplx(0) ? cplx(1) : std::sin(z) / z; });

// Any number of evaluated arguments (min 2, max 2 here; use -1 for "any number"):
R.add("hypot", 2, 2, BASIC, "hypot(a, b)", "Length of the hypotenuse: √(a² + b²).", "hypot(3, 4) = 5",
      [](Interp&, Args& a) { return Value(std::hypot(r1(a[0]), r1(a[1]))); });

// A "lazy" function receives its arguments unevaluated, for things like integ(expr, x, a, b):
R.lazy("integ", 4, 4, CALC, "integ(expr, x, a, b)", "…", "…", lazyInteg);

// Another name for an existing function:
R.alias("arcsin", "asin");
```

* The category constants (`BASIC`, `TRIG`, `CPLX`, `NUM`, `STAT`, `PROB`, `MAT`, `CALC`, …) (and `UNIT`, `PROG`, `RAND`) are defined at the top of `buildRegistry()`; they become the Catalog groups.
* `r1(value)` extracts a real number and throws a friendly error for complex numbers, lists or strings.
* Angle-dependent functions must use `in.toRadians()` / `in.fromRadians()` so they honor DEG/RAD.
* Write the **signature**, **help** and **example** for a beginner — they appear in the Catalog, the status-line hint while typing, and the [Function Reference](Function-Reference).

New names automatically become reserved (users can't assign to them), get Tab completion, syntax highlighting and "did you mean…?" suggestions.

## Adding a constant

Constants live in the `constantList()` table at the end of `builtins.cpp`:

```cpp
{"_c", "Physics", "speed of light in vacuum, 299 792 458 m/s", 299792458.0},
```

Physical constants start with `_` so they never collide with user variables.

## Adding a unit

Units are defined in `unitCategories()` in `src/core/units.cpp`. Each unit has a name, a description and a factor to the category's base unit (and an offset for temperatures):

```cpp
{"nmi", "nautical mile", 1852},
```

Friendly spellings go into `findUnit()`'s alias list. Units can only be converted within the same category. Remember to update [Unit Conversion](Unit-Conversion).

## Adding a HOME command

Commands are handled in `Session::command()` in `src/core/session.cpp`. Return `true` when the line was a command. To open an app, set `out.action` to a `UiAction`; the TUI performs it and the REPL prints a note. Also add the command to the Catalog list in `src/ui/help_screen.cpp` and to the *Keys* help text.

## Adding a screen

1. Implement `ui::Screen` in a new `src/ui/xxx_screen.cpp` and expose `makeXxxScreen(App&)`.
2. Add a `ScreenId` (in `app.hpp`) and push the screen in the `App` constructor in the same order as the `ScreenId` values.
3. Add a way to open it: a soft key or keypad key, and optionally a `UiAction` and HOME command.

## Tests

The engine test suite is `tests/test_core.cpp`. It drives a `Session` exactly like a user typing on HOME and compares the displayed output:

```cpp
expect(s, "hypot(3, 4)", "5");                       // exact displayed text
expectNear(s, "solve(x^2 = 2, x)", std::sqrt(2.0));  // numeric, with a tolerance
expectError(s, "inv([[1,2],[2,4]])", "singular");    // an error containing this text
```

Add tests next to the related section (*Arithmetic*, *Matrices*, *Solving and calculus*, *Programs*, *RPN*, *Save / load*, …) and run:

```bash
make test
```

The UI has no automated tests; please try your change in a real terminal, ideally also with `--ascii` and a light theme (`--theme Paper`).

## Continuous integration

GitHub Actions builds every push and pull request on Ubuntu, macOS and Windows (MSYS2), runs the core tests, and smoke-tests `-e` and the piped REPL. Please make sure `make test` passes locally first.

## Documentation

* User-facing behavior is documented in three places: the in-app help (`src/ui/help_screen.cpp` and the built-in help strings), the `README.md`, and this wiki. Please keep them consistent.
* The wiki pages live in the `wiki/` folder of the main repository and are published to the GitHub wiki automatically when they change on `main` (see `.github/workflows/wiki.yml`). Edit them there, through a pull request, rather than in the wiki web editor — web edits are overwritten on the next publish.
* `wiki/Function-Reference.md` mirrors the Catalog; when you add or change a built-in, update its row.

## License

Cactulator is licensed under the **GNU General Public License, version 2 or (at your option) any later version**. By contributing you agree that your contribution is licensed the same way.
