// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CalculatorController.h"
#include "DateCalculator.h"
#include "ReswStrings.h"

#include <QGuiApplication>
#include <QLocale>
#include <QTest>

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
};

QTEST_MAIN(CalculatorTests)
#include "tst_calculator.moc"
