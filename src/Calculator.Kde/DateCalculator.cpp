// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "DateCalculator.h"
#include "ReswStrings.h"

#include <QLocale>

namespace
{
    // Same range the Windows calendar accepts.
    constexpr int MinYear = 1601;
    constexpr int MaxYear = 9999;

    QString Pluralize(int value, const char* singularKey, const char* pluralKey, const char* singular, const char* plural)
    {
        auto* strings = ReswStrings::instance();
        const QString unit = value == 1 ? strings->get(QString::fromLatin1(singularKey), QString::fromLatin1(singular))
                                        : strings->get(QString::fromLatin1(pluralKey), QString::fromLatin1(plural));
        return QLocale().toString(value) + u' ' + unit;
    }
}

DateCalculator::DateCalculator(QObject* parent)
    : QObject(parent)
{
}

DateCalculator::Difference DateCalculator::difference(QDate a, QDate b)
{
    QDate start = std::min(a, b);
    const QDate end = std::max(a, b);
    Difference diff;
    diff.totalDays = start.daysTo(end);

    // Largest whole number of years, then months, that fit; the rest is weeks and days.
    while (start.addYears(diff.years + 1) <= end)
    {
        ++diff.years;
    }
    QDate pivot = start.addYears(diff.years);
    while (pivot.addMonths(diff.months + 1) <= end)
    {
        ++diff.months;
    }
    pivot = pivot.addMonths(diff.months);

    const qint64 remaining = pivot.daysTo(end);
    diff.weeks = static_cast<int>(remaining / 7);
    diff.days = static_cast<int>(remaining % 7);
    return diff;
}

QVariantMap DateCalculator::differenceBetween(int y1, int m1, int d1, int y2, int m2, int d2) const
{
    QVariantMap result;
    const QDate a(y1, m1, d1);
    const QDate b(y2, m2, d2);
    auto* strings = ReswStrings::instance();

    if (!a.isValid() || !b.isValid())
    {
        result[QStringLiteral("text")] = strings->get(QStringLiteral("CalculationFailed"), QStringLiteral("Calculation failed"));
        result[QStringLiteral("totalText")] = QString();
        result[QStringLiteral("same")] = false;
        return result;
    }

    const Difference diff = difference(a, b);
    const QString totalText = Pluralize(static_cast<int>(diff.totalDays), "Date_Day", "Date_Days", "day", "days");

    if (diff.totalDays == 0)
    {
        result[QStringLiteral("text")] = strings->get(QStringLiteral("Date_SameDates"), QStringLiteral("Same dates"));
        result[QStringLiteral("totalText")] = QString();
        result[QStringLiteral("same")] = true;
        return result;
    }

    if (diff.years == 0 && diff.months == 0 && diff.weeks == 0)
    {
        result[QStringLiteral("text")] = totalText;
        result[QStringLiteral("totalText")] = QString();
        result[QStringLiteral("same")] = false;
        return result;
    }

    QStringList parts;
    if (diff.years > 0)
    {
        parts << Pluralize(diff.years, "Date_Year", "Date_Years", "year", "years");
    }
    if (diff.months > 0)
    {
        parts << Pluralize(diff.months, "Date_Month", "Date_Months", "month", "months");
    }
    if (diff.weeks > 0)
    {
        parts << Pluralize(diff.weeks, "Date_Week", "Date_Weeks", "week", "weeks");
    }
    if (diff.days > 0)
    {
        parts << Pluralize(diff.days, "Date_Day", "Date_Days", "day", "days");
    }

    result[QStringLiteral("text")] = parts.join(QStringLiteral(", "));
    result[QStringLiteral("totalText")] = totalText;
    result[QStringLiteral("same")] = false;
    return result;
}

QVariantMap DateCalculator::adjust(int year, int month, int day, bool subtract, int years, int months, int days) const
{
    QVariantMap result;
    QDate date(year, month, day);
    if (date.isValid())
    {
        // Matches DateCalculationEngine: add largest units first, subtract smallest first.
        if (subtract)
        {
            date = date.addDays(-days).addMonths(-months).addYears(-years);
        }
        else
        {
            date = date.addYears(years).addMonths(months).addDays(days);
        }
    }

    const bool valid = date.isValid() && date.year() >= MinYear && date.year() <= MaxYear;
    result[QStringLiteral("valid")] = valid;
    if (valid)
    {
        result[QStringLiteral("year")] = date.year();
        result[QStringLiteral("month")] = date.month();
        result[QStringLiteral("day")] = date.day();
        result[QStringLiteral("text")] = QLocale().toString(date, QLocale::LongFormat);
    }
    else
    {
        result[QStringLiteral("text")] = ReswStrings::instance()->get(QStringLiteral("Date_OutOfBoundMessage"), QStringLiteral("Date out of Bound"));
    }
    return result;
}

QString DateCalculator::formatLongDate(int year, int month, int day) const
{
    return QLocale().toString(QDate(year, month, day), QLocale::LongFormat);
}
