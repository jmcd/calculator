// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CalculatorController.h"
#include "CurrencyDataLoader.h"
#include "DateCalculator.h"
#include "ReswStrings.h"
#include "UnitConverterController.h"
#include "UnitConverterDataLoader.h"

#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QTemporaryDir>
#include <QTest>

#include <clocale>

using C = CalculatorController;

class CalculatorTests : public QObject
{
    Q_OBJECT

private:
    static void press(C& calc, std::initializer_list<int> commands)
    {
        for (int command : commands)
        {
            calc.sendCommand(command);
        }
    }

    static void type(C& calc, const QString& keys)
    {
        for (QChar c : keys)
        {
            calc.handleKey(0, Qt::NoModifier, QString(c));
        }
    }

private Q_SLOTS:
    void initTestCase()
    {
        QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
        // Matches main(): the engines parse numbers with the C library.
        std::setlocale(LC_NUMERIC, "C");
    }

    void resourcesLoad()
    {
        QCOMPARE(ReswStrings::instance()->language(), QStringLiteral("en-US"));
        QCOMPARE(ReswStrings::instance()->engineString(QStringLiteral("13")), QStringLiteral("+"));
        QCOMPARE(ReswStrings::instance()->get(QStringLiteral("StandardModeText")), QStringLiteral("Standard"));
    }

    void standardAddition()
    {
        C calc;
        press(calc, { C::One, C::Add, C::Two, C::Equals });
        QCOMPARE(calc.displayValue(), QStringLiteral("3"));
        QCOMPARE(calc.expression(), QStringLiteral("1 + 2 ="));
    }

    void standardIgnoresPrecedence()
    {
        C calc;
        type(calc, QStringLiteral("2+3*4="));
        QCOMPARE(calc.displayValue(), QStringLiteral("20"));
    }

    void scientificRespectsPrecedence()
    {
        C calc;
        calc.setMode(C::Mode::Scientific);
        type(calc, QStringLiteral("2+3*4="));
        QCOMPARE(calc.displayValue(), QStringLiteral("14"));
    }

    void groupsDigits()
    {
        C calc;
        type(calc, QStringLiteral("1234567"));
        QCOMPARE(calc.displayValue(), QStringLiteral("1,234,567"));
    }

    void decimalInput()
    {
        C calc;
        type(calc, QStringLiteral("1.5*4="));
        QCOMPARE(calc.displayValue(), QStringLiteral("6"));
    }

    void divideByZeroIsError()
    {
        C calc;
        type(calc, QStringLiteral("1/0="));
        QVERIFY(calc.isError());
        QCOMPARE(calc.displayValue(), QStringLiteral("Cannot divide by zero"));
        press(calc, { C::Clear });
        QVERIFY(!calc.isError());
        QCOMPARE(calc.displayValue(), QStringLiteral("0"));
    }

    void squareRoot()
    {
        C calc;
        press(calc, { C::Two, C::SquareRoot });
        QVERIFY(calc.displayValue().startsWith(QStringLiteral("1.41421356237")));
    }

    void sineInDegreesAndRadians()
    {
        C calc;
        calc.setMode(C::Mode::Scientific);
        type(calc, QStringLiteral("30"));
        press(calc, { C::Sin });
        QCOMPARE(calc.displayValue(), QStringLiteral("0.5"));

        calc.setAngleMode(C::AngleMode::Radians);
        press(calc, { C::Clear, C::Pi, C::Cos });
        QCOMPARE(calc.displayValue(), QStringLiteral("-1"));
    }

