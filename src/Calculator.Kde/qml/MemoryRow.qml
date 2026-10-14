// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Calculator

// MC MR M+ M- MS M▾
RowLayout {
    id: row

    // Programmer mode only shows MS and M▾, like Windows.
    property bool compact: false
    // Whether to show the M▾ button (hidden when the memory list is docked).
    property bool showListButton: true
    signal memoryListRequested()

    readonly property bool hasMemory: CalculatorEngine.memory.length > 0

    spacing: 0

    component MemoryButton: QQC2.ToolButton {
        Layout.fillWidth: true
        focusPolicy: Qt.NoFocus
        font.pointSize: Kirigami.Theme.defaultFont.pointSize * 0.95
        QQC2.ToolTip.visible: hovered && QQC2.ToolTip.text.length > 0
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
    }

    MemoryButton {
        visible: !row.compact
        text: "MC"
        enabled: row.hasMemory
        QQC2.ToolTip.text: Strings.get("ClearMemoryButton.[using:Windows.UI.Xaml.Controls]ToolTipService.ToolTip", "Clear all memory (Ctrl+L)")
        onClicked: CalculatorEngine.memoryClearAll()
    }
    MemoryButton {
        visible: !row.compact
        text: "MR"
        enabled: row.hasMemory
        QQC2.ToolTip.text: "Memory recall (Ctrl+R)"
        onClicked: CalculatorEngine.memoryRecall()
    }
    MemoryButton {
        visible: !row.compact
        text: "M+"
        QQC2.ToolTip.text: "Memory add (Ctrl+P)"
        onClicked: CalculatorEngine.memoryAdd()
    }
    MemoryButton {
        visible: !row.compact
        text: "M−"
        QQC2.ToolTip.text: "Memory subtract (Ctrl+Q)"
        onClicked: CalculatorEngine.memorySubtract()
    }
    MemoryButton {
        text: "MS"
        QQC2.ToolTip.text: "Memory store (Ctrl+M)"
        onClicked: CalculatorEngine.memoryStore()
    }
    MemoryButton {
        visible: row.showListButton
        text: "M▾"
        enabled: row.hasMemory
        QQC2.ToolTip.text: Strings.get("MemoryButton_Open", "Open memory flyout")
        onClicked: row.memoryListRequested()
    }
}
