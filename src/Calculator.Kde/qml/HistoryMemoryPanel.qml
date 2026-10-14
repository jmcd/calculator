// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Calculator

// History and Memory lists, shown docked beside the keypad on wide windows
// or in a drawer on narrow ones.
ColumnLayout {
    id: panel
    spacing: 0

    // 0: history, 1: memory
    property int currentTab: 0
    readonly property bool historyAvailable: CalculatorEngine.mode !== CalculatorEngine.Programmer
    readonly property int effectiveTab: historyAvailable ? currentTab : 1

    signal itemActivated()

    QQC2.TabBar {
        id: tabs
        Layout.fillWidth: true
        currentIndex: panel.historyAvailable ? panel.currentTab : 0
        onCurrentIndexChanged: if (panel.historyAvailable) panel.currentTab = currentIndex

        QQC2.TabButton {
            visible: panel.historyAvailable
            width: visible ? implicitWidth : 0
            focusPolicy: Qt.NoFocus
            text: Strings.get("HistoryPane", "History")
        }
        QQC2.TabButton {
            focusPolicy: Qt.NoFocus
            text: Strings.get("MemoryPane", "Memory")
        }
    }

    StackLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: panel.effectiveTab

        // History
        QQC2.ScrollView {
            QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff

            ListView {
                id: historyList
                clip: true
                model: CalculatorEngine.history

                delegate: QQC2.ItemDelegate {
                    id: historyDelegate
                    required property var modelData
                    width: ListView.view.width
                    focusPolicy: Qt.NoFocus
                    onClicked: {
                        CalculatorEngine.historyLoad(modelData.index)
                        panel.itemActivated()
                    }

                    contentItem: ColumnLayout {
                        spacing: 0
                        QQC2.Label {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                            wrapMode: Text.WrapAnywhere
                            opacity: 0.7
                            text: historyDelegate.modelData.expression
                        }
                        QQC2.Label {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                            wrapMode: Text.WrapAnywhere
                            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.5
                            font.weight: Font.DemiBold
                            text: historyDelegate.modelData.result
                        }
                    }

                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: historyMenu.popup()
                    }
                    QQC2.Menu {
                        id: historyMenu
                        QQC2.MenuItem {
                            text: Strings.get("copyMenuItem", "Copy")
                            icon.name: "edit-copy"
                            onTriggered: CalculatorEngine.copyText(historyDelegate.modelData.result)
                        }
                        QQC2.MenuItem {
                            text: "Delete"
                            icon.name: "edit-delete"
                            onTriggered: CalculatorEngine.historyRemove(historyDelegate.modelData.index)
                        }
                    }
                }

                Kirigami.PlaceholderMessage {
                    anchors.centerIn: parent
                    width: parent.width - Kirigami.Units.gridUnit * 2
                    visible: historyList.count === 0
                    icon.name: "view-history"
                    text: Strings.get("HistoryEmpty.Text", "There's no history yet.")
                }
            }
        }

        // Memory
        QQC2.ScrollView {
            QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff

            ListView {
                id: memoryList
                clip: true
                model: CalculatorEngine.memory

                delegate: QQC2.ItemDelegate {
                    id: memoryDelegate
                    required property int index
                    required property string modelData
                    width: ListView.view.width
                    focusPolicy: Qt.NoFocus
                    onClicked: {
                        CalculatorEngine.memoryItemLoad(index)
                        panel.itemActivated()
                    }

                    contentItem: ColumnLayout {
                        spacing: 0
                        QQC2.Label {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                            wrapMode: Text.WrapAnywhere
                            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.5
                            font.weight: Font.DemiBold
                            text: memoryDelegate.modelData
                        }
                        RowLayout {
                            Layout.alignment: Qt.AlignRight
                            spacing: 0
                            opacity: memoryDelegate.hovered || Kirigami.Settings.tabletMode ? 1 : 0
                            QQC2.ToolButton {
                                focusPolicy: Qt.NoFocus
                                text: "MC"
                                QQC2.ToolTip.text: Strings.get("ClearMemoryItemButton.[using:Windows.UI.Xaml.Controls]ToolTipService.ToolTip", "Clear memory item")
                                QQC2.ToolTip.visible: hovered
                                onClicked: CalculatorEngine.memoryItemClear(memoryDelegate.index)
                            }
                            QQC2.ToolButton {
                                focusPolicy: Qt.NoFocus
                                text: "M+"
                                onClicked: CalculatorEngine.memoryItemAdd(memoryDelegate.index)
                            }
                            QQC2.ToolButton {
                                focusPolicy: Qt.NoFocus
                                text: "M−"
                                onClicked: CalculatorEngine.memoryItemSubtract(memoryDelegate.index)
                            }
                        }
                    }
                }

                Kirigami.PlaceholderMessage {
                    anchors.centerIn: parent
                    width: parent.width - Kirigami.Units.gridUnit * 2
                    visible: memoryList.count === 0
                    icon.name: "media-flash"
                    text: Strings.get("MemoryPaneEmpty.Text", "There's nothing saved in memory.")
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.margins: Kirigami.Units.smallSpacing
        Item { Layout.fillWidth: true }
        QQC2.ToolButton {
            focusPolicy: Qt.NoFocus
            icon.name: "edit-clear-history"
            display: QQC2.AbstractButton.IconOnly
            visible: panel.effectiveTab === 0 ? historyList.count > 0 : memoryList.count > 0
            text: panel.effectiveTab === 0
                  ? Strings.get("ClearHistory.Name", "Clear all history") + " (Ctrl+Shift+D)"
                  : Strings.get("ClearMemoryButton.[using:Windows.UI.Xaml.Controls]ToolTipService.ToolTip", "Clear all memory (Ctrl+L)")
            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            onClicked: panel.effectiveTab === 0 ? CalculatorEngine.historyClear() : CalculatorEngine.memoryClearAll()
        }
    }
}
