// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CurrencyDataLoader.h"
#include "ReswStrings.h"
#include "UnitConverterDataLoader.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QStandardPaths>

#include <algorithm>
#include <cmath>

using namespace UnitConversionManager;

namespace
{
    const QString CurrenciesUrl = QStringLiteral("https://api.frankfurter.dev/v1/currencies");
    const QString LatestUrl = QStringLiteral("https://api.frankfurter.dev/v1/latest?base=USD");

    constexpr qint64 DaySeconds = 60 * 60 * 24;
    constexpr int RateMinDecimals = 4;
    constexpr int RateMinSignificantDecimals = 4;
    constexpr int RateFractionPadding = 2;

    // ISO code -> symbol, built from Qt's locale data. When several locales use a
    // currency, prefer the one whose territory matches the code's first two
    // letters (USD -> US, GBP -> GB), which by ISO 4217 convention is its home.
    const QHash<QString, QString>& CurrencySymbols()
    {
        static const QHash<QString, QString> symbols = [] {
            QHash<QString, QString> result;
            QHash<QString, bool> isHome;
            const auto locales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyTerritory);
            for (const QLocale& locale : locales)
            {
                const QString code = locale.currencySymbol(QLocale::CurrencyIsoCode);
                const QString symbol = locale.currencySymbol(QLocale::CurrencySymbol);
                if (code.size() != 3 || symbol.isEmpty())
                {
                    continue;
                }
                const bool home = QLocale::territoryToCode(locale.territory()) == code.left(2);
                if (!result.contains(code) || (home && !isHome.value(code)))
                {
                    result.insert(code, symbol);
                    isHome.insert(code, home);
                }
            }
            return result;
        }();
        return symbols;
    }

    // Default "from" and "to" currencies for the user's locale, from the same
    // table the Windows app uses.
    std::pair<QString, QString> DefaultFromTo()
    {
        QFile file(QStringLiteral(":/converter/DefaultFromToCurrency.json"));
        QJsonObject table;
        if (file.open(QIODevice::ReadOnly))
        {
            table = QJsonDocument::fromJson(file.readAll()).object();
        }
        QString localeName = QLocale().name();
        localeName.replace(u'_', u'-');
        QJsonObject entry = table.value(localeName).toObject();
        if (entry.isEmpty())
        {
            entry = table.value(QStringLiteral("default")).toObject();
        }
        return { entry.value(QStringLiteral("from")).toString(QStringLiteral("USD")), entry.value(QStringLiteral("to")).toString(QStringLiteral("EUR")) };
    }
}

CurrencyDataLoader::CurrencyDataLoader(QString cacheFile, QObject* parent)
    : QObject(parent)
    , m_cacheFile(std::move(cacheFile))
{
    if (m_cacheFile.isEmpty())
    {
        m_cacheFile = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/currency-rates.json");
    }
}

CurrencyDataLoader::~CurrencyDataLoader() = default;

int CurrencyDataLoader::unitIdFor(const QString& code)
{
    // Stable IDs so saved preferences survive changes to the currency list.
    // Kept clear of the IDs used by the other unit categories.
    int id = 0;
    for (QChar c : code.toUpper().left(3))
    {
        id = id * 26 + std::clamp(c.unicode() - u'A', 0, 25);
    }
    return 1000 + id;
}

int CurrencyDataLoader::fractionDigits(const QString& currencyCode)
{
    // ISO 4217 currencies without minor units.
    static const QStringList zeroDigit = { QStringLiteral("BIF"), QStringLiteral("CLP"), QStringLiteral("DJF"), QStringLiteral("GNF"),
                                           QStringLiteral("ISK"), QStringLiteral("JPY"), QStringLiteral("KMF"), QStringLiteral("KRW"),
                                           QStringLiteral("PYG"), QStringLiteral("RWF"), QStringLiteral("UGX"), QStringLiteral("VND"),
                                           QStringLiteral("VUV"), QStringLiteral("XAF"), QStringLiteral("XOF"), QStringLiteral("XPF") };
    return zeroDigit.contains(currencyCode) ? 0 : 2;
}

QString CurrencyDataLoader::currencyCode(int unitId) const
{
    return m_codes.value(unitId);
}

bool CurrencyDataLoader::isWeekOld() const
{
    return m_timestamp.isValid() && m_timestamp.secsTo(QDateTime::currentDateTimeUtc()) > 7 * DaySeconds;
}

void CurrencyDataLoader::setStatus(Status status)
{
    if (m_status != status)
    {
        m_status = status;
        Q_EMIT statusChanged();
    }
}

