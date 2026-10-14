// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Calculator

// 64 toggleable bits, most significant first, grouped into nibbles.
ColumnLayout {
    spacing: Kirigami.Units.largeSpacing

    Repeater {
        model: 4 // rows of 16 bits

        RowLayout {
            id: bitRow
            required property int index
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Kirigami.Units.largeSpacing

            Repeater {
                model: 4 // nibbles per row

                ColumnLayout {
                    id: nibble
                    required property int index
                    // Bit index of this nibble's least significant bit.
                    readonly property int lowBit: 60 - (bitRow.index * 16) - (index * 4)
                    Layout.fillWidth: true
                    spacing: 0

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        Repeater {
                            model: 4
                            QQC2.ToolButton {
                                required property int index
                                readonly property int bit: nibble.lowBit + 3 - index
                                Layout.fillWidth: true
                                focusPolicy: Qt.NoFocus
                                enabled: bit < CalculatorEngine.wordSize
                                text: CalculatorEngine.bits[bit] ? "1" : "0"
                                font.family: "monospace"
                                font.weight: CalculatorEngine.bits[bit] ? Font.Bold : Font.Normal
                                font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.2
                                Accessible.name: Strings.get("BitPosition", "%1 bit").replace("%1", bit) + ", " + text
                                onClicked: CalculatorEngine.toggleBit(bit)
                            }
                        }
                    }
                    QQC2.Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignRight
                        text: nibble.lowBit
                        opacity: 0.6
                        font: Kirigami.Theme.smallFont
                    }
                }
            }
        }
    }
}
