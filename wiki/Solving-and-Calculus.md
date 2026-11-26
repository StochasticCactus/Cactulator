# Solving and Calculus

Everything on this page is numerical: Cactulator finds numbers, not formulas.

## Solving equations: `solve`

```text
solve(x^2 = 2, x)                 → 1.41421356237
solve(x^2 = 2, x, -1)             → -1.41421356237   (start looking near -1)
solve(x^3 - 2x - 5, x)            → an expression alone means "= 0"
solve x^2 = 2                     → shorthand on HOME (the variable is detected)
```

* The third argument is a **starting guess**. Without one, `solve` starts from the variable's current value if it has one, otherwise from 1. When an equation has several solutions, the guess decides which one you get.
* `solve` does **not** change the variable: after `solve(x^2 = 2, x)`, `x` keeps its old value. The answer is in `ans`.
* Equations may use any of your variables and functions: `solve(f(x) = 3, x)`.

How it works: Newton's method with a numerical derivative first; if that fails, it scans outwards from the guess for a sign change and finishes with Brent's method; finally it tries minimizing |f| to catch "touching" roots like `x^2 = 0`. If nothing works you get *no solution found (try a different starting guess)*.

### Linear systems

With a matrix and a list, `solve` solves `A·x = b`:

```text
solve([[2, 1], [1, 3]], [3, 5])   → [0.8, 1.4]
[[2, 1], [1, 3]] \ [3, 5]         → the same
```

## Polynomial roots: `polyroots`

Give the coefficients, highest power first. All roots are returned, complex ones included:

```text
polyroots([1, -3, 2])             → [1, 2]          x² − 3x + 2
polyroots([1, -6, 11, -6])        → [1, 2, 3]
polyroots([1, 2, 5])              → [-1-2i, -1+2i]
polyroots([1, 0, 0, -1])          → the three cube roots of 1
```

Roots are found simultaneously with the Durand–Kerner (Weierstrass) iteration. `polyval(coeffs, x)` evaluates a polynomial; `polyfit` fits one to data (see [Statistics](Statistics-and-Probability#regression)).

## The SOLVE app

Open with **F6** on HOME (or `solve`). It has two modes.

### Equation solver (HP style)

Type an equation with any number of variables, e.g. `P*V = n*_R*T`. Cactulator lists every variable it contains. Give values to all but one, select the unknown and press **F1 Solve**:

```text
P*V = n*_R*T      P = 101325   n = 1   T = 273.15   V = ?   → V = 0.0224…
```

* If exactly one variable has no value, it is solved for automatically; otherwise select the one you want.
* The current value of the unknown is used as the starting guess — set it near the answer you want.
* The result is stored in the variable and in `ans`, and the residual (left − right) is shown so you can check it.
* **F4 Example** cycles through sample equations (`x^2 - 5x + 6 = 0`, `A = P*(1 + r/100)^t`, `a^2 + b^2 = c^2`, …); **F5 Clear** clears the variables' values.

### Polynomial roots

**F3 Poly** switches to polynomial mode: type the coefficients separated by commas (`1, -3, 2`) and the roots appear as you type. They are also stored in `ans`. **F2 Equation** goes back.

## Derivatives: `deriv`

```text
deriv(x^3, x, 2)                  → 12      d/dx x³ at x = 2
deriv(sin(x), x, pi/4)            → slope at π/4 (RAD mode)
```

If you leave out the point, the current value of the variable is used. The derivative is computed with Ridders' method (Richardson extrapolation of central differences), which is typically accurate to 10+ digits for smooth functions.

## Integrals: `integ`

```text
integ(x^2, x, 0, 3)               → 9
integ(exp(-x^2), x, -inf, inf)    → 1.77245385091  (√π)
integ(1/x^2, x, 1, inf)           → 1
```

* Limits can be `inf` or `-inf`; infinite ranges are mapped to finite ones by a change of variables.
* The method is adaptive 15-point Gauss–Kronrod quadrature, which subdivides the interval where the error is largest.
* A divergent integral, or one with a singularity inside the range, gives an error instead of a wrong number: `integ(1/x, x, 0, 1)` → *the integral diverges, or the function is undefined somewhere in the interval*.

## Minimum and maximum: `fmin`, `fmax`

They return the **x** where an expression is smallest or largest on an interval:

```text
fmin((x-2)^2, x, 0, 5)            → 2
fmax(sin(x), x, 0, 180)           → 90        (DEG mode)
```

Evaluate the expression there to get the value itself. In GRAPH, **F4 → Min/Max** does the same on a curve.

## Sums and products

```text
sum(k^2, k, 1, 10)                → 385
prod(k, k, 1, 5)                  → 120
sum(1/2^k, k, 0, inf)             → 2
sum(1/fact(k), k, 0, inf)         → 2.71828182846
```

With `inf` as the upper limit, terms are added until they stop changing the total. This works well for series that converge quickly (geometric, factorial, …). Series that converge slowly, such as `1/k^2` or alternating series, may stop after about two million terms with *the series does not seem to converge*; use a large finite limit instead, e.g. `sum(1/k^2, k, 1, 1000000)` → `1.64493306685`.

Without the extra arguments, `sum(list)` and `prod(list)` add or multiply the items of a list.

## Piecewise functions

```text
piecewise(x < 0, -x, x^2)
piecewise(x < -1, -1, x > 1, 1, x)    clamp to [-1, 1]
```

Conditions are checked in order and the value of the first true one is used; the last argument is the "otherwise" value.

## Angles and calculus

The variable of `deriv`, `integ`, `fmin` and `fmax` is just a number, but trig functions interpret it in the current angle mode. In DEG mode, `integ(sin(x), x, 0, 180)` is `114.59…` (that is, 360/π), not `2`. Switch to **RAD** (F9) for textbook calculus.