bool CurrencyDataLoader::loadFromJson(const QJsonObject& currencies, const QJsonObject& latest, const QDateTime& timestamp)
{
    const QString base = latest.value(QStringLiteral("base")).toString();
    const QJsonObject rates = latest.value(QStringLiteral("rates")).toObject();
    if (base.isEmpty() || rates.isEmpty() || currencies.isEmpty())
    {
        return false;
    }

    QHash<QString, double> perBase;
    perBase.insert(base, 1.0);
    for (auto it = rates.begin(); it != rates.end(); ++it)
    {
        const double rate = it.value().toDouble();
        if (rate > 0)
        {
            perBase.insert(it.key(), rate);
        }
    }

    const auto [defaultFrom, defaultTo] = DefaultFromTo();
    const QString from = perBase.contains(defaultFrom) ? defaultFrom : QStringLiteral("USD");
    const QString to = perBase.contains(defaultTo) && defaultTo != from ? defaultTo : (from == u"EUR" ? QStringLiteral("USD") : QStringLiteral("EUR"));

    struct Entry
    {
        QString code;
        QString name;
    };
    std::vector<Entry> entries;
    for (auto it = perBase.begin(); it != perBase.end(); ++it)
    {
        const QString name = currencies.value(it.key()).toString();
        if (!name.isEmpty())
        {
            entries.push_back({ it.key(), name });
        }
    }
    if (entries.size() < 2)
    {
        return false;
    }
    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return QString::localeAwareCompare(a.name, b.name) < 0; });

    m_units.clear();
    m_codes.clear();
    m_symbols.clear();
    m_perBase.clear();
    for (const Entry& entry : entries)
    {
        const int id = unitIdFor(entry.code);
        m_units.emplace_back(id, entry.name.toStdWString(), entry.code.toStdWString(), entry.code == from, entry.code == to, false);
        m_codes.insert(id, entry.code);
        m_symbols.insert(id, CurrencySymbols().value(entry.code, entry.code));
        m_perBase.insert(id, perBase.value(entry.code));
    }

    m_timestamp = timestamp;
    setStatus(Status::Loaded);
    Q_EMIT dataChanged();
    return true;
}

bool CurrencyDataLoader::loadFromCache()
{
    QFile file(m_cacheFile);
    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }
    const QJsonObject cache = QJsonDocument::fromJson(file.readAll()).object();
    const QDateTime timestamp = QDateTime::fromString(cache.value(QStringLiteral("timestamp")).toString(), Qt::ISODate);
    return loadFromJson(cache.value(QStringLiteral("currencies")).toObject(), cache.value(QStringLiteral("latest")).toObject(), timestamp);
}

void CurrencyDataLoader::writeCache(const QJsonObject& currencies, const QJsonObject& latest) const
{
    QDir().mkpath(QFileInfo(m_cacheFile).absolutePath());
    QSaveFile file(m_cacheFile);
    if (!file.open(QIODevice::WriteOnly))
    {
        return;
    }
    QJsonObject cache;
    cache.insert(QStringLiteral("timestamp"), m_timestamp.toString(Qt::ISODate));
    cache.insert(QStringLiteral("source"), QStringLiteral("https://frankfurter.dev"));
    cache.insert(QStringLiteral("currencies"), currencies);
    cache.insert(QStringLiteral("latest"), latest);
    file.write(QJsonDocument(cache).toJson(QJsonDocument::Compact));
    file.commit();
}

void CurrencyDataLoader::refresh()
{
    if (!m_networkEnabled || m_status == Status::Loading)
    {
        return;
    }
    if (!m_network)
    {
        m_network = new QNetworkAccessManager(this);
        m_network->setTransferTimeout(15000);
    }

    auto get = [this](const QString& url) {
        QNetworkRequest request{ QUrl(url) };
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("calculator-kde"));
        QNetworkReply* reply = m_network->get(request);
        connect(reply, &QNetworkReply::finished, this, &CurrencyDataLoader::onReplyFinished);
        return reply;
    };

    setStatus(Status::Loading);
    m_currenciesReply = get(CurrenciesUrl);
    m_ratesReply = get(LatestUrl);
}

void CurrencyDataLoader::onReplyFinished()
{
    if (!m_currenciesReply || !m_ratesReply || !m_currenciesReply->isFinished() || !m_ratesReply->isFinished())
    {
        return;
    }

    QNetworkReply* currenciesReply = m_currenciesReply;
    QNetworkReply* ratesReply = m_ratesReply;
    m_currenciesReply = nullptr;
    m_ratesReply = nullptr;
    currenciesReply->deleteLater();
    ratesReply->deleteLater();

    bool ok = currenciesReply->error() == QNetworkReply::NoError && ratesReply->error() == QNetworkReply::NoError;
    if (ok)
    {
        const QJsonObject currencies = QJsonDocument::fromJson(currenciesReply->readAll()).object();
        const QJsonObject latest = QJsonDocument::fromJson(ratesReply->readAll()).object();
        ok = loadFromJson(currencies, latest, QDateTime::currentDateTimeUtc());
        if (ok)
        {
            writeCache(currencies, latest);
        }
    }

    if (!ok)
    {
        // Keep any cached rates we already have.
        setStatus(hasData() ? Status::Loaded : Status::Failed);
        Q_EMIT refreshFailed();
    }
}

// ---------------------------------------------------------------------------
// IConverterDataLoader
// ---------------------------------------------------------------------------

