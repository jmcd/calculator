// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "CalculatorManager.h"
#include "EngineResourceProvider.h"

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <memory>

// Bridges the calculation engine (CalculationManager::CalculatorManager) to QML.
// This plays the role of StandardCalculatorViewModel in the Windows app.
class CalculatorController : public QObject, public ICalcDisplay
{
    Q_OBJECT
    QML_NAMED_ELEMENT(CalculatorEngine)
    QML_SINGLETON

    Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(QString displayValue READ displayValue NOTIFY displayChanged)
    Q_PROPERTY(QString expression READ expression NOTIFY expressionChanged)
    Q_PROPERTY(bool isError READ isError NOTIFY displayChanged)
    Q_PROPERTY(int parenthesisCount READ parenthesisCount NOTIFY parenthesisCountChanged)
    Q_PROPERTY(AngleMode angleMode READ angleMode WRITE setAngleMode NOTIFY angleModeChanged)
    Q_PROPERTY(bool exponentialFormat READ exponentialFormat WRITE setExponentialFormat NOTIFY exponentialFormatChanged)
    Q_PROPERTY(int radix READ radix WRITE setRadix NOTIFY radixChanged)
    Q_PROPERTY(int wordSize READ wordSize WRITE setWordSize NOTIFY wordSizeChanged)
    Q_PROPERTY(QString hexValue READ hexValue NOTIFY programmerValuesChanged)
    Q_PROPERTY(QString decValue READ decValue NOTIFY programmerValuesChanged)
    Q_PROPERTY(QString octValue READ octValue NOTIFY programmerValuesChanged)
    Q_PROPERTY(QString binValue READ binValue NOTIFY programmerValuesChanged)
    Q_PROPERTY(QVariantList bits READ bits NOTIFY programmerValuesChanged)
    Q_PROPERTY(QStringList memory READ memory NOTIFY memoryChanged)
    Q_PROPERTY(QVariantList history READ history NOTIFY historyChanged)
    Q_PROPERTY(QString decimalSeparator READ decimalSeparator CONSTANT)

public:
    enum class Mode
    {
        Standard = 0,
        Scientific = 1,
        Programmer = 2,
    };
    Q_ENUM(Mode)

    enum class AngleMode
    {
        Degrees = 0,
        Radians = 1,
        Gradians = 2,
    };
    Q_ENUM(AngleMode)

    // Mirrors CalculationManager::Command so QML can refer to engine commands by name.
    enum Op
    {
        Zero = 130, One, Two, Three, Four, Five, Six, Seven, Eight, Nine,
        HexA = 140, HexB, HexC, HexD, HexE, HexF,
        Decimal = 84,
        Negate = 80,
        Clear = 81,
        ClearEntry = 82,
        Backspace = 83,
        Add = 93,
        Subtract = 94,
        Multiply = 92,
        Divide = 91,
        Equals = 121,
        Percent = 118,
        Mod = 95,
        Power = 97,
        Root = 96,
        OpenParen = 128,
        CloseParen = 129,
        Reciprocal = 114,
        Square = 111,
        Cube = 112,
        SquareRoot = 110,
        CubeRoot = 116,
        PowerOf10 = 117,
        PowerOf2 = 412,
        PowerOfE = 205,
        Log = 109,
        Ln = 108,
        LogBaseY = 500,
        Factorial = 113,
        Pi = 120,
        Euler = 601,
        Exp = 127,
        FE = 119,
        Abs = 413,
        Floor = 414,
        Ceil = 415,
        Rand = 600,
        Dms = 115,
        ToDegrees = 324,
        Sin = 102, Cos = 103, Tan = 104,
        Sinh = 105, Cosh = 106, Tanh = 107,
        Asin = 202, Acos = 203, Atan = 204,
        Asinh = 206, Acosh = 207, Atanh = 208,
        Sec = 400, Asec = 401, Csc = 402, Acsc = 403, Cot = 404, Acot = 405,
        Sech = 406, Asech = 407, Csch = 408, Acsch = 409, Coth = 410, Acoth = 411,
        And = 86,
        Or = 87,
        Xor = 88,
        Not = 101,
        Nand = 501,
        Nor = 502,
        Lsh = 89,
        Rsh = 90,
        RshLogical = 505,
        Rol = 99,
        Ror = 100,
        RolCarry = 416,
        RorCarry = 417,
    };
    Q_ENUM(Op)

    explicit CalculatorController(QObject* parent = nullptr);
    ~CalculatorController() override;

