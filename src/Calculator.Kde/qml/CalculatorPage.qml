// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Calculator

Kirigami.Page {
    id: page

    readonly property int mode: CalculatorEngine.mode
    // Dock history/memory beside the keypad when there's room, like Windows does.
    readonly property bool wide: width > Kirigami.Units.gridUnit * 32

    title: applicationWindow().categoryName(mode)
    padding: Kirigami.Units.smallSpacing
    focus: true

    function openPanel(tab) {
        if (wide) {
            sidePanel.currentTab = tab
        } else {
            drawerPanel.currentTab = tab
            drawer.open()
        }
    }

    actions: [
        Kirigami.Action {
            icon.name: "view-history"
            text: Strings.get("HistoryPane", "History")
            displayHint: Kirigami.DisplayHint.IconOnly
            visible: !page.wide
            shortcut: "Ctrl+H"
            onTriggered: drawer.opened ? drawer.close() : page.openPanel(page.mode === CalculatorEngine.Programmer ? 1 : 0)
        },
        Kirigami.Action {
            icon.name: "edit-copy"
            text: Strings.get("copyMenuItem", "Copy")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: CalculatorEngine.copyToClipboard()
        },
        Kirigami.Action {
            icon.name: "edit-paste"
            text: Strings.get("pasteMenuItem", "Paste")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: CalculatorEngine.pasteFromClipboard()
        }
    ]

    Keys.onPressed: event => {
        if (CalculatorEngine.handleKey(event.key, event.modifiers, event.text)) {
            event.accepted = true
        }
    }

    Component.onCompleted: forceActiveFocus()

    Connections {
        target: CalculatorEngine
        function onPasteFailed() {
            applicationWindow().showPassiveNotification(Strings.get("CannotPaste", "This expression cannot be pasted"))
        }
        function onNoRightParenAdded() {
            applicationWindow().showPassiveNotification(Strings.get("NoRightParenthesisAdded_Announcement", "There are no open parentheses to close."))
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: Kirigami.Units.smallSpacing

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: Kirigami.Units.gridUnit * 15
            spacing: Kirigami.Units.smallSpacing

            Display {
                Layout.fillWidth: true
            }

            ProgrammerDisplay {
                visible: page.mode === CalculatorEngine.Programmer
                Layout.fillWidth: true
            }

            MemoryRow {
                visible: page.mode !== CalculatorEngine.Programmer
                Layout.fillWidth: true
                showListButton: !page.wide
                onMemoryListRequested: page.openPanel(1)
            }

            Loader {
                Layout.fillWidth: true
                Layout.fillHeight: true
                sourceComponent: {
                    switch (page.mode) {
                    case CalculatorEngine.Scientific: return scientificKeypad
                    case CalculatorEngine.Programmer: return programmerKeypad
                    default: return standardKeypad
                    }
                }
            }
        }

        Kirigami.Separator {
            visible: page.wide
            Layout.fillHeight: true
        }

        HistoryMemoryPanel {
            id: sidePanel
            visible: page.wide
            Layout.fillHeight: true
            Layout.preferredWidth: Kirigami.Units.gridUnit * 14
            Layout.maximumWidth: Kirigami.Units.gridUnit * 18
        }
    }

    Component {
        id: standardKeypad
        StandardKeypad {}
    }
    Component {
        id: scientificKeypad
        ScientificKeypad {}
    }
    Component {
        id: programmerKeypad
        ProgrammerKeypad {
            showMemoryListButton: !page.wide
            onMemoryListRequested: page.openPanel(1)
        }
    }

    Kirigami.OverlayDrawer {
        id: drawer
        edge: Qt.BottomEdge
        modal: true
        maximumSize: page.height * 0.7
        onClosed: page.forceActiveFocus()

        contentItem: HistoryMemoryPanel {
            id: drawerPanel
            implicitHeight: page.height * 0.7
            onItemActivated: drawer.close()
        }
    }
}
