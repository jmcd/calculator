// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "UnitConverterController.h"

#include "Command.h"
#include "CurrencyDataLoader.h"
#include "UnitConverterDataLoader.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QLocale>
#include <QSettings>
#include <QVariantMap>

#include <cmath>

using namespace UnitConversionManager;

namespace
{
    QString ToQString(const std::wstring& s)
    {
        return QString::fromStdWString(s);
    }

    class ConverterCallback final : public IUnitConverterVMCallback
    {
    public:
        explicit ConverterCallback(UnitConverterController* controller)
            : m_controller(controller)
        {
        }
        void DisplayCallback(const std::wstring& from, const std::wstring& to) override
        {
            m_controller->onDisplay(from, to);
        }
        void SuggestedValueCallback(const std::vector<std::tuple<std::wstring, Unit>>& values) override
        {
            m_controller->onSuggestedValues(values);
        }
        void MaxDigitsReached() override
        {
            Q_EMIT m_controller->maxDigitsReached();
        }

    private:
        UnitConverterController* m_controller;
    };

    class CurrencyCallback final : public IViewModelCurrencyCallback
    {
    public:
        explicit CurrencyCallback(UnitConverterController* controller)
            : m_controller(controller)
        {
        }
        void CurrencyDataLoadFinished(bool) override
        {
        }
        void CurrencySymbolsCallback(const std::wstring& fromSymbol, const std::wstring& toSymbol) override
        {
            m_controller->onCurrencySymbols(fromSymbol, toSymbol);
        }
        void CurrencyRatiosCallback(const std::wstring& ratioEquality, const std::wstring&) override
        {
            m_controller->onCurrencyRatio(ratioEquality);
        }
        void CurrencyTimestampCallback(const std::wstring&, bool) override
        {
        }
        void NetworkBehaviorChanged(int) override
        {
        }

    private:
        UnitConverterController* m_controller;
    };
}

UnitConverterController::UnitConverterController(QObject* parent)
    : QObject(parent)
{
    init(QLocale::territoryToCode(QLocale().territory()), {}, true);
}

UnitConverterController::UnitConverterController(const QString& region, const QString& currencyCacheFile, bool currencyNetwork, QObject* parent)
    : QObject(parent)
    , m_persist(false)
{
    init(region, currencyCacheFile, currencyNetwork);
}

UnitConverterController::~UnitConverterController() = default;

void UnitConverterController::init(const QString& region, const QString& currencyCacheFile, bool currencyNetwork)
{
    m_unitLoader = std::make_shared<UnitConverterDataLoader>(region);
    m_currencyLoader = std::make_shared<CurrencyDataLoader>(currencyCacheFile);
    m_currencyLoader->setNetworkEnabled(currencyNetwork);
    // Use cached rates straight away; fresh ones are fetched when Currency is opened.
    m_currencyLoader->loadFromCache();

    m_model = std::make_shared<UnitConverter>(m_unitLoader, m_currencyLoader);
    m_model->Initialize();
    m_callback = std::make_shared<ConverterCallback>(this);
    m_currencyCallback = std::make_shared<CurrencyCallback>(this);
    m_model->SetViewModelCallback(m_callback);
    m_model->SetViewModelCurrencyCallback(m_currencyCallback);

    // Rates can arrive while the engine is in the middle of a call; handle them afterwards.
    connect(m_currencyLoader.get(), &CurrencyDataLoader::dataChanged, this, &UnitConverterController::onCurrencyDataChanged, Qt::QueuedConnection);
    connect(m_currencyLoader.get(), &CurrencyDataLoader::statusChanged, this, &UnitConverterController::currencyChanged);
    connect(m_currencyLoader.get(), &CurrencyDataLoader::refreshFailed, this, &UnitConverterController::currencyRefreshFailed);

    m_categories = m_model->GetCategories();
    for (const auto& category : m_categories)
    {
        m_categoryNames.append(ToQString(category.name));
    }

    int initial = 0;
    if (m_persist)
    {
        const int savedId = QSettings().value(QStringLiteral("converter/category"), static_cast<int>(ConverterCategory::Length)).toInt();
        for (size_t i = 0; i < m_categories.size(); ++i)
        {
            if (m_categories[i].id == savedId)
            {
                initial = static_cast<int>(i);
            }
        }
    }
    else
    {
        // Tests start on the first non-currency category, like a first run would.
        initial = 1;
    }
    setCategoryIndex(initial);
}

// ---------------------------------------------------------------------------
// Category and unit selection
// ---------------------------------------------------------------------------

