# Keyboard and Mouse

## Global keys

These work everywhere:

| Key | Does |
|---|---|
| **F1 … F8** | the soft keys shown at the bottom of the display (they change per app) |
| **F9** | degrees ⇄ radians |
| **F10** | algebraic ⇄ [RPN](RPN-Mode) |
| **Esc** | clear the input line; if it is already empty, go back to HOME |
| **Ctrl-T** | next color theme |
| **Ctrl-K** | show / hide the keypad |
| **Ctrl-Q** | quit (everything is saved) |
| **Ctrl-C** twice | quit |

On HOME the soft keys open the apps: **F1** Help, **F2** Graph, **F3** Matrix, **F4** Stat, **F5** Prog, **F6** Solve, **F7** Units, **F8** Mode.

## Editing a line

| Key | Does |
|---|---|
| **Enter** | calculate / confirm |
| **← →** | move the cursor |
| **Ctrl-A** / **Home** | start of line |
| **Ctrl-E** / **End** | end of line |
| **Ctrl-U** | clear the line |
| **Ctrl-W** | delete the word before the cursor |
| **⌫** / **Delete** | delete a character |
| **Tab** | complete a function or variable name |
| **↑ ↓** | on HOME: earlier / later entries; in apps: move around |
| **PgUp PgDn** | scroll the history or a long page |
| **Ctrl-L** | clear the HOME screen |
| **Ctrl-Z** | undo (RPN) |

While you type, the input is syntax-highlighted, matching brackets are marked, and the line below shows the signature and help of the function under the cursor.

## Mouse

When your terminal supports it, the mouse works too:

* click **keypad keys** and **soft keys**
* click a **result** on HOME to paste it into the input line
* click a **curve** in GRAPH to start tracing it
* the **wheel** scrolls lists and history, and **zooms graphs** around the pointer
* click cells in the MATRIX and STAT grids, rows in MODE, entries in the Catalog

Because Cactulator captures the mouse, normal text selection needs **Shift** held down in most terminals. If you'd rather keep the terminal's own mouse behavior, start with `cactulator --no-mouse`.

## Typing special symbols

You never need Unicode — every symbol has an ASCII spelling:

| Symbol | ASCII |
|---|---|
| `π` | `pi` |
| `√` | `sqrt( )` |
| `²` `³` | `^2` `^3` |
| `⁻¹` | `^-1` or `inv( )` |
| `×` `÷` `·` | `*` `/` |
| `≤` `≥` `≠` | `<=` `>=` `!=` |
| `→` | `->` |
| `°` | `todeg`/`torad`, or switch the angle mode |
| `∞` | `inf` |

The keypad inserts the Unicode versions for you.

## App-specific keys

Each app page lists its own keys:

* [Graphing](Graphing#keys)
* [Matrices](Matrices#keys)
* [Statistics and Probability](Statistics-and-Probability#the-stat-app)
* [Programming](Programming#the-prgm-app)
* [Solving and Calculus](Solving-and-Calculus#the-solve-app)
* [Unit Conversion](Unit-Conversion#the-units-app)
