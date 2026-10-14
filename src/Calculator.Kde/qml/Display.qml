// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Calculator

// Expression line and primary result, with a copy/paste context menu.
ColumnLayout {
    id: display
    spacing: 0

    QQC2.Label {
        Layout.fillWidth: true
        Layout.leftMargin: Kirigami.Units.largeSpacing
        Layout.rightMargin: Kirigami.Units.largeSpacing
        horizontalAlignment: Text.AlignRight
        elide: Text.ElideLeft
        opacity: 0.7
        font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.1
        // Keep the line's height even when the expression is empty.
        text: CalculatorEngine.expression.length > 0 ? CalculatorEngine.expression : " "
        Accessible.name: Strings.get("Format_CalculatorExpression", "Expression is %1").replace("%1", CalculatorEngine.expression)
    }

    QQC2.Label {
        id: value
        Layout.fillWidth: true
        Layout.preferredHeight: Kirigami.Units.gridUnit * 3.5
        Layout.leftMargin: Kirigami.Units.largeSpacing
        Layout.rightMargin: Kirigami.Units.largeSpacing
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: CalculatorEngine.isError ? Kirigami.Units.gridUnit * 1.6 : Kirigami.Units.gridUnit * 2.8
        font.weight: Font.DemiBold
        fontSizeMode: Text.HorizontalFit
        minimumPixelSize: Kirigami.Units.gridUnit * 0.8
        text: CalculatorEngine.displayValue
        Accessible.role: Accessible.StaticText
        Accessible.name: Strings.get("Format_CalculatorResults", "Display is %1").replace("%1", text)

        SequentialAnimation on opacity {
            id: flash
            running: false
            NumberAnimation { to: 0.4; duration: Kirigami.Units.shortDuration }
            NumberAnimation { to: 1.0; duration: Kirigami.Units.shortDuration }
        }

        Connections {
            target: CalculatorEngine
            function onMaxDigitsReached() { flash.restart() }
            function onCopied() { flash.restart() }
        }
    }

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: contextMenu.popup()
    }
    TapHandler {
        acceptedDevices: PointerDevice.TouchScreen
        onLongPressed: contextMenu.popup()
    }

    QQC2.Menu {
        id: contextMenu
        QQC2.MenuItem {
            text: Strings.get("copyMenuItem", "Copy")
            icon.name: "edit-copy"
            onTriggered: CalculatorEngine.copyToClipboard()
        }
        QQC2.MenuItem {
            text: Strings.get("pasteMenuItem", "Paste")
            icon.name: "edit-paste"
            onTriggered: CalculatorEngine.pasteFromClipboard()
        }
    }
}
