// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ReswStrings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QLocale>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QXmlStreamReader>

namespace
{
    const QString DefaultLanguage = QStringLiteral("en-US");

    // Picks the resource folder that best matches the user's UI languages:
    // an exact match (de-DE) first, then any folder for the same language (de-*).
    QString PickLanguage(const QStringList& available)
    {
        auto findCaseInsensitive = [&](const QString& name) -> QString {
            for (const auto& a : available)
            {
                if (a.compare(name, Qt::CaseInsensitive) == 0)
                {
                    return a;
                }
            }
            return {};
        };

        QStringList wanted = QLocale().uiLanguages();
        wanted.append(QLocale::system().uiLanguages());

        for (QString lang : wanted)
        {
            lang.replace(u'_', u'-');
            if (auto exact = findCaseInsensitive(lang); !exact.isEmpty())
            {
                return exact;
            }

            const QString prefix = lang.section(u'-', 0, 0) + u'-';
            // Prefer the "default" region for languages with several variants.
            static const QHash<QString, QString> preferred = {
                { QStringLiteral("en-"), QStringLiteral("en-US") }, { QStringLiteral("es-"), QStringLiteral("es-ES") },
                { QStringLiteral("fr-"), QStringLiteral("fr-FR") }, { QStringLiteral("pt-"), QStringLiteral("pt-BR") },
                { QStringLiteral("zh-"), QStringLiteral("zh-CN") },
            };
            if (auto it = preferred.find(prefix.toLower()); it != preferred.end() && available.contains(*it))
            {
                return *it;
            }
            for (const auto& a : available)
            {
                if (a.startsWith(prefix, Qt::CaseInsensitive))
                {
                    return a;
                }
            }
        }
        return DefaultLanguage;
    }
}

ReswStrings::ReswStrings(QObject* parent)
    : QObject(parent)
{
    load();
}

ReswStrings* ReswStrings::instance()
{
    static ReswStrings* s_instance = new ReswStrings(QCoreApplication::instance());
    return s_instance;
}

ReswStrings* ReswStrings::create(QQmlEngine*, QJSEngine*)
{
    auto* inst = instance();
    QQmlEngine::setObjectOwnership(inst, QQmlEngine::CppOwnership);
    return inst;
}

QStringList ReswStrings::resourceRoots()
{
    QStringList roots;
    if (const QString env = qEnvironmentVariable("CALCULATOR_RESOURCES_DIR"); !env.isEmpty())
    {
        roots << env;
    }
    for (const auto& dir : QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation))
    {
        roots << dir + QStringLiteral("/calculator-kde/Resources");
    }
    roots << QCoreApplication::applicationDirPath() + QStringLiteral("/../share/calculator-kde/Resources");
#ifdef CALCULATOR_SOURCE_RESOURCES_DIR
    // Running straight from the build tree.
    roots << QStringLiteral(CALCULATOR_SOURCE_RESOURCES_DIR);
#endif
    // en-US engine strings are always compiled in so the engine can work.
    roots << QStringLiteral(":/resources");
    return roots;
}

QHash<QString, QString> ReswStrings::parseResw(const QString& path)
{
    QHash<QString, QString> table;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        return table;
    }

    QXmlStreamReader xml(&file);
    QString currentName;
    while (!xml.atEnd())
    {
        xml.readNext();
        if (xml.isStartElement())
        {
            if (xml.name() == u"data")
            {
                currentName = xml.attributes().value(u"name").toString();
            }
            else if (xml.name() == u"value" && !currentName.isEmpty())
            {
                table.insert(currentName, xml.readElementText());
            }
        }
        else if (xml.isEndElement() && xml.name() == u"data")
        {
            currentName.clear();
        }
    }
    return table;
}

void ReswStrings::load()
{
    // Find the first root that has an en-US folder; that root holds all languages.
    QString root;
    for (const auto& candidate : resourceRoots())
    {
        if (QFile::exists(candidate + u'/' + DefaultLanguage + QStringLiteral("/CEngineStrings.resw")))
        {
            root = candidate;
            break;
        }
    }

    const QStringList available = QDir(root).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    m_language = PickLanguage(available);

    auto loadTable = [&](const QString& lang, const QString& file) { return parseResw(root + u'/' + lang + u'/' + file); };

    m_engineFallback = loadTable(DefaultLanguage, QStringLiteral("CEngineStrings.resw"));
    m_uiFallback = loadTable(DefaultLanguage, QStringLiteral("Resources.resw"));
    if (m_language != DefaultLanguage)
    {
        m_engine = loadTable(m_language, QStringLiteral("CEngineStrings.resw"));
        m_ui = loadTable(m_language, QStringLiteral("Resources.resw"));
    }

    if (m_engineFallback.isEmpty())
    {
        // Last resort: the copy compiled into the binary.
        m_engineFallback = parseResw(QStringLiteral(":/resources/en-US/CEngineStrings.resw"));
    }
}

QString ReswStrings::get(const QString& key, const QString& fallback) const
{
    if (auto it = m_ui.find(key); it != m_ui.end() && !it->isEmpty())
    {
        return *it;
    }
    if (auto it = m_uiFallback.find(key); it != m_uiFallback.end() && !it->isEmpty())
    {
        return *it;
    }
    return fallback.isEmpty() ? key : fallback;
}

QString ReswStrings::engineString(const QString& key) const
{
    if (auto it = m_engine.find(key); it != m_engine.end())
    {
        return *it;
    }
    return m_engineFallback.value(key);
}
