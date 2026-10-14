// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "UnitConverter.h"

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <memory>

class CurrencyDataLoader;
class UnitConverterDataLoader;

// Bridges UnitConversionManager::UnitConverter to QML.
// This plays the role of UnitConverterViewModel in the Windows app.
//
// The converter has two value fields. The active one takes input (the engine's
// "from" value) and the other shows the conversion; clicking the other field
// swaps their roles, as in the Windows app.
class UnitConverterController : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Converter)
    QML_SINGLETON

    Q_PROPERTY(QStringList categories READ categories CONSTANT)
    Q_PROPERTY(int categoryIndex READ categoryIndex WRITE setCategoryIndex NOTIFY categoryChanged)
    Q_PROPERTY(bool isCurrency READ isCurrency NOTIFY categoryChanged)
    Q_PROPERTY(bool supportsNegative READ supportsNegative NOTIFY categoryChanged)
    Q_PROPERTY(QStringList units READ units NOTIFY unitsChanged)
    Q_PROPERTY(QStringList unitAbbreviations READ unitAbbreviations NOTIFY unitsChanged)
    Q_PROPERTY(int unit1Index READ unit1Index WRITE setUnit1Index NOTIFY selectedUnitsChanged)
    Q_PROPERTY(int unit2Index READ unit2Index WRITE setUnit2Index NOTIFY selectedUnitsChanged)
    Q_PROPERTY(QString value1 READ value1 NOTIFY valuesChanged)
    Q_PROPERTY(QString value2 READ value2 NOTIFY valuesChanged)
    Q_PROPERTY(bool value1Active READ value1Active NOTIFY activeChanged)
    Q_PROPERTY(QVariantList supplementaryResults READ supplementaryResults NOTIFY valuesChanged)
    Q_PROPERTY(bool decimalEnabled READ decimalEnabled NOTIFY selectedUnitsChanged)
    Q_PROPERTY(QString currencySymbol1 READ currencySymbol1 NOTIFY currencyChanged)
    Q_PROPERTY(QString currencySymbol2 READ currencySymbol2 NOTIFY currencyChanged)
    Q_PROPERTY(QString currencyRatio READ currencyRatio NOTIFY currencyChanged)
    Q_PROPERTY(QString currencyTimestamp READ currencyTimestamp NOTIFY currencyChanged)
    Q_PROPERTY(bool currencyRatesOld READ currencyRatesOld NOTIFY currencyChanged)
    Q_PROPERTY(bool currencyLoading READ currencyLoading NOTIFY currencyChanged)
    Q_PROPERTY(bool currencyUnavailable READ currencyUnavailable NOTIFY currencyChanged)

public:
    // Mirrors UnitConversionManager::Command.
    enum Op
    {
        Zero = 0, One, Two, Three, Four, Five, Six, Seven, Eight, Nine,
        Decimal = 10,
        Negate = 11,
        Backspace = 12,
        Clear = 13,
    };
    Q_ENUM(Op)

    // 'currencyCacheFile' is for tests; by default the user's cache directory is used.
    explicit UnitConverterController(QObject* parent = nullptr);
    UnitConverterController(const QString& region, const QString& currencyCacheFile, bool currencyNetwork, QObject* parent = nullptr);
    ~UnitConverterController() override;

    QStringList categories() const { return m_categoryNames; }
    int categoryIndex() const { return m_categoryIndex; }
    void setCategoryIndex(int index);
    bool isCurrency() const;
    bool supportsNegative() const;
    QStringList units() const;
    QStringList unitAbbreviations() const;
    int unit1Index() const { return m_unit1; }
    void setUnit1Index(int index);
    int unit2Index() const { return m_unit2; }
    void setUnit2Index(int index);
    QString value1() const { return m_value1IsSource ? m_fromDisplay : m_toDisplay; }
    QString value2() const { return m_value1IsSource ? m_toDisplay : m_fromDisplay; }
    bool value1Active() const { return m_value1IsSource; }
    QVariantList supplementaryResults() const { return m_supplementary; }
    bool decimalEnabled() const;
    QString currencySymbol1() const { return m_symbol1; }
    QString currencySymbol2() const { return m_symbol2; }
    QString currencyRatio() const { return m_ratio; }
    QString currencyTimestamp() const { return m_timestamp; }
    bool currencyRatesOld() const { return m_ratesOld; }
    bool currencyLoading() const;
    bool currencyUnavailable() const;

    Q_INVOKABLE void sendCommand(int command);
    // Makes field 1 or 2 the input field.
    Q_INVOKABLE void activate(int field);
    Q_INVOKABLE void refreshCurrencyRates();
    Q_INVOKABLE bool handleKey(int key, int modifiers, const QString& text);
    Q_INVOKABLE void copyToClipboard();
    Q_INVOKABLE void pasteFromClipboard();
    Q_INVOKABLE bool pasteText(const QString& text);

    // Formats an engine number string ("1234.5", "1.2e+15") for display.
    static QString localizeNumber(const QString& raw);

    // Called by the engine callbacks.
    void onDisplay(const std::wstring& from, const std::wstring& to);
    void onSuggestedValues(const std::vector<std::tuple<std::wstring, UnitConversionManager::Unit>>& values);
    void onCurrencySymbols(const std::wstring& from, const std::wstring& to);
    void onCurrencyRatio(const std::wstring& ratio);

Q_SIGNALS:
    void categoryChanged();
    void unitsChanged();
    void selectedUnitsChanged();
    void valuesChanged();
    void activeChanged();
    void currencyChanged();
    void maxDigitsReached();
    void pasteFailed();
    void currencyRefreshFailed();

private:
    void init(const QString& region, const QString& currencyCacheFile, bool currencyNetwork);
    void applyCategory();
    void applyUnits();
    void onCurrencyDataChanged();
    void updateCurrencyInfo();
    QString formatValue(const std::wstring& raw, bool isInput, int unitIndex) const;
    int indexOfUnit(const UnitConversionManager::Unit& unit) const;
    int sourceIndex() const { return m_value1IsSource ? m_unit1 : m_unit2; }
    int targetIndex() const { return m_value1IsSource ? m_unit2 : m_unit1; }
    bool inputBlocked(int command) const;
    void saveSelection() const;

    std::shared_ptr<UnitConverterDataLoader> m_unitLoader;
    std::shared_ptr<CurrencyDataLoader> m_currencyLoader;
    std::shared_ptr<UnitConversionManager::UnitConverter> m_model;
    std::shared_ptr<UnitConversionManager::IUnitConverterVMCallback> m_callback;
    std::shared_ptr<UnitConversionManager::IViewModelCurrencyCallback> m_currencyCallback;

    std::vector<UnitConversionManager::Category> m_categories;
    QStringList m_categoryNames;
    int m_categoryIndex = -1;
    std::vector<UnitConversionManager::Unit> m_units;
    int m_unit1 = -1;
    int m_unit2 = -1;
    bool m_value1IsSource = true;
    bool m_persist = true;

    std::wstring m_rawFrom = L"0";
    std::wstring m_rawTo = L"0";
    QString m_fromDisplay = QStringLiteral("0");
    QString m_toDisplay = QStringLiteral("0");
    QVariantList m_supplementary;

    QString m_symbol1;
    QString m_symbol2;
    QString m_ratio;
    QString m_timestamp;
    bool m_ratesOld = false;
};
