# Statistics and Probability

## Lists on HOME

A list is written in brackets: `data = [4, 8, 15, 16, 23, 42]`.

```text
mean(data)            → 18
median(data)          → 15.5
sd(data)              → sample standard deviation (n − 1)
psd(data)             → population standard deviation (n)
percentile(data, 90)
sort(data)   rsort(data)   rev(data)   cumsum(data)
len(data)   sum(data)   prod(data)   min(data)   max(data)
mode([1, 2, 2, 3])    → 2
```

Build lists with `range(1, 10)`, `range(0, 1, 0.1)`, `seq(k^2, k, 1, 10)` and `concat(a, b)`; generate random data with `rand(n)`, `randint(a, b, n)`, `randn(μ, σ, n)`. Lists support element-wise arithmetic: `data * 2`, `data - mean(data)`.

## Regression

All fits take a list of x values and a list of y values:

| Function | Model | Returns |
|---|---|---|
| `linreg(xs, ys)` | y = m·x + b | `[m, b]` |
| `polyfit(xs, ys, n)` | polynomial of degree n | coefficients, highest power first |
| `expreg(xs, ys)` | y = a·e^(b·x) | `[a, b]` |
| `powreg(xs, ys)` | y = a·x^b | `[a, b]` |
| `logreg(xs, ys)` | y = a + b·ln(x) | `[a, b]` |
| `corr(xs, ys)` | — | correlation coefficient r |

Evaluate a fitted polynomial with `polyval(coeffs, x)`.

## The STAT app

Open with **F4** on HOME (or `stat`). It shows four data lists, **L1 … L4**, in columns. They are ordinary variables, so `mean(L1)` works on HOME and anything you store in `L1` appears here.

Type a number into the selected cell and press **Enter** to move to the next row; **Del** removes a value. You can also type an expression that gives a list, like `range(1, 20)`, to fill a column at once. For fits, **x values go in L1 and y values in L2**.

| Key | Does |
|---|---|
| **F1 1-Var** | one-variable statistics of the selected column: n, mean x̄, Σx, Σx², Sx (sample SD), σx (population SD), min, Q1, median, Q3, max |
| **F2 Fit** | fit the selected model to L1 (x) and L2 (y); shows the equation, the coefficients, R² and (for linear fits) r |
| **F3 model** | cycle the model: Linear, Quadratic, Cubic, Exponential, Power, Logarithmic |
| **F4 Plot** | scatter plot of L1 vs L2 with the fitted curve |
| **F5 → y1** | store the fitted equation as `y1` so you can graph and trace it in GRAPH |
| **F6 Sort** | sort the selected column |
| **F7 Clear** | clear the selected column (asks first) |

The mean from **1-Var** and the coefficients from **Fit** are also stored in `ans`.

### Scatter plots

**F4 Plot** draws the points with the current model's curve and its equation and R² on top. In the plot, **F2 Model** cycles the model and **F3 → y1** sends the curve to GRAPH; **F1 Data** returns to the table.

## Probability distributions

| Distribution | Density / point probability | Cumulative | Inverse |
|---|---|---|---|
| Normal | `normpdf(x, μ, σ)` | `normcdf(x, μ, σ)` or `normcdf(a, b, μ, σ)` | `invnorm(p, μ, σ)` |
| Binomial | `binompdf(n, p, k)` | `binomcdf(n, p, k)` | — |
| Poisson | `poisspdf(λ, k)` | `poisscdf(λ, k)` | — |
| Student's t | `tpdf(t, df)` | `tcdf(t, df)` or `tcdf(a, b, df)` | `invt(p, df)` |
| Chi-square | `chi2pdf(x, k)` | `chi2cdf(x, k)` | — |
| F | — | `fcdf(x, d1, d2)` | — |

For the normal distribution, μ and σ default to 0 and 1:

```text
normcdf(-1.96, 1.96)    → 0.95        P(−1.96 ≤ Z ≤ 1.96)
invnorm(0.975)          → 1.95996…
binompdf(10, 0.5, 5)    → 0.246…
tcdf(2.571, 5)          → 0.975
```

Related special functions: `erf`, `erfc`, `gamma`, `lgamma`, `beta`.

## Combinatorics

`nCr(n, r)` (combinations), `nPr(n, r)` (permutations), `n!` or `fact(n)`. Aliases: `comb`, `choose`, `perm`, `factorial`.

## Example: a confidence interval

```text
data = [12.1, 11.8, 12.4, 12.0, 11.9, 12.3]
n = len(data)
m = mean(data)
h = invt(0.975, n - 1) * sd(data) / sqrt(n)
[m - h, m + h]
```
