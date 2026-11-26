# Installation

Cactulator is built from source. You need:

* a **C++17 compiler** (GCC, Clang or MinGW)
* **ncurses with wide-character support** (`ncursesw`; macOS's system ncurses already has it)
* **make** or **CMake 3.14+**

There are no other dependencies.

## Linux / WSL

```bash
sudo apt install build-essential libncurses-dev
```

On Fedora use `sudo dnf install gcc-c++ ncurses-devel`; on Arch, `sudo pacman -S base-devel ncurses`.

```bash
git clone https://github.com/StochasticCactus/Cactulator.git
```

```bash
cd Cactulator && make
```

```bash
./cactulator
```

## macOS

ncurses ships with the system; you only need the command-line tools.

```bash
xcode-select --install
```

```bash
make && ./cactulator
```

## Windows (MSYS2)

Install [MSYS2](https://www.msys2.org), open the **MSYS2 UCRT64** terminal, then:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ncurses make
```

```bash
cmake -B build -G "MSYS Makefiles" && cmake --build build
```

```bash
./build/cactulator.exe
```

Run it inside **Windows Terminal** for the best colors and Unicode support.

## Android (Termux)

```bash
pkg install clang make ncurses
```

```bash
make && ./cactulator
```

## Building with CMake (any platform)

```bash
cmake -B build && cmake --build build
```

```bash
./build/cactulator
```

CMake builds a static library `cactcore` (the math engine), the `cactulator` executable and the `test_core` test program. The default build type is `Release`.

## Installing system-wide

With make (defaults to `/usr/local/bin`; override with `PREFIX=` or `DESTDIR=`):

```bash
sudo make install
```

With CMake:

```bash
sudo cmake --install build
```

## Running the tests

```bash
make test
```

or, after a CMake build:

```bash
ctest --test-dir build --output-on-failure
```

You should see something like `154 passed, 0 failed`.

## Build options

The Makefile honors the usual variables:

| Variable | Default | Use |
|---|---|---|
| `CXX` | `c++` | the compiler |
| `CXXFLAGS` | `-O2` | extra flags (`-std=c++17 -Wall -Wextra -Isrc` are always added) |
| `CURSES_LIBS` | `pkg-config --libs ncursesw`, or `-lncursesw` (`-lncurses` on macOS) | how to link curses |
| `PREFIX` | `/usr/local` | install prefix |

For example, a debug build with Clang:

```bash
make CXX=clang++ CXXFLAGS="-g -O0"
```

Object files go into `build-make/`; `make clean` removes them and the binary.

## Continuous integration

Every push and pull request is built by GitHub Actions (`.github/workflows/build.yml`) on Ubuntu, macOS and Windows (MSYS2 UCRT64). Each job builds the program, runs the core tests and runs a short smoke test of `-e` and piped REPL input.

## Next

Continue with [Getting Started](Getting-Started). If something doesn't build or looks wrong on screen, see [Troubleshooting and FAQ](Troubleshooting-and-FAQ).
