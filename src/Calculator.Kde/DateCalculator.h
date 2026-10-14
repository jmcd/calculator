// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <QDate>
#include <QObject>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

// Port of DateCalculationEngine / DateCalculatorViewModel from the Windows app,
// using the Gregorian calendar.
class DateCalculator : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit DateCalculator(QObject* parent = nullptr);

    struct Difference
    {
        int years = 0;
        int months = 0;
        int weeks = 0;
        int days = 0;
        qint64 totalDays = 0;
    };

    static Difference difference(QDate a, QDate b);

    // Returns { text, totalText, same } describing the difference between two dates.
    // Months are 1-based.
    Q_INVOKABLE QVariantMap differenceBetween(int y1, int m1, int d1, int y2, int m2, int d2) const;

    // Adds (or subtracts) a duration and returns { valid, year, month, day, text }.
    Q_INVOKABLE QVariantMap adjust(int year, int month, int day, bool subtract, int years, int months, int days) const;

    Q_INVOKABLE QString formatLongDate(int year, int month, int day) const;
};
