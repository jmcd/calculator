// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Calculator

// A keypad button. Clicking sends 'command' to the calculator engine unless it is
// negative or 'sendsToCalculator' is false (the converter handles its own clicks).
QQC2.Button {
    id: button

    enum Kind { Operator, Digit, Equals }

    property int command: -1
    property bool sendsToCalculator: true
    property int kind: CalcButton.Kind.Operator
    property string tip: ""

    focusPolicy: Qt.NoFocus
    highlighted: kind === CalcButton.Kind.Equals

    font.pointSize: Kirigami.Theme.defaultFont.pointSize * (kind === CalcButton.Kind.Digit ? 1.4 : 1.25)
    font.weight: kind === CalcButton.Kind.Digit ? Font.DemiBold : Font.Normal

    Layout.fillWidth: true
    Layout.fillHeight: true
    Layout.preferredWidth: Kirigami.Units.gridUnit * 3
    Layout.preferredHeight: Kirigami.Units.gridUnit * 2.2
    Layout.minimumWidth: Kirigami.Units.gridUnit * 1.8
    Layout.minimumHeight: Kirigami.Units.gridUnit * 1.6

    QQC2.ToolTip.text: tip
    QQC2.ToolTip.visible: tip.length > 0 && hovered
    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

    Accessible.name: tip.length > 0 ? tip : text

    onClicked: {
        if (sendsToCalculator && command >= 0) {
            CalculatorEngine.sendCommand(command)
        }
    }
}