    Mode mode() const { return m_mode; }
    void setMode(Mode mode);
    QString displayValue() const { return m_displayValue; }
    QString expression() const { return m_expression; }
    bool isError() const { return m_isError; }
    int parenthesisCount() const { return m_parenthesisCount; }
    AngleMode angleMode() const { return m_angleMode; }
    void setAngleMode(AngleMode mode);
    bool exponentialFormat() const { return m_exponentialFormat; }
    void setExponentialFormat(bool enabled);
    int radix() const { return m_radix; }
    void setRadix(int radix);
    int wordSize() const { return m_wordSize; }
    void setWordSize(int bits);
    QString hexValue() const { return m_hex; }
    QString decValue() const { return m_dec; }
    QString octValue() const { return m_oct; }
    QString binValue() const { return m_bin; }
    QVariantList bits() const;
    QStringList memory() const { return m_memory; }
    QVariantList history() const;
    QString decimalSeparator() const { return QString(m_resources->decimalSeparator()); }

    Q_INVOKABLE void sendCommand(int command);

    Q_INVOKABLE void memoryStore();
    Q_INVOKABLE void memoryRecall();
    Q_INVOKABLE void memoryAdd();
    Q_INVOKABLE void memorySubtract();
    Q_INVOKABLE void memoryClearAll();
    Q_INVOKABLE void memoryItemLoad(int index);
    Q_INVOKABLE void memoryItemAdd(int index);
    Q_INVOKABLE void memoryItemSubtract(int index);
    Q_INVOKABLE void memoryItemClear(int index);

    // 'index' is the position in the engine's history list (see history()).
    Q_INVOKABLE void historyLoad(int index);
    Q_INVOKABLE void historyRemove(int index);
    Q_INVOKABLE void historyClear();

    Q_INVOKABLE void toggleBit(int bit);

    Q_INVOKABLE void copyToClipboard();
    // Copies a displayed number (e.g. a history result) without group separators.
    Q_INVOKABLE void copyText(const QString& text);
    Q_INVOKABLE void pasteFromClipboard();
    // Feeds text such as "12.5*(3-1)" to the engine. Returns false if it can't be parsed.
    Q_INVOKABLE bool pasteText(const QString& text);

    // Handles a key press. Returns true if the key was consumed.
    Q_INVOKABLE bool handleKey(int key, int modifiers, const QString& text);

    // ICalcDisplay
    void SetPrimaryDisplay(const std::wstring& text, bool isError) override;
    void SetIsInError(bool isInError) override;
    void SetExpressionDisplay(
        std::shared_ptr<std::vector<std::pair<std::wstring, int>>> const& tokens,
        std::shared_ptr<std::vector<std::shared_ptr<IExpressionCommand>>> const& commands) override;
    void SetParenthesisNumber(unsigned int count) override;
    void OnNoRightParenAdded() override;
    void MaxDigitsReached() override;
    void BinaryOperatorReceived() override;
    void OnHistoryItemAdded(unsigned int addedItemIndex) override;
    void SetMemorizedNumbers(const std::vector<std::wstring>& memorizedNumbers) override;
    void MemoryItemChanged(unsigned int indexOfMemory) override;
    void InputChanged() override;

Q_SIGNALS:
    void modeChanged();
    void displayChanged();
    void expressionChanged();
    void parenthesisCountChanged();
    void angleModeChanged();
    void exponentialFormatChanged();
    void radixChanged();
    void wordSizeChanged();
    void programmerValuesChanged();
    void memoryChanged();
    void historyChanged();
    void maxDigitsReached();
    void noRightParenAdded();
    void pasteFailed();
    void copied();

private:
    void updateProgrammerValues();
    void replayCommands(const std::vector<std::shared_ptr<IExpressionCommand>>& commands);
    bool commandsForText(const QString& text, std::vector<int>& commands) const;
    void sendCommands(const std::vector<int>& commands);

    std::unique_ptr<EngineResourceProvider> m_resources;
    std::unique_ptr<CalculationManager::CalculatorManager> m_manager;

    Mode m_mode = Mode::Standard;
    QString m_displayValue = QStringLiteral("0");
    QString m_expression;
    bool m_isError = false;
    int m_parenthesisCount = 0;
    AngleMode m_angleMode = AngleMode::Degrees;
    bool m_exponentialFormat = false;
    int m_radix = 10;
    int m_wordSize = 64;
    QString m_hex, m_dec, m_oct, m_bin;
    quint64 m_bitValue = 0;
    QStringList m_memory;
};