    void historyRecordsAndLoads()
    {
        C calc;
        type(calc, QStringLiteral("5+3="));
        type(calc, QStringLiteral("2*4="));
        const QVariantList history = calc.history();
        QCOMPARE(history.size(), 2);
        // Newest first.
        QCOMPARE(history[0].toMap()[QStringLiteral("result")].toString(), QStringLiteral("8"));
        QCOMPARE(history[0].toMap()[QStringLiteral("expression")].toString(), QStringLiteral("2 × 4 ="));

        // Load "5 + 3 = 8" and continue calculating from it.
        calc.historyLoad(history[1].toMap()[QStringLiteral("index")].toInt());
        QCOMPARE(calc.displayValue(), QStringLiteral("8"));
        QCOMPARE(calc.expression(), QStringLiteral("5 + 3 ="));
        type(calc, QStringLiteral("+2="));
        QCOMPARE(calc.displayValue(), QStringLiteral("10"));

        calc.historyClear();
        QVERIFY(calc.history().isEmpty());
    }

    void memory()
    {
        C calc;
        type(calc, QStringLiteral("7"));
        calc.memoryStore();
        QCOMPARE(calc.memory(), QStringList{ QStringLiteral("7") });
        press(calc, { C::Clear });
        type(calc, QStringLiteral("3"));
        calc.memoryAdd();
        QCOMPARE(calc.memory(), QStringList{ QStringLiteral("10") });
        calc.memorySubtract();
        calc.memorySubtract();
        QCOMPARE(calc.memory(), QStringList{ QStringLiteral("4") });
        press(calc, { C::Clear });
        calc.memoryRecall();
        QCOMPARE(calc.displayValue(), QStringLiteral("4"));
        calc.memoryClearAll();
        QVERIFY(calc.memory().isEmpty());
    }

    void programmerRadixes()
    {
        C calc;
        calc.setMode(C::Mode::Programmer);
        type(calc, QStringLiteral("255"));
        QCOMPARE(calc.hexValue(), QStringLiteral("FF"));
        QCOMPARE(calc.decValue(), QStringLiteral("255"));
        QCOMPARE(calc.octValue(), QStringLiteral("377"));
        QCOMPARE(calc.binValue(), QStringLiteral("1111 1111"));
        QVERIFY(calc.bits()[7].toBool());
        QVERIFY(!calc.bits()[8].toBool());

        calc.toggleBit(8);
        QCOMPARE(calc.decValue(), QStringLiteral("511"));

        calc.setRadix(16);
        press(calc, { C::Clear });
        type(calc, QStringLiteral("1f"));
        QCOMPARE(calc.decValue(), QStringLiteral("31"));
        QCOMPARE(calc.binValue(), QStringLiteral("0001 1111"));
    }

    void programmerWordSizeAndBitwise()
    {
        C calc;
        calc.setMode(C::Mode::Programmer);
        // BYTE is signed: 127 + 1 wraps around.
        calc.setWordSize(8);
        type(calc, QStringLiteral("127+1="));
        QCOMPARE(calc.decValue(), QStringLiteral("-128"));

        press(calc, { C::Clear });
        type(calc, QStringLiteral("12&10="));
        QCOMPARE(calc.decValue(), QStringLiteral("8"));

        press(calc, { C::Clear });
        type(calc, QStringLiteral("1<4="));
        QCOMPARE(calc.decValue(), QStringLiteral("16"));
    }

    void paste_data()
    {
        QTest::addColumn<int>("mode");
        QTest::addColumn<QString>("text");
        QTest::addColumn<bool>("ok");
        QTest::addColumn<QString>("result");

        QTest::newRow("number") << 0 << QStringLiteral("1,234.5") << true << QStringLiteral("1,234.5");
        QTest::newRow("negative") << 0 << QStringLiteral("-42") << true << QStringLiteral("-42");
        QTest::newRow("expression") << 0 << QStringLiteral("12.5 * 2 =") << true << QStringLiteral("25");
        QTest::newRow("sci-parens") << 1 << QStringLiteral("(1+2)*3=") << true << QStringLiteral("9");
        QTest::newRow("sci-notation") << 1 << QStringLiteral("1.5e3=") << true << QStringLiteral("1,500");
        QTest::newRow("garbage") << 0 << QStringLiteral("hello") << false << QStringLiteral("0");
        QTest::newRow("std-parens") << 0 << QStringLiteral("(1+2)") << false << QStringLiteral("0");
    }

