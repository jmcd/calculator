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

    property bool showMemoryListButton: true
    signal memoryListRequested()

    property bool bitFlip: false
    // 0: arithmetic, 1: logical, 2: rotate circular, 3: rotate through carry
    property int shiftMode: 0

    readonly property var shiftCommands: [
        [CalculatorEngine.Lsh, CalculatorEngine.Rsh],
        [CalculatorEngine.Lsh, CalculatorEngine.RshLogical],
        [CalculatorEngine.Rol, CalculatorEngine.Ror],
        [CalculatorEngine.RolCarry, CalculatorEngine.RorCarry]
    ]
    readonly property var shiftLabels: [["<<", ">>"], ["<<", ">>"], ["RoL", "RoR"], ["RcL", "RcR"]]

    function digitEnabled(value) {
        return value < CalculatorEngine.radix
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 0

        QQC2.ToolButton {
            focusPolicy: Qt.NoFocus
            icon.name: "input-keyboard"
            checkable: true
            checked: !keypad.bitFlip
            onClicked: keypad.bitFlip = false
            display: QQC2.AbstractButton.IconOnly
            text: Strings.get("fullKeypad.[using:Windows.UI.Xaml.Controls]ToolTipService.ToolTip", "Full keypad")
            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }
        QQC2.ToolButton {
            focusPolicy: Qt.NoFocus
            icon.name: "view-grid"
            checkable: true
            checked: keypad.bitFlip
            onClicked: keypad.bitFlip = true
            display: QQC2.AbstractButton.IconOnly
            text: Strings.get("bitFlip.[using:Windows.UI.Xaml.Controls]ToolTipService.ToolTip", "Bit toggling keypad")
            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }
        QQC2.ToolButton {
            Layout.fillWidth: true
            focusPolicy: Qt.NoFocus
            text: ({ 64: "QWORD", 32: "DWORD", 16: "WORD", 8: "BYTE" })[CalculatorEngine.wordSize]
            QQC2.ToolTip.text: "QWORD (F12) · DWORD (F2) · WORD (F3) · BYTE (F4)"
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            onClicked: {
                const next = { 64: 32, 32: 16, 16: 8, 8: 64 }
                CalculatorEngine.wordSize = next[CalculatorEngine.wordSize]
            }
        }
        MemoryRow {
            compact: true
            showListButton: keypad.showMemoryListButton
            onMemoryListRequested: keypad.memoryListRequested()
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 0

        QQC2.ToolButton {
            id: bitwiseButton
            focusPolicy: Qt.NoFocus
            icon.name: "code-context"
            text: Strings.get("bitwiseButton.Text", "Bitwise")
            down: bitwisePopup.visible
            onClicked: bitwisePopup.open()

            QQC2.Popup {
                id: bitwisePopup
                y: bitwiseButton.height
                padding: Kirigami.Units.smallSpacing

                component BitwiseButton: CalcButton {
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 4
                    onClicked: bitwisePopup.close()
                }

                contentItem: GridLayout {
                    columns: 3
                    uniformCellWidths: true
                    rowSpacing: Kirigami.Units.smallSpacing
                    columnSpacing: Kirigami.Units.smallSpacing
                    BitwiseButton { text: "AND"; command: CalculatorEngine.And; tip: "AND (&)" }
                    BitwiseButton { text: "OR"; command: CalculatorEngine.Or; tip: "OR (|)" }
                    BitwiseButton { text: "NOT"; command: CalculatorEngine.Not; tip: "NOT (~)" }
                    BitwiseButton { text: "NAND"; command: CalculatorEngine.Nand }
                    BitwiseButton { text: "NOR"; command: CalculatorEngine.Nor }
                    BitwiseButton { text: "XOR"; command: CalculatorEngine.Xor; tip: "XOR (^)" }
                }
            }
        }

        QQC2.ToolButton {
            id: shiftButton
            focusPolicy: Qt.NoFocus
            icon.name: "format-indent-more"
            text: Strings.get("bitShiftButton.Text", "Bit shift")
            down: shiftMenu.visible
            onClicked: shiftMenu.popup(shiftButton, 0, shiftButton.height)

            QQC2.Menu {
                id: shiftMenu
                QQC2.ButtonGroup { id: shiftGroup }
                Repeater {
                    model: [
                        Strings.get("arithmeticShiftButton.Content", "Arithmetic shift"),
                        Strings.get("logicalShiftButton.Content", "Logical shift"),
                        Strings.get("rotateCircularButton.Content", "Rotate circular shift"),
                        Strings.get("rotateCarryShiftButton.Content", "Rotate through carry circular shift")
                    ]
                    QQC2.MenuItem {
                        required property int index
                        required property string modelData
                        text: modelData
                        checkable: true
                        checked: keypad.shiftMode === index
                        QQC2.ButtonGroup.group: shiftGroup
                        onTriggered: keypad.shiftMode = index
                    }
                }
            }
        }
        Item { Layout.fillWidth: true }
    }

    BitFlipPanel {
        visible: keypad.bitFlip
        Layout.fillWidth: true
        Layout.fillHeight: true
    }

    GridLayout {
        visible: !keypad.bitFlip
        Layout.fillWidth: true
        Layout.fillHeight: true
        columns: 5
        rowSpacing: Kirigami.Units.smallSpacing
        columnSpacing: Kirigami.Units.smallSpacing
        uniformCellWidths: true
        uniformCellHeights: true

        CalcButton { text: "A"; command: CalculatorEngine.HexA; enabled: CalculatorEngine.radix === 16; kind: CalcButton.Kind.Digit }
        CalcButton {
            text: keypad.shiftLabels[keypad.shiftMode][0]
            command: keypad.shiftCommands[keypad.shiftMode][0]
            tip: "Left shift (<)"
        }
        CalcButton {
            text: keypad.shiftLabels[keypad.shiftMode][1]
            command: keypad.shiftCommands[keypad.shiftMode][1]
            tip: "Right shift (>)"
        }
        CalcButton { text: "C"; command: CalculatorEngine.Clear; tip: "Clear (Esc)" }
        CalcButton { text: "⌫"; command: CalculatorEngine.Backspace; tip: "Backspace" }

        CalcButton { text: "B"; command: CalculatorEngine.HexB; enabled: CalculatorEngine.radix === 16; kind: CalcButton.Kind.Digit }
        CalcButton { text: "("; command: CalculatorEngine.OpenParen; tip: "Open parenthesis" }
        CalcButton { text: ")"; command: CalculatorEngine.CloseParen; tip: "Close parenthesis" }
        CalcButton { text: "%"; command: CalculatorEngine.Mod; tip: "Modulo (%)" }
        CalcButton { text: "÷"; command: CalculatorEngine.Divide; tip: "Divide (/)" }

        CalcButton { text: "C"; command: CalculatorEngine.HexC; enabled: CalculatorEngine.radix === 16; kind: CalcButton.Kind.Digit }
        CalcButton { text: "7"; command: CalculatorEngine.Seven; enabled: keypad.digitEnabled(7); kind: CalcButton.Kind.Digit }
        CalcButton { text: "8"; command: CalculatorEngine.Eight; enabled: keypad.digitEnabled(8); kind: CalcButton.Kind.Digit }
        CalcButton { text: "9"; command: CalculatorEngine.Nine; enabled: keypad.digitEnabled(9); kind: CalcButton.Kind.Digit }
        CalcButton { text: "×"; command: CalculatorEngine.Multiply; tip: "Multiply (*)" }

        CalcButton { text: "D"; command: CalculatorEngine.HexD; enabled: CalculatorEngine.radix === 16; kind: CalcButton.Kind.Digit }
        CalcButton { text: "4"; command: CalculatorEngine.Four; enabled: keypad.digitEnabled(4); kind: CalcButton.Kind.Digit }
        CalcButton { text: "5"; command: CalculatorEngine.Five; enabled: keypad.digitEnabled(5); kind: CalcButton.Kind.Digit }
        CalcButton { text: "6"; command: CalculatorEngine.Six; enabled: keypad.digitEnabled(6); kind: CalcButton.Kind.Digit }
        CalcButton { text: "−"; command: CalculatorEngine.Subtract; tip: "Subtract (-)" }

        CalcButton { text: "E"; command: CalculatorEngine.HexE; enabled: CalculatorEngine.radix === 16; kind: CalcButton.Kind.Digit }
        CalcButton { text: "1"; command: CalculatorEngine.One; kind: CalcButton.Kind.Digit }
        CalcButton { text: "2"; command: CalculatorEngine.Two; enabled: keypad.digitEnabled(2); kind: CalcButton.Kind.Digit }
        CalcButton { text: "3"; command: CalculatorEngine.Three; enabled: keypad.digitEnabled(3); kind: CalcButton.Kind.Digit }
        CalcButton { text: "+"; command: CalculatorEngine.Add; tip: "Add (+)" }

        CalcButton { text: "F"; command: CalculatorEngine.HexF; enabled: CalculatorEngine.radix === 16; kind: CalcButton.Kind.Digit }
        CalcButton { text: "+/−"; command: CalculatorEngine.Negate; tip: "Negate (F9)" }
        CalcButton { text: "0"; command: CalculatorEngine.Zero; kind: CalcButton.Kind.Digit }
        CalcButton { text: CalculatorEngine.decimalSeparator; enabled: false; kind: CalcButton.Kind.Digit }
        CalcButton { text: "="; command: CalculatorEngine.Equals; kind: CalcButton.Kind.Equals; tip: "Equals (Enter)" }
    }
}
