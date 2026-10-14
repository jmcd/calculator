// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "CalculatorResource.h"

#include <QLocale>

// Supplies the strings the calculation engine needs (operator symbols, error
// messages and number formatting settings).
class EngineResourceProvider final : public CalculationManager::IResourceProvider
{
public:
    explicit EngineResourceProvider(const QLocale& locale = QLocale());

    std::wstring GetCEngineString(std::wstring_view id) override;

    QChar decimalSeparator() const
    {
        return m_decimal;
    }
    QChar groupSeparator() const
    {
        return m_group;
    }

private:
    QChar m_decimal;
    QChar m_group;
    QString m_grouping;
};