    void paste()
    {
        QFETCH(int, mode);
        QFETCH(QString, text);
        QFETCH(bool, ok);
        QFETCH(QString, result);

        C calc;
        calc.setMode(static_cast<C::Mode>(mode));
        QCOMPARE(calc.pasteText(text), ok);
        QCOMPARE(calc.displayValue(), result);
    }

    void programmerPasteHex()
    {
        C calc;
        calc.setMode(C::Mode::Programmer);
        calc.setRadix(16);
        QVERIFY(calc.pasteText(QStringLiteral("0xFF")));
        QCOMPARE(calc.decValue(), QStringLiteral("255"));
        calc.setRadix(10);
        QVERIFY(!calc.pasteText(QStringLiteral("FF")));
    }

    void dateDifference()
    {
        auto diff = DateCalculator::difference(QDate(2024, 1, 1), QDate(2025, 3, 15));
        QCOMPARE(diff.years, 1);
        QCOMPARE(diff.months, 2);
        QCOMPARE(diff.weeks, 2);
        QCOMPARE(diff.days, 0);
        QCOMPARE(diff.totalDays, 439);

        DateCalculator calc;
        QCOMPARE(calc.differenceBetween(2025, 3, 15, 2024, 1, 1)[QStringLiteral("text")].toString(), QStringLiteral("1 year, 2 months, 2 weeks"));
        QCOMPARE(calc.differenceBetween(2025, 3, 15, 2025, 3, 15)[QStringLiteral("text")].toString(), QStringLiteral("Same dates"));
        QCOMPARE(calc.differenceBetween(2025, 3, 1, 2025, 3, 4)[QStringLiteral("text")].toString(), QStringLiteral("3 days"));
    }

    void dateAdjust()
    {
        DateCalculator calc;
        auto r = calc.adjust(2024, 1, 31, false, 0, 1, 0);
        QVERIFY(r[QStringLiteral("valid")].toBool());
        QCOMPARE(r[QStringLiteral("month")].toInt(), 2);
        QCOMPARE(r[QStringLiteral("day")].toInt(), 29);

        r = calc.adjust(2024, 3, 1, true, 1, 0, 1);
        QCOMPARE(r[QStringLiteral("year")].toInt(), 2023);
        QCOMPARE(r[QStringLiteral("month")].toInt(), 2);
        QCOMPARE(r[QStringLiteral("day")].toInt(), 28);

        r = calc.adjust(9999, 12, 1, false, 1, 0, 0);
        QVERIFY(!r[QStringLiteral("valid")].toBool());
    }

    // -----------------------------------------------------------------------
    // Unit converter
    // -----------------------------------------------------------------------

    static int categoryIndex(const UnitConverterController& converter, ConverterCategory category)
    {
        return converter.categories().indexOf(UnitConverterDataLoader::categoryName(category));
    }

    static void typeConverter(UnitConverterController& converter, const QString& keys)
    {
        for (QChar c : keys)
        {
            converter.handleKey(0, Qt::NoModifier, QString(c));
        }
    }

    void converterCategories()
    {
        UnitConverterController converter(QStringLiteral("US"), QStringLiteral("/nonexistent/rates.json"), false);
        QCOMPARE(converter.categories().size(), 13);
        QCOMPARE(converter.categories().first(), QStringLiteral("Currency"));
        QVERIFY(converter.categories().contains(QStringLiteral("Temperature")));
    }

    void converterLength()
    {
        UnitConverterController converter(QStringLiteral("US"), QStringLiteral("/nonexistent/rates.json"), false);
        converter.setCategoryIndex(categoryIndex(converter, ConverterCategory::Length));
        // US defaults: centimeters to inches.
        QCOMPARE(converter.units()[converter.unit1Index()], QStringLiteral("Centimeters"));
        QCOMPARE(converter.units()[converter.unit2Index()], QStringLiteral("Inches"));

        typeConverter(converter, QStringLiteral("254"));
        QCOMPARE(converter.value1(), QStringLiteral("254"));
        QCOMPARE(converter.value2(), QStringLiteral("100"));
        QVERIFY(!converter.supplementaryResults().isEmpty());

        // Grouping and decimals.
        typeConverter(converter, QStringLiteral("0000.5"));
        QCOMPARE(converter.value1(), QStringLiteral("2,540,000.5"));

        // Make the second field the input and convert back.
        converter.activate(2);
        QVERIFY(!converter.value1Active());
        converter.sendCommand(UnitConverterController::Clear);
        typeConverter(converter, QStringLiteral("1"));
        QCOMPARE(converter.value2(), QStringLiteral("1"));
        QCOMPARE(converter.value1(), QStringLiteral("2.54"));
    }

