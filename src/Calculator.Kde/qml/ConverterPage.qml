// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Calculator

Kirigami.Page {
    id: page

    title: Converter.categories[Converter.categoryIndex] ?? Strings.get("ConverterModeText", "Converter")
    padding: Kirigami.Units.smallSpacing
    focus: true

    actions: [
        Kirigami.Action {
            icon.name: "view-refresh"
            text: Strings.get("RefreshButtonText.Content", "Update rates")
            visible: Converter.isCurrency
            enabled: !Converter.currencyLoading
            onTriggered: Converter.refreshCurrencyRates()
        },
        Kirigami.Action {
            icon.name: "edit-copy"
            text: Strings.get("copyMenuItem", "Copy")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: Converter.copyToClipboard()
        },
        Kirigami.Action {
            icon.name: "edit-paste"
            text: Strings.get("pasteMenuItem", "Paste")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: Converter.pasteFromClipboard()
        }
    ]

    Keys.onPressed: event => {
        if (Converter.handleKey(event.key, event.modifiers, event.text)) {
            event.accepted = true
        }
    }
    Component.onCompleted: forceActiveFocus()

    Connections {
        target: Converter
        function onPasteFailed() {
            applicationWindow().showPassiveNotification(Strings.get("CannotPaste", "This expression cannot be pasted"))
        }
        function onCurrencyRefreshFailed() {
            applicationWindow().showPassiveNotification(Strings.get("FailedToRefresh", "Couldn't get new rates. Try again later."))
        }
    }

    // One value with its unit picker. Clicking the value makes it the input.
    component ValueField: ColumnLayout {
        id: field
        required property int fieldNumber
        readonly property bool active: (fieldNumber === 1) === Converter.value1Active
        readonly property string value: fieldNumber === 1 ? Converter.value1 : Converter.value2
        readonly property string symbol: fieldNumber === 1 ? Converter.currencySymbol1 : Converter.currencySymbol2
        spacing: 0

        QQC2.AbstractButton {
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 3.2
            focusPolicy: Qt.NoFocus
            onClicked: Converter.activate(field.fieldNumber)
            Accessible.name: field.value + " " + unitBox.currentText

            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing
                QQC2.Label {
                    visible: Converter.isCurrency && field.symbol.length > 0
                    text: field.symbol
                    font.pixelSize: valueLabel.font.pixelSize * 0.6
                    opacity: 0.7
                }
                QQC2.Label {
                    id: valueLabel
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    verticalAlignment: Text.AlignVCenter
                    text: field.value
                    font.pixelSize: Kirigami.Units.gridUnit * (field.active ? 2.4 : 2.0)
                    font.weight: field.active ? Font.DemiBold : Font.Normal
                    fontSizeMode: Text.HorizontalFit
                    minimumPixelSize: Kirigami.Units.gridUnit * 0.8
                    elide: Text.ElideRight
                }
            }

            background: Rectangle {
                color: "transparent"
                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.topMargin: Kirigami.Units.smallSpacing
                    anchors.bottomMargin: Kirigami.Units.smallSpacing
                    width: 3
                    radius: 1.5
                    color: Kirigami.Theme.highlightColor
                    visible: field.active
                }
            }
            leftPadding: Kirigami.Units.largeSpacing
        }

        QQC2.ComboBox {
            id: unitBox
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 20
            focusPolicy: Qt.NoFocus
            enabled: Converter.units.length > 0
            model: Converter.isCurrency
                   ? Converter.units.map((name, i) => name + " (" + Converter.unitAbbreviations[i] + ")")
                   : Converter.units
            currentIndex: field.fieldNumber === 1 ? Converter.unit1Index : Converter.unit2Index
            onActivated: index => {
                if (field.fieldNumber === 1) {
                    Converter.unit1Index = index
                } else {
                    Converter.unit2Index = index
                }
                page.forceActiveFocus()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Kirigami.Units.smallSpacing

        QQC2.ComboBox {
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 20
            focusPolicy: Qt.NoFocus
            model: Converter.categories
            currentIndex: Converter.categoryIndex
            onActivated: index => {
                Converter.categoryIndex = index
                page.forceActiveFocus()
            }
        }

        // Shown while currency rates are unavailable.
        Kirigami.PlaceholderMessage {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: Converter.isCurrency && Converter.units.length === 0
            icon.name: Converter.currencyLoading ? "view-refresh" : "network-disconnect"
            text: Converter.currencyLoading
                  ? Strings.get("UpdatingCurrencyRates", "Updating currency rates")
                  : Strings.get("CurrencyRatesUpdateFailed", "Could not update rates")
            explanation: Converter.currencyLoading ? "" : "Currency conversion needs an internet connection the first time, to download exchange rates."
            helpfulAction: Kirigami.Action {
                enabled: !Converter.currencyLoading
                icon.name: "view-refresh"
                text: Strings.get("RefreshButtonText.Content", "Update rates")
                onTriggered: Converter.refreshCurrencyRates()
            }
        }

        ColumnLayout {
            visible: !(Converter.isCurrency && Converter.units.length === 0)
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            ValueField {
                Layout.fillWidth: true
                fieldNumber: 1
            }
            ValueField {
                Layout.fillWidth: true
                Layout.topMargin: Kirigami.Units.largeSpacing
                fieldNumber: 2
            }

            // "About equal to" suggestions (not shown for currency).
            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: Kirigami.Units.smallSpacing
                visible: !Converter.isCurrency && Converter.supplementaryResults.length > 0
                spacing: 0

                QQC2.Label {
                    text: Strings.get("SupplementaryResultsHeader.Text", "About equal to")
                    opacity: 0.7
                }
                Flow {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.largeSpacing
                    Repeater {
                        model: Converter.supplementaryResults
                        QQC2.Label {
                            required property var modelData
                            text: modelData.value + " " + modelData.unit
                            QQC2.ToolTip.text: modelData.unitName
                            QQC2.ToolTip.visible: hoverHandler.hovered
                            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                            HoverHandler { id: hoverHandler }
                        }
                    }
                }
            }

            // Exchange rate and when it was fetched.
            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: Kirigami.Units.smallSpacing
                visible: Converter.isCurrency
                spacing: 0

                QQC2.Label {
                    Layout.fillWidth: true
                    text: Converter.currencyRatio
                    wrapMode: Text.Wrap
                }
                RowLayout {
                    Layout.fillWidth: true
                    QQC2.Label {
                        Layout.fillWidth: true
                        text: Converter.currencyLoading
                              ? Strings.get("UpdatingCurrencyRates", "Updating currency rates")
                              : Converter.currencyTimestamp
                        color: Converter.currencyRatesOld ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.textColor
                        opacity: 0.7
                        wrapMode: Text.Wrap
                    }
                    QQC2.BusyIndicator {
                        visible: Converter.currencyLoading
                        running: visible
                        Layout.preferredHeight: Kirigami.Units.iconSizes.small
                        Layout.preferredWidth: Kirigami.Units.iconSizes.small
                    }
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    text: "Rates: European Central Bank, via frankfurter.dev"
                    font: Kirigami.Theme.smallFont
                    opacity: 0.6
                    wrapMode: Text.Wrap
                }
            }
        }

        Item {
            visible: !(Converter.isCurrency && Converter.units.length === 0)
            Layout.fillHeight: true
            Layout.minimumHeight: Kirigami.Units.smallSpacing
        }

        GridLayout {
            visible: !(Converter.isCurrency && Converter.units.length === 0)
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 12
            Layout.fillHeight: true
            columns: 3
            rowSpacing: Kirigami.Units.smallSpacing
            columnSpacing: Kirigami.Units.smallSpacing
            uniformCellWidths: true
            uniformCellHeights: true

            component ConverterButton: CalcButton {
                sendsToCalculator: false
                onClicked: if (command >= 0) Converter.sendCommand(command)
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
            ConverterButton { text: "CE"; command: Converter.Clear; tip: "Clear entry (Esc)" }
            ConverterButton { text: "⌫"; command: Converter.Backspace; tip: "Backspace" }

            ConverterButton { text: "7"; command: Converter.Seven; kind: CalcButton.Kind.Digit }
            ConverterButton { text: "8"; command: Converter.Eight; kind: CalcButton.Kind.Digit }
            ConverterButton { text: "9"; command: Converter.Nine; kind: CalcButton.Kind.Digit }

            ConverterButton { text: "4"; command: Converter.Four; kind: CalcButton.Kind.Digit }
            ConverterButton { text: "5"; command: Converter.Five; kind: CalcButton.Kind.Digit }
            ConverterButton { text: "6"; command: Converter.Six; kind: CalcButton.Kind.Digit }

            ConverterButton { text: "1"; command: Converter.One; kind: CalcButton.Kind.Digit }
            ConverterButton { text: "2"; command: Converter.Two; kind: CalcButton.Kind.Digit }
            ConverterButton { text: "3"; command: Converter.Three; kind: CalcButton.Kind.Digit }

            ConverterButton {
                text: "+/−"
                command: Converter.Negate
                enabled: Converter.supportsNegative
                tip: "Negate (F9)"
            }
            ConverterButton { text: "0"; command: Converter.Zero; kind: CalcButton.Kind.Digit }
            ConverterButton {
                text: CalculatorEngine.decimalSeparator
                command: Converter.Decimal
                kind: CalcButton.Kind.Digit
                enabled: Converter.decimalEnabled
            }
        }
    }
}
