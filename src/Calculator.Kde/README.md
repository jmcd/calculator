# Calculator for KDE / Linux

A Qt 6 + Kirigami frontend for the Windows Calculator's calculation engine
(`src/CalcManager`), so the app runs natively on KDE Plasma and other Linux desktops.

Supported: Standard, Scientific, Graphing and Programmer modes (including the
bit-toggling keypad), history, memory, copy/paste, keyboard shortcuts, date
calculation and the unit converter (all 13 categories, including currency).

### Graphing

The Windows app's graphing engine is closed source (its open-source build ships
only mocks), so this port has its own: `GraphExpression` parses and evaluates
equations, and `GraphView` draws them. It supports up to 14 `y = f(x)` equations,
parameters with sliders, pan/zoom, tracing, and the window and angle-unit options.

Input syntax: `y = 2x^2 - 3`, `f(x) = sin x / x`, or just `a x + b`. Implicit
multiplication (`2x`, `3(x+1)`, `2sin x`), `|x|`, `x!`, `π`, `e`, `√`, `²`, `³`, and
the functions `sin cos tan sec csc cot` (with `asin`/`arcsin`/`sin⁻¹` inverses),
`sinh cosh tanh` and inverses, `sqrt cbrt exp ln log log2 abs floor ceil round
sign`, plus `log(base, x)`, `root(x, n)`, `min`, `max` and `mod`. Any other single
letter becomes a parameter with a slider.

Not yet supported: function analysis, implicit equations (`x^2 + y^2 = 25`),
inequalities, and per-equation line styles.

### Currency rates

The Windows app's exchange-rate service isn't public (the open-source Windows build
ships made-up currencies). This port uses [Frankfurter](https://frankfurter.dev),
which publishes the European Central Bank's daily reference rates for about 30
currencies, with no API key. Rates are fetched when the Currency converter is opened
and they are more than a day old, or on **Update rates**, and cached in
`~/.cache/calculator-kde/currency-rates.json` so conversion works offline.

## Build

Fedora:

```sh
sudo dnf install cmake ninja-build gcc-c++ qt6-qtbase-devel qt6-qtdeclarative-devel \
    kf6-kirigami kf6-kirigami-addons kf6-qqc2-desktop-style
```

Then, from the repository root:

```sh
cmake -S . -B build-linux -G Ninja
cmake --build build-linux
ctest --test-dir build-linux          # engine + controller tests
./build-linux/src/Calculator.Kde/calculator-kde
```

Install system-wide (adds a desktop entry and the localized strings):

```sh
sudo cmake --install build-linux --prefix /usr/local
```

## How it fits together

| Windows app                         | KDE app                              |
|-------------------------------------|--------------------------------------|
| `CalcManager` (C++)                 | same sources, built with CMake       |
| `StandardCalculatorViewModel` (C#)  | `CalculatorController` (C++/QObject) |
| `DateCalculationEngine` (C#)        | `DateCalculator` (C++, `QDate`)      |
| `UnitConverterViewModel` (C#)       | `UnitConverterController` (C++)      |
| `GraphingCalculatorViewModel` (C#)  | `GraphingController` (C++)           |
| closed-source graphing engine       | `GraphExpression` + `GraphView` (C++)|
| `UnitConverterDataLoader` (C#)      | `UnitConverterDataLoader` (C++)      |
| `CurrencyDataLoader` (C#)           | `CurrencyDataLoader` (C++, Frankfurter) |
| `EngineResourceProvider` (C#)       | `EngineResourceProvider` (C++)       |
| XAML views                          | QML in `qml/`                        |

UI and engine strings are read from the same `.resw` files the Windows app uses
(`src/Calculator/Resources/<lang>/`), so existing translations apply.
