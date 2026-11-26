# Expressions and Syntax

This page describes exactly what you can type on HOME (in algebraic mode), in the GRAPH Y= list, in the SOLVE app and inside programs. They all share the same language.

## Numbers

| You type | Meaning |
|---|---|
| `42`, `3.14`, `.5` | ordinary decimal numbers |
| `2E3`, `1e-9`, `6.02e23` | scientific notation (`2E3` = 2000) |
| `0xFF`, `0b1011`, `0o17` | hexadecimal, binary and octal integers |
| `4i`, `3+4i`, `2.5i` | imaginary and complex numbers (`i` right after a number) |
| `pi` or `π`, `e`, `i`, `phi`, `inf` or `∞`, `nan` | built-in constants |
| `_c`, `_h`, `_NA`, `_R` … | physical constants — see [Function Reference](Function-Reference#constants) |
| `"text"` | a string (used by `print`, `input`, `convert`, `str`) |

Numbers are stored as double-precision complex values. Results display with up to 12 significant digits by default (see [Modes and Settings](Modes-and-Settings)).

## Operators

From lowest to highest precedence:

| Precedence | Operators | Notes |
|---|---|---|
| 1 (lowest) | `=` | definition, assignment or equation (see below) |
| 2 | `or` (also `\|\|`) | logical OR |
| 3 | `and` (also `&&`) | logical AND |
| 4 | `not` | logical NOT |
| 5 | `==` `!=` `<` `>` `<=` `>=` (also `≠` `≤` `≥` `<>`) | comparisons give `1` (true) or `0` (false) |
| 6 | `+` `-` | addition, subtraction |
| 7 | `*` `/` `%` `\`, and **implicit multiplication** | `%` is the remainder; `a \ b` is *left division* `b / a` (for matrices: solve `A·x = b`) |
| 8 | unary `-` and `+` | `-2^2` is `-4` |
| 9 | `^` (also `**`) | power, **right-associative**: `2^3^2` = `2^9` = 512 |
| 10 (highest) | postfix `!` `'` `²` `³` `°` `⁻¹`, indexing `[ ]` | factorial, transpose, square, cube, degrees, inverse |

Unicode alternatives are accepted everywhere: `×` and `·` for `*`, `÷` for `/`, `−` for `-`, `√` for `sqrt`, `→` for `->`.

### Implicit multiplication

You can leave out `*` before a name, a parenthesis or `√`:

```text
2x        3(4 + 1)        2pi        x y        2√3        (a+b)(a-b)
```

Implicit multiplication has the **same** precedence as `*` and `/` and is evaluated left to right, so `1/2x` means `(1/2)·x`, not `1/(2x)`. Use parentheses when in doubt.

### Functions without parentheses

A function name followed directly by a number or name takes just that one factor: `sin 30` is `sin(30)`, and `sin 30 + 1` is `sin(30) + 1`.

### Remainder

`a % b` and `mod(a, b)` always return a result with the sign of `b`: `-7 % 3` is `2`.

## Angles

Trig functions read and return angles in the current mode: **DEG** (the default) or **RAD**. Toggle with **F9** or the commands `deg` / `rad`.

The postfix `°` converts a number from degrees to the current mode, so `sin(30°)` is `0.5` in either mode. Conversions: `todeg(x)`, `torad(x)`, or units (`90 deg to rad`).

In DEG mode, multiples of 30° and 45° are exact: `sin(30)` gives exactly `0.5` and `cos(90)` exactly `0`.

## Variables, functions and assignment

```text
x = 5                  store 5 in x
5 -> x                 the same, HP-style store (also 5 → x)
f(x) = x^2 + 1         define a one-line function
area(w, h) = w*h       functions can take several parameters
v[3] = 7               set item 3 of a list (the list grows as needed)
A[2, 1] = 7            set an entry of a matrix
```

* Names start with a letter or `_` and may contain letters, digits and `_`. Non-ASCII letters such as `θ` work too.
* Names are case-sensitive: `A` and `a` are different.
* Built-in constants and function names can't be reused for variables (`pi = 3` is an error).
* A variable and one of your functions can't share a name: assigning `f = 2` replaces the function `f`.
* `ans` always holds the last result.
* `vars` lists everything; `del name` deletes one thing; `clearvars` deletes all variables.

### `=` is context-dependent

* `name = expr` and `f(params) = expr` **define** things.
* Inside `solve(…)`, the SOLVE app, or anywhere it is an argument, `left = right` is an **equation**: `solve(x^2 = 2, x)`.
* Inside an `if`/`while` condition, a single `=` also works as a comparison.

## Lists and matrices

```text
[1, 2, 3]              a list (a vector)
[]                     an empty list
[[1, 2], [3, 4]]       a 2×2 matrix (rows inside brackets)
[1, 2; 3, 4]           the same matrix, rows separated by ;
v[2]                   item 2 of a list — counting starts at 1
A[2, 1]                row 2, column 1 of a matrix
A[2]                   row 2 of a matrix, as a list
"hello"[2]             character 2 of a string
```

Arithmetic works the way you'd expect from linear algebra:

| Expression | Result |
|---|---|
| `[1, 2] + [3, 4]` | element-wise sum `[4, 6]` |
| `[1, 2, 3] * 2` | scaling `[2, 4, 6]` |
| `A * B` | matrix product (sizes must match) |
| `A * [1, 1]` | matrix × vector |
| `A^3`, `A^-1`, `A⁻¹` | matrix powers and inverse |
| `A'` | transpose |
| `A / B` | `A · inv(B)` |
| `A \ b` | solves `A·x = b` |

Most scalar functions apply element-wise: `sqrt([4, 9])` → `[2, 3]`, `sin([0, 90])` → `[0, 1]`.

Comparisons between lists or matrices compare the whole value: `[1, 2] == [1, 2]` is `1`.

## Complex numbers

Complex numbers are first-class. Anything that would leave the real line simply becomes complex:

```text
sqrt(-4)           → 2i
(2+3i)*(1-i)       → 5+i
abs(3+4i)          → 5
arg(1+i)           → 45            (DEG mode)
log(-1)            → 1.36437635384i
polyroots([1, 2, 5]) → [-1-2i, -1+2i]
```

See `re`, `im`, `conj`, `arg`, `polar`, `rect` in the [Function Reference](Function-Reference#complex-numbers).

## Strings

Strings are written in double quotes and support `\n`, `\t` and `\"`. They can be joined with `+`: `"x = " + str(5)` → `x = 5`. They are mainly useful in programs with `print` and `input`, and as unit names in `convert(3, "cup", "mL")`.

## Continuing from the last answer

On HOME, a line that **starts** with `+ * / ^ ² ³ ⁻¹ !` (or `×`, `÷`) continues from `ans`: `*2` means `ans*2`. A line starting with `-` is a negative number, not a continuation.

## Unit conversion shortcut

On HOME you can write `value unit to unit`, e.g. `5 km to mi`, `100 F to C`, `(3+4) ft to m`. See [Unit Conversion](Unit-Conversion).

## Several statements on one line

Separate statements with `;`: `a = 2; b = 3; a*b`. Inside `[ ]` the `;` separates matrix rows instead.

## Error messages

Errors point at the problem and often suggest a fix:

```text
sinn(3)        → unknown function 'sinn' — did you mean 'sin'?
f(1, 2)        → f expects 1 argument: f(x)
2*             → Syntax error: the expression is incomplete
inv([[1,2],[2,4]]) → this matrix is singular (its determinant is 0), so it has no inverse
```
