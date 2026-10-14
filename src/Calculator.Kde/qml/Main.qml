// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

import QtCore
import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import Calculator

Kirigami.ApplicationWindow {
    id: root

    // Category shown in the window: one of the CalculatorEngine.Mode values, or DateMode.
    readonly property int dateMode: 100
    readonly property int converterMode: 200
    readonly property int graphingMode: 300
    property int category: CalculatorEngine.Standard

    title: categoryName(category)
    width: Kirigami.Units.gridUnit * 22
    height: Kirigami.Units.gridUnit * 34
    minimumWidth: Kirigami.Units.gridUnit * 16
    minimumHeight: Kirigami.Units.gridUnit * 24

    function categoryName(c) {
        switch (c) {
        case CalculatorEngine.Scientific: return Strings.get("ScientificModeText", "Scientific")
        case CalculatorEngine.Programmer: return Strings.get("ProgrammerModeText", "Programmer")
        case dateMode: return Strings.get("DateCalculationModeText", "Date calculation")
        case graphingMode: return Strings.get("GraphingCalculatorModeText", "Graphing")
        case converterMode: return Converter.categories[Converter.categoryIndex] ?? Strings.get("ConverterModeText", "Converter")
        default: return Strings.get("StandardModeText", "Standard")
        }
    }

    function showCategory(c) {
        category = c
        if (c !== dateMode && c !== converterMode && c !== graphingMode) {
            CalculatorEngine.mode = c
        }
        const page = c === dateMode ? dateCalculatorPage
                   : c === converterMode ? converterPage
                   : c === graphingMode ? graphingPage
                   : calculatorPage
        if (pageStack.depth === 0) {
            pageStack.push(page)
        } else {
            pageStack.replace(page)
        }
    }

    Settings {
        id: settings
        property int lastCategory: CalculatorEngine.Standard
        property int angleMode: CalculatorEngine.Degrees
        property alias width: root.width
        property alias height: root.height
    }

    Component.onCompleted: {
        CalculatorEngine.angleMode = settings.angleMode
        showCategory(settings.lastCategory)
    }
    onCategoryChanged: settings.lastCategory = category
    Connections {
        target: CalculatorEngine
        function onAngleModeChanged() { settings.angleMode = CalculatorEngine.angleMode }
    }

    // Alt+1..Alt+5 switch modes, in the same order as the Windows app.
    Shortcut { sequence: "Alt+1"; onActivated: root.showCategory(CalculatorEngine.Standard) }
    Shortcut { sequence: "Alt+2"; onActivated: root.showCategory(CalculatorEngine.Scientific) }
    Shortcut { sequence: "Alt+3"; onActivated: root.showCategory(root.graphingMode) }
    Shortcut { sequence: "Alt+4"; onActivated: root.showCategory(CalculatorEngine.Programmer) }
    Shortcut { sequence: "Alt+5"; onActivated: root.showCategory(root.dateMode) }

    function showConverter(index) {
        Converter.categoryIndex = index
        showCategory(converterMode)
    }

    // One drawer entry per converter category.
    component ConverterAction: Kirigami.Action {
        required property int index
        text: Converter.categories[index]
        checkable: true
        checked: root.category === root.converterMode && Converter.categoryIndex === index
        onTriggered: root.showConverter(index)
    }

    globalDrawer: Kirigami.GlobalDrawer {
        modal: true
        collapsible: false
        title: Strings.get("AppName", "Calculator")
        titleIcon: "accessories-calculator"

        actions: [
            Kirigami.Action {
                text: Strings.get("StandardModeText", "Standard")
                icon.name: "accessories-calculator"
                checkable: true
                checked: root.category === CalculatorEngine.Standard
                onTriggered: root.showCategory(CalculatorEngine.Standard)
            },
            Kirigami.Action {
                text: Strings.get("ScientificModeText", "Scientific")
                icon.name: "format-text-superscript"
                checkable: true
                checked: root.category === CalculatorEngine.Scientific
                onTriggered: root.showCategory(CalculatorEngine.Scientific)
            },
            Kirigami.Action {
                text: Strings.get("GraphingCalculatorModeText", "Graphing")
                icon.name: "labplot-xy-curve"
                checkable: true
                checked: root.category === root.graphingMode
                onTriggered: root.showCategory(root.graphingMode)
            },
            Kirigami.Action {
                text: Strings.get("ProgrammerModeText", "Programmer")
                icon.name: "code-context"
                checkable: true
                checked: root.category === CalculatorEngine.Programmer
                onTriggered: root.showCategory(CalculatorEngine.Programmer)
            },
            Kirigami.Action {
                text: Strings.get("DateCalculationModeText", "Date calculation")
                icon.name: "view-calendar"
                checkable: true
                checked: root.category === root.dateMode
                onTriggered: root.showCategory(root.dateMode)
            },
            Kirigami.Action {
                text: Strings.get("ConverterModeText", "Converter")
                icon.name: "measure"
                checked: root.category === root.converterMode

                ConverterAction { index: 0; icon.name: "taxes-finances" }
                ConverterAction { index: 1; icon.name: "kt-set-max-upload-speed" }
                ConverterAction { index: 2; icon.name: "measure" }
                ConverterAction { index: 3; icon.name: "weight" }
                ConverterAction { index: 4; icon.name: "temperature-normal" }
                ConverterAction { index: 5; icon.name: "battery-full" }
                ConverterAction { index: 6; icon.name: "draw-rectangle" }
                ConverterAction { index: 7; icon.name: "speedometer" }
                ConverterAction { index: 8; icon.name: "chronometer" }
                ConverterAction { index: 9; icon.name: "battery-charging" }
                ConverterAction { index: 10; icon.name: "drive-harddisk" }
                ConverterAction { index: 11; icon.name: "weather-clouds" }
                ConverterAction { index: 12; icon.name: "draw-arc" }
            },
            Kirigami.Action {
                separator: true
            },
            Kirigami.Action {
                text: Strings.get("AboutGroupTitle.Text", "About")
                icon.name: "help-about"
                onTriggered: aboutDialog.open()
            }
        ]
    }

    Component {
        id: calculatorPage
        CalculatorPage {}
    }

    Component {
        id: graphingPage
        GraphingPage {}
    }

    Component {
        id: converterPage
        ConverterPage {}
    }

    Component {
        id: dateCalculatorPage
        DateCalculatorPage {}
    }

    AboutDialog {
        id: aboutDialog
    }
}
