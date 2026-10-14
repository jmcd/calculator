// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Calculator

// HEX / DEC / OCT / BIN rows. Clicking a row switches the input radix.
ColumnLayout {
    spacing: 0

    Repeater {
        model: [
            { radix: 16, key: "Hex", label: "HEX", shortcut: "F5" },
            { radix: 10, key: "Dec", label: "DEC", shortcut: "F6" },
            { radix: 8, key: "Oct", label: "OCT", shortcut: "F7" },
            { radix: 2, key: "Bin", label: "BIN", shortcut: "F8" }
        ]

        delegate: QQC2.ItemDelegate {
            id: row
            required property var modelData
            readonly property bool selected: CalculatorEngine.radix === modelData.radix
            readonly property string value: {
                switch (modelData.radix) {
                case 16: return CalculatorEngine.hexValue
                case 8: return CalculatorEngine.octValue
                case 2: return CalculatorEngine.binValue
                default: return CalculatorEngine.decValue
                }
            }

            Layout.fillWidth: true
            focusPolicy: Qt.NoFocus
            highlighted: selected
            topPadding: Kirigami.Units.smallSpacing
            bottomPadding: Kirigami.Units.smallSpacing
            onClicked: CalculatorEngine.radix = modelData.radix

            QQC2.ToolTip.text: modelData.shortcut
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay * 2

            contentItem: RowLayout {
                spacing: Kirigami.Units.largeSpacing
                QQC2.Label {
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 2.5
                    text: Strings.get(row.modelData.key, row.modelData.label)
                    font.weight: row.selected ? Font.Bold : Font.Normal
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    text: row.value
                    wrapMode: Text.WrapAnywhere
                    maximumLineCount: 3
                    font.family: "monospace"
                    font.weight: row.selected ? Font.DemiBold : Font.Normal
                }
            }
        }
    }
}
