# Function Reference

Every built-in function and constant, generated from the same table that powers the in-app **Catalog** (F1 → *Catalog*). On HOME you can also type `help name` (e.g. `help normcdf`) to jump straight to an entry.

Trig functions and `arg`, `polar`, `rect`, `atan2` follow the current angle mode (**DEG** or **RAD**, toggled with **F9**). Most functions also work element-wise on lists and matrices, e.g. `sqrt([4, 9])` → `[2, 3]`.

**Categories:** [Basic](#basic) · [Trigonometry](#trigonometry) · [Complex numbers](#complex-numbers) · [Number theory](#number-theory) · [Random](#random) · [Statistics & lists](#statistics--lists) · [Probability](#probability) · [Matrices](#matrices) · [Calculus & solving](#calculus--solving) · [Units](#units) · [Programs](#programs) · [Constants](#constants) · [Aliases](#aliases)

## Basic

| Function | What it does | Example |
|---|---|---|
| `sqrt(x)` | Square root. Negative numbers give imaginary results. | `sqrt(16) = 4` |
| `cbrt(x)` | Cube root (real for negative numbers). | `cbrt(-8) = -2` |
| `root(x, n)` | The n-th root of x. | `root(32, 5) = 2` |
| `sq(x)` | x squared (x²). | `sq(12) = 144` |
| `exp(x)` | e raised to the power x. | `exp(1) = 2.71828182846` |
| `ln(x)` | Natural logarithm (base e). | `ln(e^2) = 2` |
| `log(x) or log(x, base)` | Logarithm, base 10 unless you give a base. | `log(1000) = 3,  log(8, 2) = 3` |
| `log10(x)` | Base-10 logarithm. | `log10(0.01) = -2` |
| `log2(x)` | Base-2 logarithm. | `log2(1024) = 10` |
| `abs(x)` | Absolute value (distance from 0; modulus for complex numbers). | `abs(-7) = 7` |
| `sign(x)` | -1, 0 or 1 depending on the sign of x. | `sign(-3.2) = -1` |
| `floor(x)` | Round down to a whole number. | `floor(2.7) = 2` |
| `ceil(x)` | Round up to a whole number. | `ceil(2.1) = 3` |
| `round(x) or round(x, digits)` | Round to the nearest whole number or to some decimals. | `round(3.14159, 2) = 3.14` |
| `trunc(x)` | Drop the decimals (round toward zero). Also: ipart. | `trunc(-2.7) = -2` |
| `fpart(x)` | The decimal part of x. | `fpart(3.25) = 0.25` |
| `mod(a, b)` | Remainder of a divided by b (also a % b). | `mod(17, 5) = 2` |
| `inv(x)` | Reciprocal 1/x, or the inverse of a matrix. | `inv(4) = 0.25,  inv([[1,2],[3,4]])` |
| `neg(x)` | Change the sign of x. | `neg(5) = -5` |
| `hypot(a, b)` | Length of the hypotenuse: √(a² + b²). | `hypot(3, 4) = 5` |
| `min(a, b, …) or min(list)` | The smallest value. | `min(4, 9, 2) = 2` |
| `max(a, b, …) or max(list)` | The largest value. | `max([4, 9, 2]) = 9` |
| `percent(p, x)` | p percent of x. | `percent(15, 80) = 12` |

## Trigonometry

| Function | What it does | Example |
|---|---|---|
| `sin(x)` | Sine. Uses degrees or radians depending on the mode (DEG/RAD). | `sin(30) = 0.5 in DEG mode` |
| `cos(x)` | Cosine (angle in the current mode). | `cos(60) = 0.5 in DEG mode` |
| `tan(x)` | Tangent (angle in the current mode). | `tan(45) = 1 in DEG mode` |
| `sec(x)` | Secant, 1/cos(x). | `sec(60) = 2 in DEG mode` |
| `csc(x)` | Cosecant, 1/sin(x). | `csc(30) = 2 in DEG mode` |
| `cot(x)` | Cotangent, 1/tan(x). | `cot(45) = 1 in DEG mode` |
| `asin(x)` | Inverse sine (arcsin): the angle whose sine is x. | `asin(0.5) = 30 in DEG mode` |
| `acos(x)` | Inverse cosine (arccos). | `acos(0) = 90 in DEG mode` |
| `atan(x)` | Inverse tangent (arctan). | `atan(1) = 45 in DEG mode` |
| `atan2(y, x)` | Angle of the point (x, y) from the positive x axis. | `atan2(1, -1) = 135 in DEG mode` |
| `sinh(x)` | Hyperbolic sine. | `sinh(0) = 0` |
| `cosh(x)` | Hyperbolic cosine. | `cosh(0) = 1` |
| `tanh(x)` | Hyperbolic tangent. | `tanh(0) = 0` |
| `asinh(x)` | Inverse hyperbolic sine. | `asinh(0) = 0` |
| `acosh(x)` | Inverse hyperbolic cosine. | `acosh(1) = 0` |
| `atanh(x)` | Inverse hyperbolic tangent. | `atanh(0) = 0` |
| `todeg(x)` | Convert radians to degrees. | `todeg(pi) = 180` |
| `torad(x)` | Convert degrees to radians. | `torad(180) = 3.14159265359` |

## Complex numbers

| Function | What it does | Example |
|---|---|---|
| `re(z)` | Real part of a complex number. | `re(3+4i) = 3` |
| `im(z)` | Imaginary part of a complex number. | `im(3+4i) = 4` |
| `conj(z)` | Complex conjugate. | `conj(3+4i) = 3-4i` |
| `arg(z)` | Angle of a complex number (in the current angle mode). | `arg(1+i) = 45 in DEG mode` |
| `polar(z)` | Polar form [r, θ] of a complex number. | `polar(3+4i) = [5, 53.13…]` |
| `rect(r, θ)` | Complex number from polar form r·e^(iθ). | `rect(2, 90) = 2i in DEG mode` |

## Number theory

| Function | What it does | Example |
|---|---|---|
| `gcd(a, b, …)` | Greatest common divisor. | `gcd(12, 18) = 6` |
| `lcm(a, b, …)` | Least common multiple. | `lcm(4, 6) = 12` |
| `fact(n)` | Factorial n! = 1·2·3·…·n (also written n!). | `fact(5) = 120` |
| `nCr(n, r)` | Combinations: ways to choose r items from n (order doesn't matter). | `nCr(5, 2) = 10` |
| `nPr(n, r)` | Permutations: ordered ways to pick r items from n. | `nPr(5, 2) = 20` |
| `isprime(n)` | 1 if n is a prime number, otherwise 0. | `isprime(97) = 1` |
| `factor(n)` | Prime factors of a whole number. | `factor(360) = [2, 2, 2, 3, 3, 5]` |
| `tofrac(x)` | Show a decimal as a fraction (text). | `tofrac(0.375) = 3/8` |
| `hex(n)` | Write a whole number in hexadecimal (base 16). | `hex(255) = 0xFF` |
| `bin(n)` | Write a whole number in binary (base 2). | `bin(10) = 0b1010` |
| `oct(n)` | Write a whole number in octal (base 8). | `oct(64) = 0o100` |
| `band(a, b)` | Bitwise AND of two whole numbers. | `band(12, 10) = 8` |
| `bor(a, b)` | Bitwise OR. | `bor(12, 10) = 14` |
| `bxor(a, b)` | Bitwise exclusive OR. | `bxor(12, 10) = 6` |
| `shl(a, n)` | Shift bits left by n (multiply by 2ⁿ). | `shl(1, 4) = 16` |
| `shr(a, n)` | Shift bits right by n. | `shr(256, 4) = 16` |

## Random

| Function | What it does | Example |
|---|---|---|
| `rand() or rand(n)` | Random number between 0 and 1 (or a list of n of them). | `rand() = 0.4170…` |
| `randint(a, b) or randint(a, b, n)` | Random whole number from a to b (inclusive). | `randint(1, 6) = a dice roll` |
| `randn() or randn(μ, σ) or randn(μ, σ, n)` | Random number from a normal distribution. | `randn(100, 15)` |

## Statistics & lists

| Function | What it does | Example |
|---|---|---|
| `sum(list) or sum(expr, k, from, to)` | Add up a list, or a series Σ expr for k = from…to (to can be inf for quickly converging series — see [Solving and Calculus](Solving-and-Calculus#sums-and-products)). | `sum([1,2,3]) = 6,  sum(1/2^k, k, 0, inf) = 2` |
| `prod(list) or prod(expr, k, from, to)` | Multiply a list, or a product Π expr for k = from…to. | `prod([1,2,3,4]) = 24` |
| `mean(list)` | Average of the values. Also: avg. | `mean([2, 4, 9]) = 5` |
| `median(list)` | The middle value. | `median([3, 1, 7]) = 3` |
| `mode(list)` | The most common value(s). | `mode([1, 2, 2, 3]) = 2` |
| `sd(list)` | Sample standard deviation (divides by n−1). Also: stdev. | `sd([2,4,4,4,5,5,7,9]) = 2.138…` |
| `var(list)` | Sample variance (divides by n−1). | `var([1, 2, 3, 4]) = 1.666…` |
| `psd(list)` | Population standard deviation σ (divides by n). | `psd([2,4,4,4,5,5,7,9]) = 2` |
| `pvar(list)` | Population variance (divides by n). | `pvar([1, 2, 3, 4]) = 1.25` |
| `percentile(list, p)` | The value below which p percent of the data falls. | `percentile([1,2,3,4,5], 50) = 3` |
| `sort(list)` | Sort a list from smallest to largest. | `sort([3, 1, 2]) = [1, 2, 3]` |
| `rsort(list)` | Sort a list from largest to smallest. | `rsort([3, 1, 2]) = [3, 2, 1]` |
| `rev(list)` | Reverse the order of a list. | `rev([1, 2, 3]) = [3, 2, 1]` |
| `len(list)` | Number of items in a list (or entries in a matrix). Also: count. | `len([5, 6, 7]) = 3` |
| `cumsum(list)` | Running totals of a list. | `cumsum([1, 2, 3]) = [1, 3, 6]` |
| `range(from, to) or range(from, to, step)` | A list of evenly spaced numbers. | `range(1, 5) = [1, 2, 3, 4, 5]` |
| `seq(expr, k, from, to, step)` | Build a list by evaluating expr for each k. | `seq(k^2, k, 1, 5) = [1, 4, 9, 16, 25]` |
| `concat(list1, list2, …)` | Join lists together. | `concat([1, 2], [3]) = [1, 2, 3]` |
| `linreg(xs, ys)` | Best-fit line y = m·x + b. Returns [m, b]. | `linreg([1,2,3], [2,4,6.1]) = [2.05, -0.0667…]` |
| `corr(xs, ys)` | Correlation coefficient r (−1…1). | `corr([1,2,3], [2,4,7]) = 0.993…` |
| `polyfit(xs, ys, degree)` | Best-fit polynomial; returns coefficients, highest power first. | `polyfit([0,1,2], [1,2,5], 2) = [1, 0, 1]` |
| `polyval(coeffs, x)` | Evaluate a polynomial (coefficients highest power first). | `polyval([1, 0, -2], 3) = 7` |
| `expreg(xs, ys)` | Exponential fit y = a·e^(b·x). Returns [a, b]. | `expreg([0,1,2], [1,2.7,7.4])` |
| `powreg(xs, ys)` | Power fit y = a·x^b. Returns [a, b]. | `powreg([1,2,3], [2,8,18]) = [2, 2]` |
| `logreg(xs, ys)` | Logarithmic fit y = a + b·ln(x). Returns [a, b]. | `logreg([1,2,4], [0,1,2])` |

## Probability

| Function | What it does | Example |
|---|---|---|
| `normpdf(x) or normpdf(x, μ, σ)` | Normal distribution density (bell curve height). | `normpdf(0) = 0.3989…` |
| `normcdf(x) · normcdf(a, b) · normcdf(x, μ, σ) · normcdf(a, b, μ, σ)` | Normal probability: P(X ≤ x), or P(a ≤ X ≤ b) with two bounds. Default μ=0, σ=1. | `normcdf(-1.96, 1.96) = 0.95` |
| `invnorm(p) or invnorm(p, μ, σ)` | Inverse normal: the x with P(X ≤ x) = p. | `invnorm(0.975) = 1.95996…` |
| `binompdf(n, p, k)` | Probability of exactly k successes in n tries. | `binompdf(10, 0.5, 5) = 0.246…` |
| `binomcdf(n, p, k)` | Probability of at most k successes in n tries. | `binomcdf(10, 0.5, 5) = 0.623…` |
| `poisspdf(λ, k)` | Poisson probability of exactly k events. | `poisspdf(3, 2) = 0.224…` |
| `poisscdf(λ, k)` | Poisson probability of at most k events. | `poisscdf(3, 2) = 0.423…` |
| `tpdf(t, df)` | Student's t density. | `tpdf(0, 5) = 0.3796…` |
| `tcdf(t, df) or tcdf(a, b, df)` | Student's t probability P(T ≤ t), or between a and b. | `tcdf(2.571, 5) = 0.975` |
| `invt(p, df)` | Inverse t: the t with P(T ≤ t) = p. | `invt(0.975, 5) = 2.5706…` |
| `chi2pdf(x, k)` | Chi-square density. | `chi2pdf(2, 3) = 0.2075…` |
| `chi2cdf(x, k)` | Chi-square probability P(X ≤ x). | `chi2cdf(3.841, 1) = 0.95` |
| `fcdf(x, d1, d2)` | F distribution probability P(X ≤ x). | `fcdf(1, 5, 5) = 0.5` |
| `erf(x)` | Error function. | `erf(1) = 0.8427…` |
| `erfc(x)` | Complementary error function 1 − erf(x). | `erfc(1) = 0.1572…` |
| `gamma(x)` | Gamma function Γ(x); gamma(n) = (n−1)!. | `gamma(5) = 24` |
| `lgamma(x)` | Natural log of \|Γ(x)\|. | `lgamma(100) = 359.13…` |
| `beta(a, b)` | Beta function B(a, b). | `beta(2, 3) = 0.08333…` |

## Matrices

| Function | What it does | Example |
|---|---|---|
| `det(A)` | Determinant of a square matrix. | `det([[1,2],[3,4]]) = -2` |
| `transpose(A)` | Swap rows and columns (also A'). Also: trn. | `transpose([[1,2],[3,4]])` |
| `trace(A)` | Sum of the diagonal entries. | `trace([[1,2],[3,4]]) = 5` |
| `rank(A)` | Number of independent rows. | `rank([[1,2],[2,4]]) = 1` |
| `rref(A)` | Reduced row echelon form (Gauss–Jordan elimination). | `rref([[1,2,3],[4,5,6]])` |
| `eig(A)` | Eigenvalues of a square matrix (largest first). | `eig([[2,0],[0,3]]) = [3, 2]` |
| `eigvec(A)` | Eigenvectors as columns, in the same order as eig(A); each scaled so its largest entry is 1. | `eigvec([[2,1],[1,2]])` |
| `identity(n)` | The n×n identity matrix. Also: eye. | `identity(3)` |
| `zeros(rows, cols)` | A matrix full of zeros. | `zeros(2, 3)` |
| `ones(rows, cols)` | A matrix full of ones. | `ones(2, 2)` |
| `diag(list) or diag(A)` | Diagonal matrix from a list, or the diagonal of a matrix. | `diag([1, 2, 3])` |
| `norm(v)` | Length of a vector (Frobenius norm for matrices). | `norm([3, 4]) = 5` |
| `dot(u, v)` | Dot product of two vectors. | `dot([1,2,3], [4,5,6]) = 32` |
| `cross(u, v)` | Cross product of two 3D vectors. | `cross([1,0,0], [0,1,0]) = [0, 0, 1]` |
| `size(A)` | Dimensions [rows, columns]. | `size([[1,2,3],[4,5,6]]) = [2, 3]` |
| `rows(A)` | Number of rows. | `rows([[1,2],[3,4],[5,6]]) = 3` |
| `cols(A)` | Number of columns. | `cols([[1,2,3]]) = 3` |
| `augment(A, B)` | Put B to the right of A. | `augment([[1],[2]], [[3],[4]])` |
| `row(A, i)` | Row i of a matrix, as a list. | `row([[1,2],[3,4]], 2) = [3, 4]` |
| `col(A, j)` | Column j of a matrix, as a list. | `col([[1,2],[3,4]], 2) = [2, 4]` |

## Calculus & solving

| Function | What it does | Example |
|---|---|---|
| `solve(equation, x, guess) · solve(A, b)` | Find x that makes the equation true (numerically). With a matrix A and list b, solves A·x = b. | `solve(x^2 = 2, x) = 1.41421356237` |
| `deriv(expr, x, at)` | Derivative of expr with respect to x at a point (numerical). | `deriv(x^3, x, 2) = 12` |
| `integ(expr, x, a, b)` | Definite integral ∫ expr dx from a to b. Limits can be inf. | `integ(x^2, x, 0, 3) = 9` |
| `fmin(expr, x, a, b)` | Where expr is smallest on [a, b] (returns x). | `fmin((x-2)^2, x, 0, 5) = 2` |
| `fmax(expr, x, a, b)` | Where expr is largest on [a, b] (returns x). | `fmax(sin(x), x, 0, 180) = 90 in DEG mode` |
| `polyroots(coeffs)` | All roots of a polynomial; coefficients highest power first. | `polyroots([1, -3, 2]) = [1, 2]   (x² − 3x + 2)` |
| `piecewise(cond1, value1, cond2, value2, …, otherwise)` | Pick the value for the first true condition. Great for graphing. | `piecewise(x < 0, -x, x^2)` |

## Units

| Function | What it does | Example |
|---|---|---|
| `convert(value, "from", "to")` | Convert between units. You can also type: 5 km to mi | `convert(100, "C", "F") = 212` |

## Programs

| Function | What it does | Example |
|---|---|---|
| `input("prompt")` | Ask the user to type a value while a program runs. | `n = input("How many? ")` |
| `str(x)` | Turn a value into text (for print). | `print "x = " + str(5)` |

## Constants

Constants are read-only names. Physical constants start with an underscore so they never clash with your own variables (`_c` is the speed of light; `c` is free for you to use). Values are in SI units.

### Math

| Name | Meaning |
|---|---|
| `pi` | π = 3.14159…, ratio of a circle's circumference to its diameter |
| `e` | Euler's number e = 2.71828… |
| `i` | the imaginary unit, i² = −1 (you can also write 3+4i) |
| `phi` | golden ratio φ = 1.61803… |
| `inf` | infinity ∞ (e.g. as an integration limit) |
| `nan` | 'not a number' (undefined) |

### Physics

| Name | Meaning |
|---|---|
| `_c` | speed of light in vacuum, 299 792 458 m/s |
| `_h` | Planck constant, J·s |
| `_hbar` | reduced Planck constant ħ = h/2π, J·s |
| `_e` | elementary charge, C |
| `_kB` | Boltzmann constant, J/K |
| `_G` | gravitational constant, m³/(kg·s²) |
| `_g` | standard gravity, m/s² |
| `_me` | electron mass, kg |
| `_mp` | proton mass, kg |
| `_mn` | neutron mass, kg |
| `_eps0` | vacuum permittivity ε₀, F/m |
| `_mu0` | vacuum permeability μ₀, N/A² |
| `_sigma` | Stefan–Boltzmann constant, W/(m²·K⁴) |
| `_a0` | Bohr radius, m |
| `_Ry` | Rydberg constant, 1/m |
| `_alpha` | fine-structure constant |

### Chemistry

| Name | Meaning |
|---|---|
| `_NA` | Avogadro constant, 1/mol |
| `_R` | molar gas constant, J/(mol·K) |
| `_F` | Faraday constant, C/mol |
| `_u` | atomic mass unit (dalton), kg |
| `_atm` | standard atmosphere, Pa |
| `_Vm` | molar volume of an ideal gas at 0 °C and 1 atm, m³/mol |
## Aliases

These alternative names work everywhere a function name does:

| Alias | Same as |
|---|---|
| `ipart` | `trunc` |
| `arcsin`, `arccos`, `arctan` | `asin`, `acos`, `atan` |
| `factorial` | `fact` |
| `comb`, `choose` | `nCr` |
| `perm` | `nPr` |
| `avg`, `average` | `mean` |
| `stdev` | `sd` |
| `count`, `length` | `len` |
| `trn` | `transpose` |
| `eye` | `identity` |
| `nsolve` | `solve` |
| `derivative` | `deriv` |
| `integral`, `integrate` | `integ` |
| `roots`, `proots` | `polyroots` |

## See also

* [Expressions and Syntax](Expressions-and-Syntax) — operators, lists, matrices, complex numbers
* [Unit Conversion](Unit-Conversion) — every unit `convert` and `to` understand
* [Solving and Calculus](Solving-and-Calculus) — how `solve`, `integ`, `deriv` work and when they need a guess
