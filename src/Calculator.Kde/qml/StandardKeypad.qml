// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Calculator

GridLayout {
    columns: 4
    rowSpacing: Kirigami.Units.smallSpacing
    columnSpacing: Kirigami.Units.smallSpacing
    uniformCellWidths: true
    uniformCellHeights: true

    CalcButton { text: "%"; command: CalculatorEngine.Percent; tip: "Percent (%)" }
    CalcButton { text: "CE"; command: CalculatorEngine.ClearEntry; tip: "Clear entry (Delete)" }
    CalcButton { text: "C"; command: CalculatorEngine.Clear; tip: "Clear (Esc)" }
    CalcButton { text: "⌫"; command: CalculatorEngine.Backspace; tip: "Backspace" }

    CalcButton { text: "1/x"; command: CalculatorEngine.Reciprocal; tip: "Reciprocal (R)" }
    CalcButton { text: "x²"; command: CalculatorEngine.Square; tip: "Square (Q)" }
    CalcButton { text: "²√x"; command: CalculatorEngine.SquareRoot; tip: "Square root (@)" }
    CalcButton { text: "÷"; command: CalculatorEngine.Divide; tip: "Divide (/)" }

    CalcButton { text: "7"; command: CalculatorEngine.Seven; kind: CalcButton.Kind.Digit }
    CalcButton { text: "8"; command: CalculatorEngine.Eight; kind: CalcButton.Kind.Digit }
    CalcButton { text: "9"; command: CalculatorEngine.Nine; kind: CalcButton.Kind.Digit }
    CalcButton { text: "×"; command: CalculatorEngine.Multiply; tip: "Multiply (*)" }

    CalcButton { text: "4"; command: CalculatorEngine.Four; kind: CalcButton.Kind.Digit }
    CalcButton { text: "5"; command: CalculatorEngine.Five; kind: CalcButton.Kind.Digit }
    CalcButton { text: "6"; command: CalculatorEngine.Six; kind: CalcButton.Kind.Digit }
    CalcButton { text: "−"; command: CalculatorEngine.Subtract; tip: "Subtract (-)" }

    CalcButton { text: "1"; command: CalculatorEngine.One; kind: CalcButton.Kind.Digit }
    CalcButton { text: "2"; command: CalculatorEngine.Two; kind: CalcButton.Kind.Digit }
    CalcButton { text: "3"; command: CalculatorEngine.Three; kind: CalcButton.Kind.Digit }
    CalcButton { text: "+"; command: CalculatorEngine.Add; tip: "Add (+)" }

    CalcButton { text: "+/−"; command: CalculatorEngine.Negate; tip: "Negate (F9)" }
    CalcButton { text: "0"; command: CalculatorEngine.Zero; kind: CalcButton.Kind.Digit }
    CalcButton { text: CalculatorEngine.decimalSeparator; command: CalculatorEngine.Decimal; kind: CalcButton.Kind.Digit }
    CalcButton { text: "="; command: CalculatorEngine.Equals; kind: CalcButton.Kind.Equals; tip: "Equals (Enter)" }
}
