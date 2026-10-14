// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Calculator

Kirigami.Page {
    id: page

    // Side by side when there's room; otherwise switch between equations and graph.
    readonly property bool wide: width > Kirigami.Units.gridUnit * 36
    property bool showGraphOnly: false

    title: Strings.get("GraphingCalculatorModeText", "Graphing")
    padding: 0

    actions: [
        Kirigami.Action {
            visible: !page.wide
            icon.name: page.showGraphOnly ? "document-edit" : "labplot-xy-curve"
            text: page.showGraphOnly
                  ? Strings.get("GraphSwitchToEquationMode", "Switch to equation mode")
                  : Strings.get("GraphSwitchToGraphMode", "Switch to graph mode")
            displayHint: Kirigami.DisplayHint.KeepVisible
            onTriggered: page.showGraphOnly = !page.showGraphOnly
        }
    ]

    // Windows: Ctrl + plus / minus zoom the graph.
    Shortcut { sequences: ["Ctrl++", "Ctrl+="]; onActivated: graph.zoom(0.8) }
    Shortcut { sequence: "Ctrl+-"; onActivated: graph.zoom(1.25) }
    Shortcut { sequence: "Ctrl+0"; onActivated: graph.resetView() }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Equations and variables.
        QQC2.ScrollView {
            id: inputPane
            visible: page.wide || !page.showGraphOnly
            Layout.fillHeight: true
            Layout.fillWidth: !page.wide
            Layout.preferredWidth: Kirigami.Units.gridUnit * 20
            QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff

            ColumnLayout {
                width: inputPane.availableWidth
                spacing: Kirigami.Units.smallSpacing

                Repeater {
                    model: Graphing.equations
                    delegate: EquationRow {}
                }

                Kirigami.Heading {
                    visible: variableRepeater.count > 0
                    Layout.topMargin: Kirigami.Units.largeSpacing
                    Layout.leftMargin: Kirigami.Units.largeSpacing
                    level: 4
                    text: Strings.get("VariablesShareHeader", "Variables")
                }

                Repeater {
                    id: variableRepeater
                    model: Graphing.variables
                    delegate: VariableRow {}
                }

                Item { Layout.preferredHeight: Kirigami.Units.largeSpacing }
            }
        }

        Kirigami.Separator {
            visible: page.wide
            Layout.fillHeight: true
        }

        // The graph, with zoom and option buttons.
        Item {
            visible: page.wide || page.showGraphOnly
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            GraphView {
                id: graph
                anchors.fill: parent
                controller: Graphing
                backgroundColor: Kirigami.Theme.backgroundColor
                textColor: Kirigami.Theme.textColor
                Kirigami.Theme.colorSet: Kirigami.Theme.View
                Kirigami.Theme.inherit: false
                Accessible.role: Accessible.Graphic
                Accessible.name: Strings.get("GraphImageAltText", "Image of a graph with equations")
                Accessible.description: tracing ? traceText : ""

                PinchHandler {
                    target: null
                    onScaleChanged: delta => graph.zoomAt(1 / delta, centroid.position.x, centroid.position.y)
                }
            }

            ColumnLayout {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: Kirigami.Units.smallSpacing
                spacing: Kirigami.Units.smallSpacing

                component GraphButton: QQC2.Button {
                    display: QQC2.AbstractButton.IconOnly
                    focusPolicy: Qt.NoFocus
                    QQC2.ToolTip.text: text
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                }

                GraphButton {
                    icon.name: "zoom-in"
                    text: Strings.get("zoomInButton.[using:Windows.UI.Xaml.Controls]ToolTipService.ToolTip", "Zoom in (Ctrl + plus)")
                    onClicked: graph.zoom(0.8)
                }
                GraphButton {
                    icon.name: "zoom-out"
                    text: Strings.get("zoomOutButton.[using:Windows.UI.Xaml.Controls]ToolTipService.ToolTip", "Zoom out (Ctrl + minus)")
                    onClicked: graph.zoom(1.25)
                }
                GraphButton {
                    icon.name: "zoom-original"
                    text: Strings.get("ResetViewButton.Content", "Reset view") + " (Ctrl+0)"
                    onClicked: graph.resetView()
                }
                GraphButton {
                    icon.name: "configure"
                    text: Strings.get("graphSettingsButton.[using:Windows.UI.Xaml.Controls]ToolTipService.ToolTip", "Graph options")
                    onClicked: optionsDialog.open()
                }
            }
        }
    }

    // One equation: color chip (click to show/hide), input, remove button and error.
    component EquationRow: ColumnLayout {
        id: row
        required property int index
        required property string text
        required property int colorIndex
        required property bool shown
        required property string error
        readonly property color color: Graphing.equationColor(colorIndex, graph.darkBackground)

        Layout.fillWidth: true
        Layout.leftMargin: Kirigami.Units.smallSpacing
        Layout.rightMargin: Kirigami.Units.smallSpacing
        Layout.topMargin: index === 0 ? Kirigami.Units.smallSpacing : 0
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            QQC2.AbstractButton {
                id: chip
                Layout.preferredWidth: Kirigami.Units.gridUnit * 1.4
                Layout.preferredHeight: field.implicitHeight
                focusPolicy: Qt.NoFocus
                enabled: row.text.length > 0
                onClicked: Graphing.setEquationShown(row.index, !row.shown)
                QQC2.ToolTip.text: row.shown
                                   ? Strings.get("hideEquationButtonToolTip", "Hide equation")
                                   : Strings.get("showEquationButtonToolTip", "Show equation")
                QQC2.ToolTip.visible: hovered && enabled
                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                Accessible.name: QQC2.ToolTip.text

                contentItem: Item {
                    Rectangle {
                        anchors.centerIn: parent
                        width: Kirigami.Units.gridUnit * 0.9
                        height: width
                        radius: width / 2
                        color: row.shown && row.text.length > 0 ? row.color : "transparent"
                        border.color: row.color
                        border.width: 2
                        opacity: chip.enabled ? 1 : 0.5
                    }
                }
            }

            QQC2.TextField {
                id: field
                Layout.fillWidth: true
                text: row.text
                placeholderText: Strings.get("mathRichEditBox.PlaceholderText", "Enter an expression")
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.15
                Accessible.name: Strings.get("EquationInputButton.[using:Windows.UI.Xaml.Automation]AutomationProperties.Name", "Function input")
                onTextEdited: Graphing.setEquationText(row.index, text)
            }

            QQC2.ToolButton {
                visible: row.text.length > 0
                icon.name: "edit-delete-remove"
                display: QQC2.AbstractButton.IconOnly
                text: Strings.get("removeMenuItem", "Remove equation")
                focusPolicy: Qt.NoFocus
                QQC2.ToolTip.text: text
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                onClicked: Graphing.removeEquation(row.index)
            }
        }

        QQC2.Label {
            visible: row.error.length > 0
            Layout.fillWidth: true
            Layout.leftMargin: chip.width + Kirigami.Units.smallSpacing
            text: row.error
            color: Kirigami.Theme.negativeTextColor
            wrapMode: Text.Wrap
            font: Kirigami.Theme.smallFont
        }
    }

    function parseNumber(text) {
        const n = Number.fromLocaleString(Qt.locale(), text.trim())
        return isNaN(n) ? Number(text.trim()) : n
    }

    // A labelled number field for a variable's min, step or max.
    component LimitField: ColumnLayout {
        id: limit
        property string label
        property double number
        signal accepted(double value)
        Layout.fillWidth: true
        spacing: 0
        QQC2.Label {
            text: limit.label
            font: Kirigami.Theme.smallFont
            opacity: 0.7
        }
        QQC2.TextField {
            Layout.fillWidth: true
            text: String(Number(limit.number.toPrecision(10)))
            inputMethodHints: Qt.ImhFormattedNumbersOnly
            onEditingFinished: {
                const n = page.parseNumber(text)
                if (!isNaN(n)) {
                    limit.accepted(n)
                }
                text = Qt.binding(() => String(Number(limit.number.toPrecision(10))))
            }
        }
    }

    // A variable: value, slider, and (toggled) min / step / max.
    component VariableRow: ColumnLayout {
        id: variable
        required property int index
        required property string name
        required property double value
        required property double min
        required property double max
        required property double step
        property bool showSettings: false


        Layout.fillWidth: true
        Layout.leftMargin: Kirigami.Units.largeSpacing
        Layout.rightMargin: Kirigami.Units.smallSpacing
        spacing: 0

        RowLayout {
            Layout.fillWidth: true

            QQC2.Label {
                text: variable.name + " ="
                font.italic: true
                font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.15
            }
            QQC2.TextField {
                id: valueField
                Layout.preferredWidth: Kirigami.Units.gridUnit * 5
                text: String(Number(variable.value.toPrecision(10)))
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                Accessible.name: Strings.get("VariableValueTextBox.[using:Windows.UI.Xaml.Automation]AutomationProperties.Name", "Variable value textbox")
                onEditingFinished: {
                    const n = page.parseNumber(text)
                    if (!isNaN(n)) {
                        Graphing.setVariableValue(variable.index, n)
                    }
                    text = Qt.binding(() => String(Number(variable.value.toPrecision(10))))
                }
            }
            QQC2.Slider {
                Layout.fillWidth: true
                from: variable.min
                to: variable.max
                stepSize: variable.step
                snapMode: QQC2.Slider.SnapAlways
                value: variable.value
                focusPolicy: Qt.NoFocus
                Accessible.name: Strings.get("VariableValueSlider.[using:Windows.UI.Xaml.Automation]AutomationProperties.Name", "Variable value slider")
                onMoved: Graphing.setVariableValue(variable.index, value)
            }
            QQC2.ToolButton {
                icon.name: "configure"
                display: QQC2.AbstractButton.IconOnly
                checkable: true
                checked: variable.showSettings
                onToggled: variable.showSettings = checked
                text: Strings.get("VariableAreaSettings.[using:Windows.UI.Xaml.Controls]ToolTipService.ToolTip", "Toggle variable options")
                QQC2.ToolTip.text: text
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            }
        }

        RowLayout {
            visible: variable.showSettings
            Layout.fillWidth: true
            Layout.bottomMargin: Kirigami.Units.smallSpacing

            LimitField {
                label: Strings.get("MinTextBlock.Text", "Min")
                number: variable.min
                onAccepted: value => Graphing.setVariableMin(variable.index, value)
            }
            LimitField {
                label: Strings.get("StepTextBlock.Text", "Step")
                number: variable.step
                onAccepted: value => Graphing.setVariableStep(variable.index, value)
            }
            LimitField {
                label: Strings.get("MaxTextBlock.Text", "Max")
                number: variable.max
                onAccepted: value => Graphing.setVariableMax(variable.index, value)
            }
        }
    }

    Kirigami.Dialog {
        id: optionsDialog
        title: Strings.get("GraphOptionsHeading.Text", "Graph options")
        standardButtons: Kirigami.Dialog.Close
        padding: Kirigami.Units.largeSpacing

        function number(text) {
            const n = Number.fromLocaleString(Qt.locale(), text.trim())
            return isNaN(n) ? Number(text.trim()) : n
        }
        function show(n) {
            return String(Number(n.toPrecision(6)))
        }
        function applyWindow() {
            const ok = graph.setWindow(number(xMinField.text), number(xMaxField.text), number(yMinField.text), number(yMaxField.text))
            windowError.visible = !ok
        }

        function fill() {
            xMinField.text = show(graph.xMin)
            xMaxField.text = show(graph.xMax)
            yMinField.text = show(graph.yMin)
            yMaxField.text = show(graph.yMax)
            windowError.visible = false
        }
        onOpened: fill()

        Kirigami.FormLayout {
            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: Strings.get("GridHeading.Text", "Window")
            }
            QQC2.TextField {
                id: xMinField
                Kirigami.FormData.label: Strings.get("GraphSettingsXMin.Header", "X-Min")
                onEditingFinished: optionsDialog.applyWindow()
            }
            QQC2.TextField {
                id: xMaxField
                Kirigami.FormData.label: Strings.get("GraphSettingsXMax.Header", "X-Max")
                onEditingFinished: optionsDialog.applyWindow()
            }
            QQC2.TextField {
                id: yMinField
                Kirigami.FormData.label: Strings.get("GraphSettingsYMin.Header", "Y-Min")
                onEditingFinished: optionsDialog.applyWindow()
            }
            QQC2.TextField {
                id: yMaxField
                Kirigami.FormData.label: Strings.get("GraphSettingsYMax.Header", "Y-Max")
                onEditingFinished: optionsDialog.applyWindow()
            }
            QQC2.Label {
                id: windowError
                visible: false
                text: "Min must be less than max."
                color: Kirigami.Theme.negativeTextColor
            }
            QQC2.Button {
                text: Strings.get("ResetViewButton.Content", "Reset view")
                icon.name: "zoom-original"
                onClicked: {
                    graph.resetView()
                    optionsDialog.fill()
                }
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: Strings.get("UnitsHeading.Text", "Units")
            }
            QQC2.RadioButton {
                text: Strings.get("TrigModeRadians.Content", "Radians")
                checked: Graphing.angleUnit === Graphing.Radians
                onToggled: if (checked) Graphing.angleUnit = Graphing.Radians
            }
            QQC2.RadioButton {
                text: Strings.get("TrigModeDegrees.Content", "Degrees")
                checked: Graphing.angleUnit === Graphing.Degrees
                onToggled: if (checked) Graphing.angleUnit = Graphing.Degrees
            }
            QQC2.RadioButton {
                text: Strings.get("TrigModeGradians.Content", "Gradians")
                checked: Graphing.angleUnit === Graphing.Gradians
                onToggled: if (checked) Graphing.angleUnit = Graphing.Gradians
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: Strings.get("LineThicknessBoxHeading.Text", "Line thickness")
            }
            QQC2.Slider {
                from: 1
                to: 6
                stepSize: 0.5
                value: graph.lineWidth
                onMoved: graph.lineWidth = value
            }
        }
    }
}
