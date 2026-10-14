# Calculator for KDE / Linux

A Qt 6 + Kirigami frontend for the Windows Calculator's calculation engine
(`src/CalcManager`), so the app runs natively on KDE Plasma and other Linux desktops.

Supported: Standard, Scientific and Programmer modes (including the bit-toggling
keypad), history, memory, copy/paste, keyboard shortcuts and date calculation.
Not yet ported: unit/currency converters and graphing.

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
| `EngineResourceProvider` (C#)       | `EngineResourceProvider` (C++)       |
| XAML views                          | QML in `qml/`                        |

UI and engine strings are read from the same `.resw` files the Windows app uses
(`src/Calculator/Resources/<lang>/`), so existing translations apply.
