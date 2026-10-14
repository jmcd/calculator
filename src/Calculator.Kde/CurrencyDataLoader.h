// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "UnitConverter.h"

#include <QDateTime>
#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

// Supplies currency units and exchange rates to the unit converter engine.
//
// The Windows app's rate service isn't public, so rates come from Frankfurter
// (https://frankfurter.dev), which publishes the European Central Bank's daily
// reference rates. Responses are cached so the converter works offline with the
// last rates fetched.
class CurrencyDataLoader final : public QObject,
                                 public UnitConversionManager::IConverterDataLoader,
                                 public UnitConversionManager::ICurrencyConverterDataLoader
{
    Q_OBJECT

public:
    enum class Status
    {
        NotLoaded,
        Loading,
        Loaded,
        Failed,
    };
    Q_ENUM(Status)

    // 'cacheFile' defaults to currency-rates.json in the user's cache directory.
    explicit CurrencyDataLoader(QString cacheFile = {}, QObject* parent = nullptr);
    ~CurrencyDataLoader() override;

    // When disabled, only cached data is used (for tests).
    void setNetworkEnabled(bool enabled)
    {
        m_networkEnabled = enabled;
    }

    Status status() const
    {
        return m_status;
    }
    bool hasData() const
    {
        return !m_units.empty();
    }
    QDateTime timestamp() const
    {
        return m_timestamp;
    }
    bool isWeekOld() const;

    // Loads rates from the cache file. Returns false if there's no usable cache.
    bool loadFromCache();
    // Fetches fresh rates from the web, regardless of the cache age.
    void refresh();
    // Replaces the data with the given Frankfurter responses (/currencies and /latest).
    bool loadFromJson(const QJsonObject& currencies, const QJsonObject& latest, const QDateTime& timestamp);

    // Number of fraction digits normally shown for a currency (0 for JPY, 2 for USD...).
    static int fractionDigits(const QString& currencyCode);
    QString currencyCode(int unitId) const;

    // IConverterDataLoader
    void LoadData() override;
    std::vector<UnitConversionManager::Category> GetOrderedCategories() override;
    std::vector<UnitConversionManager::Unit> GetOrderedUnits(const UnitConversionManager::Category& category) override;
    std::unordered_map<UnitConversionManager::Unit, UnitConversionManager::ConversionData, UnitConversionManager::UnitHash>
    LoadOrderedRatios(const UnitConversionManager::Unit& unit) override;
    bool SupportsCategory(const UnitConversionManager::Category& target) override;

    // ICurrencyConverterDataLoader
    void SetViewModelCallback(const std::shared_ptr<UnitConversionManager::IViewModelCurrencyCallback>& callback) override;
    std::pair<std::wstring, std::wstring> GetCurrencySymbols(const UnitConversionManager::Unit& unit1, const UnitConversionManager::Unit& unit2) override;
    std::pair<std::wstring, std::wstring>
    GetCurrencyRatioEquality(const UnitConversionManager::Unit& unit1, const UnitConversionManager::Unit& unit2) override;
    std::wstring GetCurrencyTimestamp() override;
    concurrency::task<bool> TryLoadDataFromCacheAsync() override;
    concurrency::task<bool> TryLoadDataFromWebAsync() override;
    concurrency::task<bool> TryLoadDataFromWebOverrideAsync() override;

    // Port of CurrencyDataLoader.RoundCurrencyRatio: keeps at least four significant decimals.
    static double roundCurrencyRatio(double ratio);

Q_SIGNALS:
    // Emitted when rates become available or change.
    void dataChanged();
    void statusChanged();
    // Emitted when fetching rates from the web fails (cached rates, if any, are kept).
    void refreshFailed();

private:
    void setStatus(Status status);
    void onReplyFinished();
    void writeCache(const QJsonObject& currencies, const QJsonObject& latest) const;
    static int unitIdFor(const QString& code);

    QString m_cacheFile;
    bool m_networkEnabled = true;
    Status m_status = Status::NotLoaded;
    QDateTime m_timestamp;

    std::vector<UnitConversionManager::Unit> m_units;
    QHash<int, QString> m_codes;     // unit id -> ISO code
    QHash<int, QString> m_symbols;   // unit id -> symbol
    QHash<int, double> m_perBase;    // unit id -> amount per 1 base currency
    std::shared_ptr<UnitConversionManager::IViewModelCurrencyCallback> m_vmCallback;

    QNetworkAccessManager* m_network = nullptr;
    QPointer<QNetworkReply> m_currenciesReply;
    QPointer<QNetworkReply> m_ratesReply;
};
