// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Calculator

ColumnLayout {
    id: keypad
    spacing: Kirigami.Units.smallSpacing

    property bool second: false
    property bool trigInverse: false
    property bool trigHyperbolic: false

    // Angle unit and scientific-notation toggle.
    RowLayout {
        Layout.fillWidth: true
        spacing: 0

        QQC2.ToolButton {
            focusPolicy: Qt.NoFocus
            text: ["DEG", "RAD", "GRAD"][CalculatorEngine.angleMode]
            QQC2.ToolTip.text: "Degrees (F3) · Radians (F4) · Gradians (F5)"
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            onClicked: CalculatorEngine.angleMode = (CalculatorEngine.angleMode + 1) % 3
        }
        QQC2.ToolButton {
            focusPolicy: Qt.NoFocus
            text: "F-E"
            checkable: true
            checked: CalculatorEngine.exponentialFormat
            onToggled: CalculatorEngine.exponentialFormat = checked
            QQC2.ToolTip.text: Strings.get("ftoeButton.[using:Windows.UI.Xaml.Automation]AutomationProperties.Name", "Scientific notation") + " (V)"
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }
        Item { Layout.fillWidth: true }
    }

    // Trigonometry and function flyouts.
    RowLayout {
        Layout.fillWidth: true
        spacing: 0

        QQC2.ToolButton {
            id: trigButton
            focusPolicy: Qt.NoFocus
            icon.name: "draw-triangle"
            text: Strings.get("trigButton.Text", "Trigonometry")
            down: trigPopup.visible
            onClicked: trigPopup.open()

            QQC2.Popup {
                id: trigPopup
                y: trigButton.height
                padding: Kirigami.Units.smallSpacing

                component TrigButton: CalcButton {
                    property int normal
                    property int inverse
                    property int hyperbolic
                    property int inverseHyperbolic
                    property string name
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 4
                    text: name + (keypad.trigHyperbolic ? "h" : "") + (keypad.trigInverse ? "⁻¹" : "")
                    command: keypad.trigHyperbolic ? (keypad.trigInverse ? inverseHyperbolic : hyperbolic)
                                                   : (keypad.trigInverse ? inverse : normal)
                    onClicked: trigPopup.close()
                }

                contentItem: GridLayout {
                    columns: 4
                    uniformCellWidths: true
                    rowSpacing: Kirigami.Units.smallSpacing
                    columnSpacing: Kirigami.Units.smallSpacing

                    CalcButton {
                        text: "2ⁿᵈ"
                        checkable: true
                        checked: keypad.trigInverse
                        onToggled: keypad.trigInverse = checked
                        tip: Strings.get("trigShiftButton.[using:Windows.UI.Xaml.Automation]AutomationProperties.Name", "Inverse function")
                    }
                    TrigButton { name: "sin"; normal: CalculatorEngine.Sin; inverse: CalculatorEngine.Asin; hyperbolic: CalculatorEngine.Sinh; inverseHyperbolic: CalculatorEngine.Asinh }
                    TrigButton { name: "cos"; normal: CalculatorEngine.Cos; inverse: CalculatorEngine.Acos; hyperbolic: CalculatorEngine.Cosh; inverseHyperbolic: CalculatorEngine.Acosh }
                    TrigButton { name: "tan"; normal: CalculatorEngine.Tan; inverse: CalculatorEngine.Atan; hyperbolic: CalculatorEngine.Tanh; inverseHyperbolic: CalculatorEngine.Atanh }

                    CalcButton {
                        text: "hyp"
                        checkable: true
                        checked: keypad.trigHyperbolic
                        onToggled: keypad.trigHyperbolic = checked
                        tip: Strings.get("hypButton.[using:Windows.UI.Xaml.Automation]AutomationProperties.Name", "Hyperbolic function")
                    }
                    TrigButton { name: "sec"; normal: CalculatorEngine.Sec; inverse: CalculatorEngine.Asec; hyperbolic: CalculatorEngine.Sech; inverseHyperbolic: CalculatorEngine.Asech }
                    TrigButton { name: "csc"; normal: CalculatorEngine.Csc; inverse: CalculatorEngine.Acsc; hyperbolic: CalculatorEngine.Csch; inverseHyperbolic: CalculatorEngine.Acsch }
                    TrigButton { name: "cot"; normal: CalculatorEngine.Cot; inverse: CalculatorEngine.Acot; hyperbolic: CalculatorEngine.Coth; inverseHyperbolic: CalculatorEngine.Acoth }
                }
            }
        }

        QQC2.ToolButton {
            id: funcButton
            focusPolicy: Qt.NoFocus
            icon.name: "labplot-xy-equation-curve"
            text: "Function"
            down: funcPopup.visible
            onClicked: funcPopup.open()

            QQC2.Popup {
                id: funcPopup
                y: funcButton.height
                padding: Kirigami.Units.smallSpacing

                component FuncButton: CalcButton {
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 4
                    onClicked: funcPopup.close()
                }

                contentItem: GridLayout {
                    columns: 3
                    uniformCellWidths: true
                    rowSpacing: Kirigami.Units.smallSpacing
                    columnSpacing: Kirigami.Units.smallSpacing

                    FuncButton { text: "|x|"; command: CalculatorEngine.Abs; tip: Strings.get("AbsoluteValue", "Absolute value") }
                    FuncButton { text: "⌊x⌋"; command: CalculatorEngine.Floor; tip: "Floor" }
                    FuncButton { text: "⌈x⌉"; command: CalculatorEngine.Ceil; tip: "Ceiling" }
                    FuncButton { text: "rand"; command: CalculatorEngine.Rand; tip: "Random" }
                    FuncButton { text: "→dms"; command: CalculatorEngine.Dms; tip: Strings.get("DegreeMinuteSecond", "Degree minute second") }
                    FuncButton { text: "→deg"; command: CalculatorEngine.ToDegrees; tip: "Degrees" }
                }
            }
        }
        Item { Layout.fillWidth: true }
    }

    GridLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        columns: 5
        rowSpacing: Kirigami.Units.smallSpacing
        columnSpacing: Kirigami.Units.smallSpacing
        uniformCellWidths: true
        uniformCellHeights: true

        CalcButton {
            text: "2ⁿᵈ"
            checkable: true
            checked: keypad.second
            onToggled: keypad.second = checked
            tip: Strings.get("shiftButton.[using:Windows.UI.Xaml.Automation]AutomationProperties.Name", "Inverse function")
        }
        CalcButton { text: "π"; command: CalculatorEngine.Pi; tip: "Pi (P)" }
        CalcButton { text: "e"; command: CalculatorEngine.Euler; tip: "Euler's number (E)" }
        CalcButton { text: "C"; command: CalculatorEngine.Clear; tip: "Clear (Esc)" }
        CalcButton { text: "⌫"; command: CalculatorEngine.Backspace; tip: "Backspace" }

        CalcButton {
            text: keypad.second ? "x³" : "x²"
            command: keypad.second ? CalculatorEngine.Cube : CalculatorEngine.Square
            tip: keypad.second ? "Cube (#)" : "Square (Q)"
        }
        CalcButton { text: "1/x"; command: CalculatorEngine.Reciprocal; tip: "Reciprocal (R)" }
        CalcButton { text: "|x|"; command: CalculatorEngine.Abs; tip: Strings.get("AbsoluteValue", "Absolute value") }
        CalcButton { text: "exp"; command: CalculatorEngine.Exp; tip: "Exponential notation (X)" }
        CalcButton { text: "mod"; command: CalculatorEngine.Mod; tip: "Modulo" }

        CalcButton {
            text: keypad.second ? "∛x" : "²√x"
            command: keypad.second ? CalculatorEngine.CubeRoot : CalculatorEngine.SquareRoot
            tip: keypad.second ? Strings.get("CubeRoot", "Cube root") : "Square root (@)"
        }
        CalcButton { text: "("; command: CalculatorEngine.OpenParen; tip: "Open parenthesis" + (CalculatorEngine.parenthesisCount > 0 ? " (" + CalculatorEngine.parenthesisCount + ")" : "") }
        CalcButton { text: ")"; command: CalculatorEngine.CloseParen; tip: "Close parenthesis" }
        CalcButton { text: "n!"; command: CalculatorEngine.Factorial; tip: Strings.get("Factorial", "Factorial") + " (!)" }
        CalcButton { text: "÷"; command: CalculatorEngine.Divide; tip: "Divide (/)" }

        CalcButton {
            text: keypad.second ? "ʸ√x" : "xʸ"
            command: keypad.second ? CalculatorEngine.Root : CalculatorEngine.Power
            tip: keypad.second ? Strings.get("YRoot", "y root") + " (Ctrl+Y)" : "Power (^)"
        }
        CalcButton { text: "7"; command: CalculatorEngine.Seven; kind: CalcButton.Kind.Digit }
        CalcButton { text: "8"; command: CalculatorEngine.Eight; kind: CalcButton.Kind.Digit }
        CalcButton { text: "9"; command: CalculatorEngine.Nine; kind: CalcButton.Kind.Digit }
        CalcButton { text: "×"; command: CalculatorEngine.Multiply; tip: "Multiply (*)" }

        CalcButton {
            text: keypad.second ? "2ˣ" : "10ˣ"
            command: keypad.second ? CalculatorEngine.PowerOf2 : CalculatorEngine.PowerOf10
            tip: keypad.second ? "Two to the exponent" : "Ten to the exponent (Ctrl+G)"
        }
        CalcButton { text: "4"; command: CalculatorEngine.Four; kind: CalcButton.Kind.Digit }
        CalcButton { text: "5"; command: CalculatorEngine.Five; kind: CalcButton.Kind.Digit }
        CalcButton { text: "6"; command: CalculatorEngine.Six; kind: CalcButton.Kind.Digit }
        CalcButton { text: "−"; command: CalculatorEngine.Subtract; tip: "Subtract (-)" }

        CalcButton {
            text: keypad.second ? "logᵧx" : "log"
            command: keypad.second ? CalculatorEngine.LogBaseY : CalculatorEngine.Log
            tip: keypad.second ? Strings.get("Logy", "Log base") + " (Shift+L)" : "Log (L)"
        }
        CalcButton { text: "1"; command: CalculatorEngine.One; kind: CalcButton.Kind.Digit }
        CalcButton { text: "2"; command: CalculatorEngine.Two; kind: CalcButton.Kind.Digit }
        CalcButton { text: "3"; command: CalculatorEngine.Three; kind: CalcButton.Kind.Digit }
        CalcButton { text: "+"; command: CalculatorEngine.Add; tip: "Add (+)" }

        CalcButton {
            text: keypad.second ? "eˣ" : "ln"
            command: keypad.second ? CalculatorEngine.PowerOfE : CalculatorEngine.Ln
            tip: keypad.second ? "e to the exponent (Ctrl+N)" : Strings.get("NaturalLog", "Natural log") + " (N)"
        }
        CalcButton { text: "+/−"; command: CalculatorEngine.Negate; tip: "Negate (F9)" }
        CalcButton { text: "0"; command: CalculatorEngine.Zero; kind: CalcButton.Kind.Digit }
        CalcButton { text: CalculatorEngine.decimalSeparator; command: CalculatorEngine.Decimal; kind: CalcButton.Kind.Digit }
        CalcButton { text: "="; command: CalculatorEngine.Equals; kind: CalcButton.Kind.Equals; tip: "Equals (Enter)" }
    }
}
