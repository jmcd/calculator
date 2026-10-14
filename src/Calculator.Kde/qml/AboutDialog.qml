// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Calculator

Kirigami.PromptDialog {
    title: Strings.get("AboutGroupTitle.Text", "About")
    standardButtons: Kirigami.Dialog.Close

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        RowLayout {
            spacing: Kirigami.Units.largeSpacing
            Kirigami.Icon {
                source: "accessories-calculator"
                Layout.preferredWidth: Kirigami.Units.iconSizes.huge
                Layout.preferredHeight: Kirigami.Units.iconSizes.huge
            }
            ColumnLayout {
                Kirigami.Heading {
                    text: Strings.get("AppName", "Calculator")
                }
                QQC2.Label {
                    text: "Version " + Qt.application.version
                    opacity: 0.7
                }
            }
        }
        QQC2.Label {
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 22
            wrapMode: Text.Wrap
            text: "A KDE port of the open-source Windows Calculator. The calculation engine is shared with the Windows app; the interface is built with Qt and Kirigami."
        }
        Kirigami.UrlButton {
            url: "https://github.com/microsoft/calculator"
        }
        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            opacity: 0.7
            text: "Licensed under the MIT License."
        }
    }
}