    void converterTemperatureRegions()
    {
        UnitConverterController us(QStringLiteral("US"), QStringLiteral("/nonexistent/rates.json"), false);
        us.setCategoryIndex(categoryIndex(us, ConverterCategory::Temperature));
        QCOMPARE(us.units()[us.unit1Index()], QStringLiteral("Celsius"));
        QCOMPARE(us.units()[us.unit2Index()], QStringLiteral("Fahrenheit"));
        typeConverter(us, QStringLiteral("100"));
        QCOMPARE(us.value2(), QStringLiteral("212"));

        // Negative input: -40 is the same in both scales.
        us.sendCommand(UnitConverterController::Clear);
        typeConverter(us, QStringLiteral("40-"));
        QCOMPARE(us.value1(), QStringLiteral("-40"));
        QCOMPARE(us.value2(), QStringLiteral("-40"));

        UnitConverterController gb(QStringLiteral("GB"), QStringLiteral("/nonexistent/rates.json"), false);
        gb.setCategoryIndex(categoryIndex(gb, ConverterCategory::Temperature));
        QCOMPARE(gb.units()[gb.unit1Index()], QStringLiteral("Fahrenheit"));
        QCOMPARE(gb.units()[gb.unit2Index()], QStringLiteral("Celsius"));
    }

    void converterNegativeOnlyWhereSupported()
    {
        UnitConverterController converter(QStringLiteral("US"), QStringLiteral("/nonexistent/rates.json"), false);
        converter.setCategoryIndex(categoryIndex(converter, ConverterCategory::Length));
        QVERIFY(!converter.supportsNegative());
        typeConverter(converter, QStringLiteral("5-"));
        QCOMPARE(converter.value1(), QStringLiteral("5"));
    }

    void converterRegionalUnits()
    {
        UnitConverterController jp(QStringLiteral("JP"), QStringLiteral("/nonexistent/rates.json"), false);
        jp.setCategoryIndex(categoryIndex(jp, ConverterCategory::Area));
        QVERIFY(jp.units().contains(QStringLiteral("Pyeong")));

        UnitConverterController us(QStringLiteral("US"), QStringLiteral("/nonexistent/rates.json"), false);
        us.setCategoryIndex(categoryIndex(us, ConverterCategory::Area));
        QVERIFY(!us.units().contains(QStringLiteral("Pyeong")));
    }

    void converterDataUnits()
    {
        UnitConverterController converter(QStringLiteral("US"), QStringLiteral("/nonexistent/rates.json"), false);
        converter.setCategoryIndex(categoryIndex(converter, ConverterCategory::Data));
        converter.setUnit1Index(converter.units().indexOf(QStringLiteral("Kibibytes")));
        converter.setUnit2Index(converter.units().indexOf(QStringLiteral("Bytes")));
        typeConverter(converter, QStringLiteral("2"));
        QCOMPARE(converter.value2(), QStringLiteral("2,048"));
    }

    void converterPaste()
    {
        UnitConverterController converter(QStringLiteral("US"), QStringLiteral("/nonexistent/rates.json"), false);
        converter.setCategoryIndex(categoryIndex(converter, ConverterCategory::Length));
        QVERIFY(converter.pasteText(QStringLiteral("1,234.5")));
        QCOMPARE(converter.value1(), QStringLiteral("1,234.5"));
        QVERIFY(!converter.pasteText(QStringLiteral("abc")));
    }

