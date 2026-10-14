// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import Calculator

FormCard.FormCardPage {
    id: page

    title: Strings.get("DateCalculationModeText", "Date calculation")

    // 0: difference between dates, 1: add or subtract days
    property int calcMode: 0
    property date fromDate: new Date()
    property date toDate: new Date()
    property bool subtract: false

    readonly property var difference: DateCalculator.differenceBetween(
        fromDate.getFullYear(), fromDate.getMonth() + 1, fromDate.getDate(),
        toDate.getFullYear(), toDate.getMonth() + 1, toDate.getDate())

    readonly property var adjusted: DateCalculator.adjust(
        fromDate.getFullYear(), fromDate.getMonth() + 1, fromDate.getDate(),
        subtract, yearsBox.value, monthsBox.value, daysBox.value)

    FormCard.FormCard {
        Layout.topMargin: Kirigami.Units.largeSpacing

        FormCard.FormComboBoxDelegate {
            text: Strings.get("DateCalculationOption.[using:Windows.UI.Xaml.Automation]AutomationProperties.Name", "Calculation mode")
            model: [
                Strings.get("Date_DifferenceOption.Content", "Difference between dates"),
                Strings.get("Date_AddSubtractOption.Content", "Add or subtract days")
            ]
            currentIndex: page.calcMode
            onActivated: index => page.calcMode = index
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormDateTimeDelegate {
            text: Strings.get("DateDiff_FromHeader.Header", "From")
            dateTimeDisplay: FormCard.FormDateTimeDelegate.DateTimeDisplay.Date
            onValueChanged: page.fromDate = value
        }

        FormCard.FormDelegateSeparator { visible: page.calcMode === 0 }

        FormCard.FormDateTimeDelegate {
            visible: page.calcMode === 0
            text: Strings.get("DateDiff_ToHeader.Header", "To")
            dateTimeDisplay: FormCard.FormDateTimeDelegate.DateTimeDisplay.Date
            onValueChanged: page.toDate = value
        }
    }

    FormCard.FormCard {
        visible: page.calcMode === 1
        Layout.topMargin: Kirigami.Units.largeSpacing

        FormCard.FormRadioDelegate {
            text: Strings.get("AddOption.Content", "Add")
            checked: !page.subtract
            onToggled: if (checked) page.subtract = false
        }
        FormCard.FormRadioDelegate {
            text: Strings.get("SubtractOption.Content", "Subtract")
            checked: page.subtract
            onToggled: if (checked) page.subtract = true
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormSpinBoxDelegate {
            id: yearsBox
            label: Strings.get("YearsLabel.Text", "Years")
            from: 0
            to: 999
        }
        FormCard.FormSpinBoxDelegate {
            id: monthsBox
            label: Strings.get("MonthsLabel.Text", "Months")
            from: 0
            to: 999
        }
        FormCard.FormSpinBoxDelegate {
            id: daysBox
            label: Strings.get("DaysLabel.Text", "Days")
            from: 0
            to: 999
        }
    }

    FormCard.FormHeader {
        title: page.calcMode === 0 ? Strings.get("Date_DifferenceLabel.Text", "Difference") : "Date"
    }

    FormCard.FormCard {
        FormCard.AbstractFormDelegate {
            background: null
            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.SelectableLabel {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.6
                    font.weight: Font.DemiBold
                    text: page.calcMode === 0 ? page.difference.text : page.adjusted.text
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    visible: text.length > 0
                    opacity: 0.7
                    wrapMode: Text.Wrap
                    text: page.calcMode === 0 ? page.difference.totalText : ""
                }
            }
        }
    }
}
