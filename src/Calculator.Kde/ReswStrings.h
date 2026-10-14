// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

// Loads the localized string tables (.resw files) shared with the Windows app.
//
// The .resw files live in src/Calculator/Resources/<lang>/ and are installed to
// <datadir>/calculator-kde/Resources/<lang>/. The best match for the user's UI
// languages is picked, falling back to en-US for any missing key.
class ReswStrings : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Strings)
    QML_SINGLETON

public:
    static ReswStrings* instance();
    static ReswStrings* create(class QQmlEngine*, class QJSEngine*);

    // Language folder that was selected, e.g. "de-DE".
    Q_PROPERTY(QString language READ language CONSTANT)
    QString language() const
    {
        return m_language;
    }

    // Returns the string for 'key' from Resources.resw, or 'fallback' if not found.
    Q_INVOKABLE QString get(const QString& key, const QString& fallback = {}) const;

    // Returns the string for 'key' from CEngineStrings.resw.
    QString engineString(const QString& key) const;

    // Parses a .resw file into a key/value table. Exposed for tests.
    static QHash<QString, QString> parseResw(const QString& path);

    // Directories searched for the Resources/<lang> folders.
    static QStringList resourceRoots();

private:
    explicit ReswStrings(QObject* parent = nullptr);
    void load();

    QString m_language;
    QHash<QString, QString> m_ui;
    QHash<QString, QString> m_uiFallback;
    QHash<QString, QString> m_engine;
    QHash<QString, QString> m_engineFallback;
};
