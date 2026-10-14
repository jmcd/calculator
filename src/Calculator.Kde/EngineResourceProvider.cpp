// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "EngineResourceProvider.h"
#include "ReswStrings.h"

EngineResourceProvider::EngineResourceProvider(const QLocale& locale)
{
    // The engine only supports single-character separators.
    const QString decimal = locale.decimalPoint();
    const QString group = locale.groupSeparator();
    m_decimal = decimal.size() == 1 ? decimal.front() : QChar(u'.');
    m_group = group.size() == 1 ? group.front() : QChar(u',');
    if (m_group == m_decimal)
    {
        m_group = m_decimal == u'.' ? QChar(u',') : QChar(u'.');
    }

    // Grouping uses the Windows LOCALE_SGROUPING format. Most locales group by
    // threes; the Indian numbering system groups by three, then by twos.
    m_grouping = locale.territory() == QLocale::India ? QStringLiteral("3;2;0") : QStringLiteral("3;0");
}

std::wstring EngineResourceProvider::GetCEngineString(std::wstring_view id)
{
    const QString key = QString::fromWCharArray(id.data(), static_cast<qsizetype>(id.size()));
    if (key == u"sDecimal")
    {
        return QString(m_decimal).toStdWString();
    }
    if (key == u"sThousand")
    {
        return QString(m_group).toStdWString();
    }
    if (key == u"sGrouping")
    {
        return m_grouping.toStdWString();
    }
    return ReswStrings::instance()->engineString(key).toStdWString();
}