bool UnitConverterController::isCurrency() const
{
    return m_categoryIndex >= 0 && m_categories[static_cast<size_t>(m_categoryIndex)].id == static_cast<int>(ConverterCategory::Currency);
}

bool UnitConverterController::supportsNegative() const
{
    return m_categoryIndex >= 0 && m_categories[static_cast<size_t>(m_categoryIndex)].supportsNegative;
}

QStringList UnitConverterController::units() const
{
    QStringList names;
    for (const auto& unit : m_units)
    {
        names.append(ToQString(unit.name));
    }
    return names;
}

QStringList UnitConverterController::unitAbbreviations() const
{
    QStringList abbreviations;
    for (const auto& unit : m_units)
    {
        abbreviations.append(ToQString(unit.abbreviation));
    }
    return abbreviations;
}

int UnitConverterController::indexOfUnit(const Unit& unit) const
{
    for (size_t i = 0; i < m_units.size(); ++i)
    {
        if (m_units[i].id == unit.id)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void UnitConverterController::setCategoryIndex(int index)
{
    if (index < 0 || static_cast<size_t>(index) >= m_categories.size() || index == m_categoryIndex)
    {
        return;
    }
    m_categoryIndex = index;
    if (m_persist)
    {
        QSettings().setValue(QStringLiteral("converter/category"), m_categories[static_cast<size_t>(index)].id);
    }
    applyCategory();
}

void UnitConverterController::applyCategory()
{
    // Remember what was selected, in case the engine resets it (e.g. after new
    // rates load). Unit IDs are unique across categories, so after a category
    // change these simply won't be found.
    const int previousUnit1 = m_unit1 >= 0 ? m_units[static_cast<size_t>(m_unit1)].id : -1;
    const int previousUnit2 = m_unit2 >= 0 ? m_units[static_cast<size_t>(m_unit2)].id : -1;

    const Category& category = m_categories[static_cast<size_t>(m_categoryIndex)];
    auto [units, from, to] = m_model->SetCurrentCategory(category);
    m_units = units;
    m_value1IsSource = true;
    m_unit1 = indexOfUnit(from);
    m_unit2 = indexOfUnit(to);

    auto findId = [this](int id) {
        for (size_t i = 0; i < m_units.size(); ++i)
        {
            if (m_units[i].id == id)
            {
                return static_cast<int>(i);
            }
        }
        return -1;
    };

    // Prefer the units that were showing, then the ones saved for this category.
    if (const int i1 = findId(previousUnit1), i2 = findId(previousUnit2); i1 >= 0 && i2 >= 0)
    {
        m_unit1 = i1;
        m_unit2 = i2;
    }
    else if (m_persist)
    {
        const QStringList saved = QSettings().value(QStringLiteral("converter/units/%1").arg(category.id)).toString().split(u',');
        if (const int s1 = findId(saved.value(0).toInt()), s2 = findId(saved.value(1).toInt()); saved.size() == 2 && s1 >= 0 && s2 >= 0)
        {
            m_unit1 = s1;
            m_unit2 = s2;
        }
    }

    if (!isCurrency())
    {
        m_symbol1.clear();
        m_symbol2.clear();
        m_ratio.clear();
    }

    Q_EMIT categoryChanged();
    Q_EMIT unitsChanged();
    Q_EMIT activeChanged();
    applyUnits();
    updateCurrencyInfo();
}

void UnitConverterController::setUnit1Index(int index)
{
    if (index < 0 || static_cast<size_t>(index) >= m_units.size() || index == m_unit1)
    {
        return;
    }
    m_unit1 = index;
    applyUnits();
}

void UnitConverterController::setUnit2Index(int index)
{
    if (index < 0 || static_cast<size_t>(index) >= m_units.size() || index == m_unit2)
    {
        return;
    }
    m_unit2 = index;
    applyUnits();
}

void UnitConverterController::applyUnits()
{
    if (m_unit1 >= 0 && m_unit2 >= 0)
    {
        m_model->SetCurrentUnitTypes(m_units[static_cast<size_t>(sourceIndex())], m_units[static_cast<size_t>(targetIndex())]);
        saveSelection();
    }
    Q_EMIT selectedUnitsChanged();
}

void UnitConverterController::saveSelection() const
{
    if (!m_persist || m_unit1 < 0 || m_unit2 < 0)
    {
        return;
    }
    const int categoryId = m_categories[static_cast<size_t>(m_categoryIndex)].id;
    QSettings().setValue(
        QStringLiteral("converter/units/%1").arg(categoryId),
        QStringLiteral("%1,%2").arg(m_units[static_cast<size_t>(m_unit1)].id).arg(m_units[static_cast<size_t>(m_unit2)].id));
}

void UnitConverterController::activate(int field)
{
    const bool wantValue1 = field == 1;
    if (wantValue1 == m_value1IsSource || m_unit1 < 0 || m_unit2 < 0)
    {
        return;
    }
    // The field being activated currently shows the converted value; it becomes
    // the input, and the previous input field now shows the conversion.
    m_value1IsSource = wantValue1;
    m_model->SwitchActive(m_rawTo);
    std::swap(m_rawFrom, m_rawTo);
    m_fromDisplay = formatValue(m_rawFrom, true, sourceIndex());
    m_toDisplay = formatValue(m_rawTo, false, targetIndex());
    Q_EMIT activeChanged();
    Q_EMIT valuesChanged();
    Q_EMIT selectedUnitsChanged();
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

bool UnitConverterController::decimalEnabled() const
{
    if (!isCurrency() || sourceIndex() < 0)
    {
        return true;
    }
    return CurrencyDataLoader::fractionDigits(ToQString(m_units[static_cast<size_t>(sourceIndex())].abbreviation)) > 0;
}

bool UnitConverterController::inputBlocked(int command) const
{
    if (!isCurrency() || sourceIndex() < 0 || m_model->IsSwitchedActive())
    {
        return false;
    }
    // Don't accept more decimals than the currency has (Windows: UpdateInputBlocked).
    const int digits = CurrencyDataLoader::fractionDigits(ToQString(m_units[static_cast<size_t>(sourceIndex())].abbreviation));
    if (command == Decimal)
    {
        return digits == 0;
    }
    if (command >= Zero && command <= Nine)
    {
        const size_t point = m_rawFrom.find(L'.');
        return point != std::wstring::npos && static_cast<int>(m_rawFrom.size() - point - 1) >= digits;
    }
    return false;
}

void UnitConverterController::sendCommand(int command)
{
    if (command < Zero || command > Clear || inputBlocked(command))
    {
        return;
    }
    if (command == Negate && !supportsNegative())
    {
        return;
    }
    m_model->SendCommand(static_cast<Command>(command));
}

bool UnitConverterController::pasteText(const QString& text)
{
    // Port of UnitConverterViewModel.OnPaste: digits and the decimal separator are
    // entered; a leading minus becomes a sign; anything else is skipped.
    const QChar decimal = QLocale().decimalPoint().front();
    bool first = true;
    bool negate = false;
    for (QChar c : text.trimmed())
    {
        int command = -1;
        if (c.isDigit())
        {
            command = Zero + c.digitValue();
        }
        else if (c == decimal || c == u'.')
        {
            command = Decimal;
        }
        else if (c == u'-' || c == u'−')
        {
            if (first)
            {
                negate = true;
                first = false;
                m_model->SendCommand(Command::Clear);
            }
            continue;
        }
        else
        {
            continue;
        }

        if (first)
        {
            m_model->SendCommand(Command::Clear);
            first = false;
        }
        if (inputBlocked(command))
        {
            break;
        }
        m_model->SendCommand(static_cast<Command>(command));
        if (negate && command != Decimal)
        {
            if (supportsNegative())
            {
                m_model->SendCommand(Command::Negate);
            }
            negate = false;
        }
    }

    if (first)
    {
        Q_EMIT pasteFailed();
        return false;
    }
    return true;
}

void UnitConverterController::copyToClipboard()
{
    // Copy the converted value, without group separators.
    QString value = value1Active() ? value2() : value1();
    value.remove(QLocale().groupSeparator());
    QGuiApplication::clipboard()->setText(value);
}

void UnitConverterController::pasteFromClipboard()
{
    pasteText(QGuiApplication::clipboard()->text());
}

bool UnitConverterController::handleKey(int key, int modifiers, const QString& text)
{
    if (modifiers & Qt::AltModifier)
    {
        return false;
    }
    if (modifiers & Qt::ControlModifier)
    {
        if (key == Qt::Key_C)
        {
            copyToClipboard();
            return true;
        }
        if (key == Qt::Key_V)
        {
            pasteFromClipboard();
            return true;
        }
        return false;
    }

    switch (key)
    {
    case Qt::Key_Backspace:
        sendCommand(Backspace);
        return true;
    case Qt::Key_Escape:
    case Qt::Key_Delete:
        sendCommand(Clear);
        return true;
    case Qt::Key_F9:
        sendCommand(Negate);
        return true;
    default:
        break;
    }

    if (text.size() != 1)
    {
        return false;
    }
    const QChar c = text.front();
    if (c.isDigit())
    {
        sendCommand(Zero + c.digitValue());
        return true;
    }
    if (c == QLocale().decimalPoint().front() || c == u'.' || c == u',')
    {
        sendCommand(Decimal);
        return true;
    }
    if (c == u'-')
    {
        sendCommand(Negate);
        return true;
    }
    return false;
}

void UnitConverterController::refreshCurrencyRates()
{
    m_currencyLoader->refresh();
}

// ---------------------------------------------------------------------------
// Display
// ---------------------------------------------------------------------------

QString UnitConverterController::localizeNumber(const QString& raw)
{
    const QLocale locale;
    const QString group = locale.groupSeparator();
    const QString decimal = locale.decimalPoint();

    QString number = raw;
    QString sign;
    if (number.startsWith(u'-'))
    {
        sign = locale.negativeSign();
        number.remove(0, 1);
    }

    QString exponent;
    if (const qsizetype e = number.indexOf(u'e', 0, Qt::CaseInsensitive); e >= 0)
    {
        exponent = number.mid(e);
        number.truncate(e);
    }

    QString integer = number;
    QString fraction;
    bool hasPoint = false;
    if (const qsizetype point = number.indexOf(u'.'); point >= 0)
    {
        hasPoint = true;
        integer = number.left(point);
        fraction = number.mid(point + 1);
    }

    for (qsizetype i = integer.size() - 3; i > 0; i -= 3)
    {
        integer.insert(i, group);
    }

    return sign + integer + (hasPoint ? decimal + fraction : QString()) + exponent;
}

QString UnitConverterController::formatValue(const std::wstring& raw, bool isInput, int unitIndex) const
{
    const QString value = ToQString(raw);
    if (isCurrency() && !isInput && unitIndex >= 0 && !value.contains(u'e', Qt::CaseInsensitive))
    {
        // Converted currency amounts are shown with the currency's usual decimals.
        bool ok = false;
        const double number = value.toDouble(&ok); // QString parsing ignores the C locale.
        if (ok)
        {
            const int digits = CurrencyDataLoader::fractionDigits(ToQString(m_units[static_cast<size_t>(unitIndex)].abbreviation));
            return QLocale().toString(number, 'f', digits);
        }
    }
    return localizeNumber(value);
}

void UnitConverterController::onDisplay(const std::wstring& from, const std::wstring& to)
{
    m_rawFrom = from;
    m_rawTo = to;
    m_fromDisplay = formatValue(from, true, sourceIndex());
    m_toDisplay = formatValue(to, false, targetIndex());
    Q_EMIT valuesChanged();
}

void UnitConverterController::onSuggestedValues(const std::vector<std::tuple<std::wstring, Unit>>& values)
{
    // Like the Windows app: a few nearby units, plus one fun ("whimsical") comparison.
    constexpr int MaxResults = 3;
    QVariantList results;
    QVariantMap whimsical;
    for (const auto& [value, unit] : values)
    {
        QVariantMap entry;
        entry[QStringLiteral("value")] = localizeNumber(ToQString(value));
        entry[QStringLiteral("unit")] = ToQString(unit.abbreviation);
        entry[QStringLiteral("unitName")] = ToQString(unit.name);
        if (unit.isWhimsical)
        {
            if (whimsical.isEmpty())
            {
                whimsical = entry;
            }
        }
        else if (results.size() < MaxResults)
        {
            results.append(entry);
        }
    }
    if (!whimsical.isEmpty())
    {
        results.append(whimsical);
    }
    m_supplementary = results;
    Q_EMIT valuesChanged();
}

void UnitConverterController::onCurrencySymbols(const std::wstring& from, const std::wstring& to)
{
    m_symbol1 = ToQString(m_value1IsSource ? from : to);
    m_symbol2 = ToQString(m_value1IsSource ? to : from);
    Q_EMIT currencyChanged();
}

void UnitConverterController::onCurrencyRatio(const std::wstring& ratio)
{
    m_ratio = ToQString(ratio);
    Q_EMIT currencyChanged();
}

void UnitConverterController::onCurrencyDataChanged()
{
    m_model->ResetCategoriesAndRatios();
    applyCategory();
}

void UnitConverterController::updateCurrencyInfo()
{
    m_timestamp = ToQString(m_currencyLoader->GetCurrencyTimestamp());
    m_ratesOld = m_currencyLoader->isWeekOld();
    Q_EMIT currencyChanged();
}

bool UnitConverterController::currencyLoading() const
{
    return m_currencyLoader->status() == CurrencyDataLoader::Status::Loading;
}

bool UnitConverterController::currencyUnavailable() const
{
    return !m_currencyLoader->hasData() && m_currencyLoader->status() != CurrencyDataLoader::Status::Loading;
}