    void localizeNumber()
    {
        QCOMPARE(UnitConverterController::localizeNumber(QStringLiteral("1234567.25")), QStringLiteral("1,234,567.25"));
        QCOMPARE(UnitConverterController::localizeNumber(QStringLiteral("-1000")), QStringLiteral("-1,000"));
        QCOMPARE(UnitConverterController::localizeNumber(QStringLiteral("12.")), QStringLiteral("12."));
        QCOMPARE(UnitConverterController::localizeNumber(QStringLiteral("1.5e+20")), QStringLiteral("1.5e+20"));
        QCOMPARE(UnitConverterController::localizeNumber(QStringLiteral("999")), QStringLiteral("999"));
    }

    void currencyWithoutRates()
    {
        UnitConverterController converter(QStringLiteral("US"), QStringLiteral("/nonexistent/rates.json"), false);
        converter.setCategoryIndex(categoryIndex(converter, ConverterCategory::Currency));
        QVERIFY(converter.isCurrency());
        QVERIFY(converter.units().isEmpty());
        QVERIFY(converter.currencyUnavailable());
    }

    void currencyFromCache()
    {
        QTemporaryDir dir;
        const QString cache = dir.filePath(QStringLiteral("rates.json"));
        {
            QJsonObject currencies{ { QStringLiteral("USD"), QStringLiteral("US Dollar") },
                                    { QStringLiteral("EUR"), QStringLiteral("Euro") },
                                    { QStringLiteral("JPY"), QStringLiteral("Japanese Yen") } };
            QJsonObject latest{ { QStringLiteral("base"), QStringLiteral("USD") },
                                { QStringLiteral("rates"), QJsonObject{ { QStringLiteral("EUR"), 0.89381 }, { QStringLiteral("JPY"), 158.12 } } } };
            QJsonObject root{ { QStringLiteral("timestamp"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate) },
                              { QStringLiteral("currencies"), currencies },
                              { QStringLiteral("latest"), latest } };
            QFile file(cache);
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write(QJsonDocument(root).toJson());
        }

        UnitConverterController converter(QStringLiteral("US"), cache, false);
        converter.setCategoryIndex(categoryIndex(converter, ConverterCategory::Currency));
        QCOMPARE(converter.units().size(), 3);
        // en-US defaults: US dollars to euros.
        QCOMPARE(converter.unitAbbreviations()[converter.unit1Index()], QStringLiteral("USD"));
        QCOMPARE(converter.unitAbbreviations()[converter.unit2Index()], QStringLiteral("EUR"));
        QCOMPARE(converter.currencySymbol1(), QStringLiteral("$"));
        QCOMPARE(converter.currencySymbol2(), QStringLiteral("€"));
        QCOMPARE(converter.currencyRatio(), QStringLiteral("1 USD = 0.8938 EUR"));
        QVERIFY(converter.currencyTimestamp().startsWith(QStringLiteral("Updated ")));
        QVERIFY(!converter.currencyRatesOld());

        typeConverter(converter, QStringLiteral("100"));
        QCOMPARE(converter.value2(), QStringLiteral("89.38"));

        // At most two decimals can be typed for dollars.
        typeConverter(converter, QStringLiteral(".999"));
        QCOMPARE(converter.value1(), QStringLiteral("100.99"));

        // Yen has no minor unit: no decimals in input or output.
        converter.setUnit2Index(converter.unitAbbreviations().indexOf(QStringLiteral("JPY")));
        QCOMPARE(converter.value2(), QStringLiteral("15,969")); // 100.99 × 158.12
        converter.activate(2);
        QVERIFY(!converter.decimalEnabled());
    }

    void currencyRatioRounding()
    {
        QCOMPARE(CurrencyDataLoader::roundCurrencyRatio(0.89381), 0.8938);
        QCOMPARE(CurrencyDataLoader::roundCurrencyRatio(0.000212), 0.000212);
        QCOMPARE(CurrencyDataLoader::roundCurrencyRatio(158.123456), 158.1235);
    }
};

QTEST_MAIN(CalculatorTests)
#include "tst_calculator.moc"