void CurrencyDataLoader::LoadData()
{
    if (!hasData())
    {
        loadFromCache();
    }

    const bool stale = !m_timestamp.isValid() || m_timestamp.secsTo(QDateTime::currentDateTimeUtc()) > DaySeconds;
    if ((!hasData() || stale) && m_networkEnabled)
    {
        refresh();
    }
    else if (!hasData())
    {
        setStatus(Status::Failed);
    }
}

std::vector<Category> CurrencyDataLoader::GetOrderedCategories()
{
    return { Category(static_cast<int>(ConverterCategory::Currency), UnitConverterDataLoader::categoryName(ConverterCategory::Currency).toStdWString(), false) };
}

std::vector<Unit> CurrencyDataLoader::GetOrderedUnits(const Category& category)
{
    return SupportsCategory(category) ? m_units : std::vector<Unit>{};
}

std::unordered_map<Unit, ConversionData, UnitHash> CurrencyDataLoader::LoadOrderedRatios(const Unit& unit)
{
    std::unordered_map<Unit, ConversionData, UnitHash> ratios;
    const double source = m_perBase.value(unit.id);
    if (source <= 0)
    {
        return ratios;
    }
    for (const Unit& target : m_units)
    {
        ratios[target] = ConversionData(m_perBase.value(target.id) / source, 0, false);
    }
    return ratios;
}

bool CurrencyDataLoader::SupportsCategory(const Category& target)
{
    return target.id == static_cast<int>(ConverterCategory::Currency);
}

// ---------------------------------------------------------------------------
// ICurrencyConverterDataLoader
// ---------------------------------------------------------------------------

void CurrencyDataLoader::SetViewModelCallback(const std::shared_ptr<IViewModelCurrencyCallback>& callback)
{
    m_vmCallback = callback;
}

std::pair<std::wstring, std::wstring> CurrencyDataLoader::GetCurrencySymbols(const Unit& unit1, const Unit& unit2)
{
    if (!m_symbols.contains(unit1.id) || !m_symbols.contains(unit2.id))
    {
        return {};
    }
    return { m_symbols.value(unit1.id).toStdWString(), m_symbols.value(unit2.id).toStdWString() };
}

double CurrencyDataLoader::roundCurrencyRatio(double ratio)
{
    // Keep at least two meaningful digits: 0.00000000342334 -> 0.000000003423, 0.000212 -> 0.000212
    int decimals = RateMinDecimals;
    if (ratio < 1)
    {
        decimals = std::max(RateMinDecimals, static_cast<int>(-std::log10(ratio)) + RateMinSignificantDecimals);
    }
    const double scale = std::pow(10.0, decimals);
    return std::round(ratio * scale) / scale;
}

std::pair<std::wstring, std::wstring> CurrencyDataLoader::GetCurrencyRatioEquality(const Unit& unit1, const Unit& unit2)
{
    const double source = m_perBase.value(unit1.id);
    const double target = m_perBase.value(unit2.id);
    if (source <= 0 || target <= 0)
    {
        return {};
    }

    const double rounded = roundCurrencyRatio(target / source);
    // Show the rounded value with at least two decimals and no extra trailing zeros.
    QString number = QLocale().toString(rounded, 'f', 15);
    const QChar decimal = QLocale().decimalPoint().front();
    if (const qsizetype point = number.indexOf(decimal); point >= 0)
    {
        while (number.size() > point + 1 + RateFractionPadding && number.endsWith(u'0'))
        {
            number.chop(1);
        }
    }

    const QString format = ReswStrings::instance()->get(QStringLiteral("CurrencyFromToRatioFormat"), QStringLiteral("%1 %2 = %3 %4"));
    const QString one = QLocale().toString(1);
    const QString text = format.arg(one, m_codes.value(unit1.id), number, m_codes.value(unit2.id));
    const QString accessible = format.arg(one, QString::fromStdWString(unit1.name), number, QString::fromStdWString(unit2.name));
    return { text.toStdWString(), accessible.toStdWString() };
}

std::wstring CurrencyDataLoader::GetCurrencyTimestamp()
{
    if (!m_timestamp.isValid())
    {
        return {};
    }
    const QDateTime local = m_timestamp.toLocalTime();
    const QString format = ReswStrings::instance()->get(QStringLiteral("CurrencyTimestampFormat"), QStringLiteral("Updated %1 %2"));
    return format.arg(QLocale().toString(local.date(), QLocale::ShortFormat), QLocale().toString(local.time(), QLocale::ShortFormat)).toStdWString();
}

concurrency::task<bool> CurrencyDataLoader::TryLoadDataFromCacheAsync()
{
    return concurrency::task_from_result(loadFromCache());
}

concurrency::task<bool> CurrencyDataLoader::TryLoadDataFromWebAsync()
{
    // Network loads complete asynchronously; listen for dataChanged().
    refresh();
    return concurrency::task_from_result(false);
}

concurrency::task<bool> CurrencyDataLoader::TryLoadDataFromWebOverrideAsync()
{
    refresh();
    return concurrency::task_from_result(false);
}
