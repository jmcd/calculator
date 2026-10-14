// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "UnitConverter.h"

#include <QString>

#include <map>

// Converter categories, in the order they're listed in the UI.
enum class ConverterCategory
{
    Currency = 0,
    Volume,
    Length,
    Weight,
    Temperature,
    Energy,
    Area,
    Speed,
    Time,
    Power,
    Data,
    Pressure,
    Angle,
};

// Port of UnitConverterDataLoader from the Windows app: the units, ratios and
// region-specific defaults for every category except currency.
class UnitConverterDataLoader final : public UnitConversionManager::IConverterDataLoader
{
public:
    // 'region' is a two-letter territory code such as "US" or "GB"; it decides
    // the default units (e.g. Fahrenheit vs Celsius).
    explicit UnitConverterDataLoader(QString region);

    void LoadData() override;
    std::vector<UnitConversionManager::Category> GetOrderedCategories() override;
    std::vector<UnitConversionManager::Unit> GetOrderedUnits(const UnitConversionManager::Category& category) override;
    std::unordered_map<UnitConversionManager::Unit, UnitConversionManager::ConversionData, UnitConversionManager::UnitHash>
    LoadOrderedRatios(const UnitConversionManager::Unit& unit) override;
    bool SupportsCategory(const UnitConversionManager::Category& target) override;

    static QString categoryName(ConverterCategory category);

private:
    QString m_region;
    bool m_loaded = false;
    std::vector<UnitConversionManager::Category> m_categories;
    std::map<int, std::vector<UnitConversionManager::Unit>> m_unitsByCategory;
    std::map<int, std::unordered_map<UnitConversionManager::Unit, UnitConversionManager::ConversionData, UnitConversionManager::UnitHash>>
        m_ratiosByUnit;
};
