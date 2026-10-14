// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CalculatorController.h"

#include "Header Files/RadixType.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QVariantMap>

using CalculationManager::Command;
using CalculationManager::CommandType;

namespace
{
    QString ToQString(const std::wstring& s)
    {
        return QString::fromStdWString(s);
    }

    // Pads the leading group of a grouped binary string to a full nibble (e.g. "11 0101" -> "0011 0101").
    QString PadBinary(const QString& binary)
    {
        const qsizetype firstSpace = binary.indexOf(u' ');
        const qsizetype leadLength = firstSpace < 0 ? binary.size() : firstSpace;
        if (leadLength == 0 || leadLength % 4 == 0)
        {
            return binary;
        }
        return QString(4 - leadLength % 4, u'0') + binary;
    }

    int CommandFor(Command c)
    {
        return static_cast<int>(c);
    }
}

CalculatorController::CalculatorController(QObject* parent)
    : QObject(parent)
    , m_resources(std::make_unique<EngineResourceProvider>())
{
    m_manager = std::make_unique<CalculationManager::CalculatorManager>(this, m_resources.get());
    m_manager->Reset();
    updateProgrammerValues();
}

CalculatorController::~CalculatorController() = default;

// ---------------------------------------------------------------------------
// Mode and settings
// ---------------------------------------------------------------------------

void CalculatorController::setMode(Mode mode)
{
    if (mode == m_mode)
    {
        return;
    }
    m_mode = mode;
    m_isError = false;

    switch (mode)
    {
    case Mode::Standard:
        m_manager->SendCommand(Command::ModeBasic);
        m_manager->SetRadix(RadixType::Decimal);
        break;
    case Mode::Scientific:
        m_manager->SendCommand(Command::ModeScientific);
        m_manager->SetRadix(RadixType::Decimal);
        m_manager->SendCommand(static_cast<Command>(CommandFor(Command::CommandDEG) + static_cast<int>(m_angleMode)));
        break;
    case Mode::Programmer:
        m_manager->SendCommand(Command::ModeProgrammer);
        m_radix = 10;
        Q_EMIT radixChanged();
        m_manager->SetRadix(RadixType::Decimal);
        {
            const int wordSize = m_wordSize;
            m_wordSize = 0;
            setWordSize(wordSize);
        }
        break;
    }

    Q_EMIT modeChanged();
    Q_EMIT historyChanged();
    updateProgrammerValues();
}

void CalculatorController::setAngleMode(AngleMode mode)
{
    if (mode == m_angleMode)
    {
        return;
    }
    m_angleMode = mode;
    if (m_mode == Mode::Scientific)
    {
        m_manager->SendCommand(static_cast<Command>(CommandFor(Command::CommandDEG) + static_cast<int>(mode)));
    }
    Q_EMIT angleModeChanged();
}

void CalculatorController::setExponentialFormat(bool enabled)
{
    if (enabled == m_exponentialFormat)
    {
        return;
    }
    m_exponentialFormat = enabled;
    m_manager->SendCommand(Command::CommandFE);
    Q_EMIT exponentialFormatChanged();
}

void CalculatorController::setRadix(int radix)
{
    RadixType type;
    switch (radix)
    {
    case 16:
        type = RadixType::Hex;
        break;
    case 8:
        type = RadixType::Octal;
        break;
    case 2:
        type = RadixType::Binary;
        break;
    default:
        radix = 10;
        type = RadixType::Decimal;
        break;
    }

    if (m_isError)
    {
        m_manager->SendCommand(Command::CommandCLEAR);
    }
    m_radix = radix;
    m_manager->SetRadix(type);
    Q_EMIT radixChanged();
    updateProgrammerValues();
}

void CalculatorController::setWordSize(int bits)
{
    Command command;
    switch (bits)
    {
    case 8:
        command = Command::CommandByte;
        break;
    case 16:
        command = Command::CommandWord;
        break;
    case 32:
        command = Command::CommandDword;
        break;
    default:
        bits = 64;
        command = Command::CommandQword;
        break;
    }
    if (bits == m_wordSize)
    {
        return;
    }
    m_wordSize = bits;
    if (m_mode == Mode::Programmer)
    {
        m_manager->SendCommand(command);
    }
    Q_EMIT wordSizeChanged();
    updateProgrammerValues();
}

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------

void CalculatorController::sendCommand(int command)
{
    if (command == CommandFor(Command::CommandFE))
    {
        setExponentialFormat(!m_exponentialFormat);
        return;
    }
    if (command >= CommandFor(Command::CommandDEG) && command <= CommandFor(Command::CommandGRAD))
    {
        setAngleMode(static_cast<AngleMode>(command - CommandFor(Command::CommandDEG)));
        return;
    }
    m_manager->SendCommand(static_cast<Command>(command));
}

void CalculatorController::sendCommands(const std::vector<int>& commands)
{
    for (int command : commands)
    {
        sendCommand(command);
    }
}

void CalculatorController::memoryStore()
{
    m_manager->MemorizeNumber();
}

void CalculatorController::memoryRecall()
{
    if (!m_memory.isEmpty())
    {
        m_manager->MemorizedNumberLoad(0);
    }
}

void CalculatorController::memoryAdd()
{
    m_manager->MemorizedNumberAdd(0);
}

void CalculatorController::memorySubtract()
{
    m_manager->MemorizedNumberSubtract(0);
}

void CalculatorController::memoryClearAll()
{
    m_manager->MemorizedNumberClearAll();
}

void CalculatorController::memoryItemLoad(int index)
{
    if (index >= 0 && index < m_memory.size())
    {
        m_manager->MemorizedNumberLoad(static_cast<unsigned int>(index));
    }
}

void CalculatorController::memoryItemAdd(int index)
{
    if (index >= 0 && index < m_memory.size())
    {
        m_manager->MemorizedNumberAdd(static_cast<unsigned int>(index));
    }
}

void CalculatorController::memoryItemSubtract(int index)
{
    if (index >= 0 && index < m_memory.size())
    {
        m_manager->MemorizedNumberSubtract(static_cast<unsigned int>(index));
    }
}

void CalculatorController::memoryItemClear(int index)
{
    if (index >= 0 && index < m_memory.size())
    {
        m_manager->MemorizedNumberClear(static_cast<unsigned int>(index));
        m_manager->SetMemorizedNumbersString();
    }
}

QVariantList CalculatorController::history() const
{
    QVariantList list;
    if (m_mode == Mode::Programmer)
    {
        return list;
    }
    const auto& items = m_manager->GetHistoryItems();
    // Newest first, like the Windows history pane.
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i)
    {
        const auto& item = items[static_cast<size_t>(i)]->historyItemVector;
        QVariantMap entry;
        entry[QStringLiteral("index")] = i;
        entry[QStringLiteral("expression")] = ToQString(item.expression).simplified();
        entry[QStringLiteral("result")] = ToQString(item.result);
        list.append(entry);
    }
    return list;
}

void CalculatorController::replayCommands(const std::vector<std::shared_ptr<IExpressionCommand>>& commands)
{
    // Port of StandardCalculatorViewModel.GetCommandsFromExpressionCommands.
    std::vector<int> codes;
    for (const auto& command : commands)
    {
        switch (command->GetCommandType())
        {
        case CommandType::UnaryCommand:
            for (int code : *std::static_pointer_cast<IUnaryCommand>(command)->GetCommands())
            {
                codes.push_back(code);
            }
            break;
        case CommandType::BinaryCommand:
            codes.push_back(std::static_pointer_cast<IBinaryCommand>(command)->GetCommand());
            break;
        case CommandType::Parentheses:
            codes.push_back(std::static_pointer_cast<IParenthesisCommand>(command)->GetCommand());
            break;
        case CommandType::OperandCommand:
        {
            auto operand = std::static_pointer_cast<IOpndCommand>(command);
            bool needSign = operand->IsNegative();
            for (int code : *operand->GetCommands())
            {
                codes.push_back(code);
                if (needSign && code != CommandFor(Command::Command0))
                {
                    codes.push_back(CommandFor(Command::CommandSIGN));
                    needSign = false;
                }
            }
            break;
        }
        }
    }

    for (int code : codes)
    {
        m_manager->SendCommand(static_cast<Command>(code));
    }
}

void CalculatorController::historyLoad(int index)
{
    const auto& items = m_manager->GetHistoryItems();
    if (index < 0 || static_cast<size_t>(index) >= items.size())
    {
        return;
    }
    // Copy: Reset() below must not invalidate what we're replaying.
    const auto item = items[static_cast<size_t>(index)]->historyItemVector;

    // Port of StandardCalculatorViewModel.Recalculate(fromHistory: true): rebuild the
    // engine state by replaying the item's commands without updating the display,
    // then show the stored expression and result.
    m_manager->SetInHistoryItemLoadMode(true);
    m_manager->Reset(false);
    if (m_mode == Mode::Scientific)
    {
        m_manager->SendCommand(Command::ModeScientific);
        if (m_exponentialFormat)
        {
            m_manager->SendCommand(Command::CommandFE);
        }
        m_manager->SendCommand(static_cast<Command>(CommandFor(Command::CommandDEG) + static_cast<int>(m_angleMode)));
    }
    replayCommands(*item.spCommands);
    m_manager->SendCommand(Command::CommandFE);
    m_manager->SendCommand(Command::CommandFE);
    m_manager->SetInHistoryItemLoadMode(false);

    SetExpressionDisplay(item.spTokens, item.spCommands);
    SetPrimaryDisplay(item.result, false);
}

void CalculatorController::historyRemove(int index)
{
    if (index >= 0 && m_manager->RemoveHistoryItem(static_cast<unsigned int>(index)))
    {
        Q_EMIT historyChanged();
    }
}

void CalculatorController::historyClear()
{
    m_manager->ClearHistory();
    Q_EMIT historyChanged();
}

void CalculatorController::toggleBit(int bit)
{
    if (bit >= 0 && bit < m_wordSize)
    {
        m_manager->SendCommand(static_cast<Command>(CommandFor(Command::CommandBINEDITSTART) + bit));
    }
}

QVariantList CalculatorController::bits() const
{
    QVariantList result;
    result.reserve(64);
    for (int i = 0; i < 64; ++i)
    {
        result.append(((m_bitValue >> i) & 1u) != 0);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Clipboard
// ---------------------------------------------------------------------------

void CalculatorController::copyToClipboard()
{
    if (m_isError)
    {
        QGuiApplication::clipboard()->setText(m_displayValue);
        Q_EMIT copied();
        return;
    }
    copyText(m_displayValue);
}

void CalculatorController::copyText(const QString& text)
{
    QString value = text;
    value.remove(m_resources->groupSeparator());
    if (m_mode == Mode::Programmer)
    {
        value.remove(u' ');
    }
    QGuiApplication::clipboard()->setText(value);
    Q_EMIT copied();
}

void CalculatorController::pasteFromClipboard()
{
    pasteText(QGuiApplication::clipboard()->text());
}

bool CalculatorController::commandsForText(const QString& input, std::vector<int>& commands) const
{
    const QChar decimal = m_resources->decimalSeparator();
    const QChar group = m_resources->groupSeparator();
    const bool programmer = m_mode == Mode::Programmer;

    QString text;
    for (QChar c : input.trimmed())
    {
        if (c.isSpace() || c == u'_' || c == u'\'' || (c == group && !(programmer && c == u'.')))
        {
            continue;
        }
        text.append(c);
    }
    if (text.isEmpty() || text.size() > 400)
    {
        return false;
    }

    auto digitValue = [&](QChar c) -> int {
        if (c.isDigit())
        {
            return c.digitValue();
        }
        if (programmer && m_radix == 16)
        {
            const char16_t u = c.toUpper().unicode();
            if (u >= u'A' && u <= u'F')
            {
                return 10 + (u - u'A');
            }
        }
        return -1;
    };

    // Allow common programming prefixes in programmer mode.
    auto stripPrefix = [&](qsizetype& i) {
        if (programmer && i + 1 < text.size() && text[i] == u'0')
        {
            const QChar p = text[i + 1].toLower();
            if ((p == u'x' && m_radix == 16) || (p == u'b' && m_radix == 2) || (p == u'o' && m_radix == 8))
            {
                i += 2;
            }
        }
    };

    bool expectOperand = true;
    qsizetype i = 0;
    while (i < text.size())
    {
        const QChar c = text[i];
        bool negative = false;

        if (expectOperand && (c == u'-' || c == u'−') && i + 1 < text.size() && (digitValue(text[i + 1]) >= 0 || text[i + 1] == decimal))
        {
            negative = true;
            ++i;
        }

        if (i < text.size() && (digitValue(text[i]) >= 0 || (!programmer && text[i] == decimal)))
        {
            if (!expectOperand)
            {
                return false;
            }
            stripPrefix(i);
            bool sawDecimal = false;
            while (i < text.size())
            {
                const QChar d = text[i];
                const int v = digitValue(d);
                if (v >= 0)
                {
                    if (v >= m_radix)
                    {
                        return false;
                    }
                    commands.push_back(CommandFor(Command::Command0) + v);
                }
                else if (!programmer && d == decimal && !sawDecimal)
                {
                    sawDecimal = true;
                    commands.push_back(CommandFor(Command::CommandPNT));
                }
                else if (!programmer && (d == u'e' || d == u'E') && i + 1 < text.size())
                {
                    // Scientific notation: 1.5e-7
                    commands.push_back(CommandFor(Command::CommandEXP));
                    bool negExp = false;
                    if (text[i + 1] == u'+' || text[i + 1] == u'-')
                    {
                        negExp = text[i + 1] == u'-';
                        ++i;
                    }
                    ++i;
                    bool any = false;
                    while (i < text.size() && text[i].isDigit())
                    {
                        commands.push_back(CommandFor(Command::Command0) + text[i].digitValue());
                        ++i;
                        any = true;
                    }
                    if (!any)
                    {
                        return false;
                    }
                    if (negExp)
                    {
                        commands.push_back(CommandFor(Command::CommandSIGN));
                    }
                    break;
                }
                else
                {
                    break;
                }
                ++i;
            }
            if (negative)
            {
                commands.push_back(CommandFor(Command::CommandSIGN));
            }
            expectOperand = false;
            continue;
        }
        if (negative)
        {
            return false;
        }

        int op = -1;
        switch (c.unicode())
        {
        case u'+':
            op = CommandFor(Command::CommandADD);
            break;
        case u'-':
        case u'−':
            op = CommandFor(Command::CommandSUB);
            break;
        case u'*':
        case u'x':
        case u'X':
        case u'×':
            op = CommandFor(Command::CommandMUL);
            break;
        case u'/':
        case u'÷':
            op = CommandFor(Command::CommandDIV);
            break;
        case u'^':
            op = programmer ? CommandFor(Command::CommandXor) : (m_mode == Mode::Scientific ? CommandFor(Command::CommandPWR) : -1);
            break;
        case u'%':
            op = programmer ? CommandFor(Command::CommandMOD) : -1;
            break;
        case u'&':
            op = programmer ? CommandFor(Command::CommandAnd) : -1;
            break;
        case u'|':
            op = programmer ? CommandFor(Command::CommandOR) : -1;
            break;
        case u'(':
            if (!expectOperand || m_mode == Mode::Standard)
            {
                return false;
            }
            commands.push_back(CommandFor(Command::CommandOPENP));
            ++i;
            continue;
        case u')':
            if (expectOperand || m_mode == Mode::Standard)
            {
                return false;
            }
            commands.push_back(CommandFor(Command::CommandCLOSEP));
            ++i;
            continue;
        case u'=':
            if (i != text.size() - 1)
            {
                return false;
            }
            commands.push_back(CommandFor(Command::CommandEQU));
            ++i;
            continue;
        default:
            break;
        }

        if (op < 0 || expectOperand)
        {
            return false;
        }
        commands.push_back(op);
        expectOperand = true;
        ++i;
    }

    return !commands.empty();
}

bool CalculatorController::pasteText(const QString& text)
{
    std::vector<int> commands;
    if (!commandsForText(text, commands))
    {
        Q_EMIT pasteFailed();
        return false;
    }
    if (m_isError)
    {
        m_manager->SendCommand(Command::CommandCLEAR);
    }
    // Start a fresh entry, like Windows does.
    m_manager->SendCommand(Command::CommandCENTR);
    sendCommands(commands);
    return true;
}

// ---------------------------------------------------------------------------
// Keyboard
// ---------------------------------------------------------------------------

bool CalculatorController::handleKey(int key, int modifiers, const QString& text)
{
    const bool ctrl = modifiers & Qt::ControlModifier;
    const bool shift = modifiers & Qt::ShiftModifier;
    const bool alt = modifiers & Qt::AltModifier;
    const bool sci = m_mode == Mode::Scientific;
    const bool prog = m_mode == Mode::Programmer;

    if (alt)
    {
        return false;
    }

    if (ctrl)
    {
        switch (key)
        {
        case Qt::Key_C:
            copyToClipboard();
            return true;
        case Qt::Key_V:
            pasteFromClipboard();
            return true;
        case Qt::Key_M:
            memoryStore();
            return true;
        case Qt::Key_R:
            memoryRecall();
            return true;
        case Qt::Key_P:
            memoryAdd();
            return true;
        case Qt::Key_Q:
            memorySubtract();
            return true;
        case Qt::Key_L:
            memoryClearAll();
            return true;
        case Qt::Key_D:
            if (shift)
            {
                historyClear();
                return true;
            }
            return false;
        case Qt::Key_G:
            if (sci)
            {
                sendCommand(PowerOf10);
                return true;
            }
            return false;
        case Qt::Key_Y:
            if (sci)
            {
                sendCommand(Root);
                return true;
            }
            return false;
        case Qt::Key_N:
            if (sci)
            {
                sendCommand(PowerOfE);
                return true;
            }
            return false;
        case Qt::Key_S:
            if (sci)
            {
                sendCommand(shift ? Asinh : Sinh);
                return true;
            }
            return false;
        case Qt::Key_O:
            if (sci)
            {
                sendCommand(shift ? Acosh : Cosh);
                return true;
            }
            return false;
        case Qt::Key_T:
            if (sci)
            {
                sendCommand(shift ? Atanh : Tanh);
                return true;
            }
            return false;
        default:
            return false;
        }
    }

    // Keys that don't depend on the produced character.
    switch (key)
    {
    case Qt::Key_Enter:
    case Qt::Key_Return:
        sendCommand(Equals);
        return true;
    case Qt::Key_Escape:
        sendCommand(Clear);
        return true;
    case Qt::Key_Delete:
        sendCommand(ClearEntry);
        return true;
    case Qt::Key_Backspace:
        sendCommand(Backspace);
        return true;
    case Qt::Key_F9:
        sendCommand(Negate);
        return true;
    case Qt::Key_F2:
        if (prog)
        {
            setWordSize(32);
            return true;
        }
        return false;
    case Qt::Key_F3:
        if (sci)
        {
            setAngleMode(AngleMode::Degrees);
            return true;
        }
        if (prog)
        {
            setWordSize(16);
            return true;
        }
        return false;
    case Qt::Key_F4:
        if (sci)
        {
            setAngleMode(AngleMode::Radians);
            return true;
        }
        if (prog)
        {
            setWordSize(8);
            return true;
        }
        return false;
    case Qt::Key_F5:
        if (sci)
        {
            setAngleMode(AngleMode::Gradians);
            return true;
        }
        if (prog)
        {
            setRadix(16);
            return true;
        }
        return false;
    case Qt::Key_F6:
        if (prog)
        {
            setRadix(10);
            return true;
        }
        return false;
    case Qt::Key_F7:
        if (prog)
        {
            setRadix(8);
            return true;
        }
        return false;
    case Qt::Key_F8:
        if (prog)
        {
            setRadix(2);
            return true;
        }
        return false;
    case Qt::Key_F12:
        if (prog)
        {
            setWordSize(64);
            return true;
        }
        return false;
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
        const int v = c.digitValue();
        if (v < m_radix || !prog)
        {
            sendCommand(Zero + v);
        }
        return true;
    }
    if (prog && m_radix == 16)
    {
        const char16_t u = c.toUpper().unicode();
        if (u >= u'A' && u <= u'F')
        {
            sendCommand(HexA + (u - u'A'));
            return true;
        }
    }
    if (c == m_resources->decimalSeparator() || c == u'.' || c == u',')
    {
        if (!prog)
        {
            sendCommand(Decimal);
        }
        return true;
    }

    switch (c.unicode())
    {
    case u'+':
        sendCommand(Add);
        return true;
    case u'-':
        sendCommand(Subtract);
        return true;
    case u'*':
        sendCommand(Multiply);
        return true;
    case u'/':
        sendCommand(Divide);
        return true;
    case u'=':
        sendCommand(Equals);
        return true;
    case u'%':
        sendCommand(prog ? Mod : Percent);
        return true;
    case u'(':
        if (m_mode != Mode::Standard)
        {
            sendCommand(OpenParen);
            return true;
        }
        return false;
    case u')':
        if (m_mode != Mode::Standard)
        {
            sendCommand(CloseParen);
            return true;
        }
        return false;
    default:
        break;
    }

    if (prog)
    {
        switch (c.unicode())
        {
        case u'&':
            sendCommand(And);
            return true;
        case u'|':
            sendCommand(Or);
            return true;
        case u'^':
            sendCommand(Xor);
            return true;
        case u'~':
            sendCommand(Not);
            return true;
        case u'<':
            sendCommand(Lsh);
            return true;
        case u'>':
            sendCommand(Rsh);
            return true;
        case u'j':
        case u'J':
            sendCommand(Rol);
            return true;
        case u'k':
        case u'K':
            sendCommand(Ror);
            return true;
        default:
            return false;
        }
    }

    switch (c.unicode())
    {
    case u'r':
    case u'R':
        sendCommand(Reciprocal);
        return true;
    case u'@':
        sendCommand(SquareRoot);
        return true;
    case u'q':
    case u'Q':
        sendCommand(Square);
        return true;
    default:
        break;
    }

    if (!sci)
    {
        return false;
    }

    switch (c.unicode())
    {
    case u'^':
    case u'y':
    case u'Y':
        sendCommand(Power);
        return true;
    case u'#':
        sendCommand(Cube);
        return true;
    case u'!':
        sendCommand(Factorial);
        return true;
    case u'p':
    case u'P':
        sendCommand(Pi);
        return true;
    case u'l':
        sendCommand(Log);
        return true;
    case u'L':
        sendCommand(LogBaseY);
        return true;
    case u'n':
    case u'N':
        sendCommand(Ln);
        return true;
    case u'x':
    case u'X':
        sendCommand(Exp);
        return true;
    case u'v':
    case u'V':
        sendCommand(FE);
        return true;
    case u'm':
    case u'M':
        sendCommand(Dms);
        return true;
    case u'|':
        sendCommand(Abs);
        return true;
    case u's':
        sendCommand(Sin);
        return true;
    case u'S':
        sendCommand(Asin);
        return true;
    case u'o':
        sendCommand(Cos);
        return true;
    case u'O':
        sendCommand(Acos);
        return true;
    case u't':
        sendCommand(Tan);
        return true;
    case u'T':
        sendCommand(Atan);
        return true;
    case u'e':
    case u'E':
        sendCommand(Euler);
        return true;
    default:
        return false;
    }
}

// ---------------------------------------------------------------------------
// ICalcDisplay (callbacks from the engine)
// ---------------------------------------------------------------------------

void CalculatorController::SetPrimaryDisplay(const std::wstring& text, bool isError)
{
    m_displayValue = ToQString(text);
    m_isError = isError;
    Q_EMIT displayChanged();
    updateProgrammerValues();
}

void CalculatorController::SetIsInError(bool isInError)
{
    if (m_isError != isInError)
    {
        m_isError = isInError;
        Q_EMIT displayChanged();
    }
}

void CalculatorController::SetExpressionDisplay(
    std::shared_ptr<std::vector<std::pair<std::wstring, int>>> const& tokens,
    std::shared_ptr<std::vector<std::shared_ptr<IExpressionCommand>>> const& /*commands*/)
{
    QStringList parts;
    if (tokens)
    {
        for (const auto& token : *tokens)
        {
            // Operator tokens carry their own padding; normalize to single spaces.
            const QString part = ToQString(token.first).trimmed();
            if (!part.isEmpty())
            {
                parts.append(part);
            }
        }
    }
    const QString expression = parts.join(u' ');
    if (expression != m_expression)
    {
        m_expression = expression;
        Q_EMIT expressionChanged();
    }
}

void CalculatorController::SetParenthesisNumber(unsigned int count)
{
    if (m_parenthesisCount != static_cast<int>(count))
    {
        m_parenthesisCount = static_cast<int>(count);
        Q_EMIT parenthesisCountChanged();
    }
}

void CalculatorController::OnNoRightParenAdded()
{
    Q_EMIT noRightParenAdded();
}

void CalculatorController::MaxDigitsReached()
{
    Q_EMIT maxDigitsReached();
}

void CalculatorController::BinaryOperatorReceived()
{
}

void CalculatorController::OnHistoryItemAdded(unsigned int /*addedItemIndex*/)
{
    Q_EMIT historyChanged();
}

void CalculatorController::SetMemorizedNumbers(const std::vector<std::wstring>& memorizedNumbers)
{
    QStringList memory;
    // The engine keeps the newest item first, like the Windows memory pane.
    for (const auto& number : memorizedNumbers)
    {
        memory.append(ToQString(number));
    }
    m_memory = memory;
    Q_EMIT memoryChanged();
}

void CalculatorController::MemoryItemChanged(unsigned int /*indexOfMemory*/)
{
}

void CalculatorController::InputChanged()
{
}

void CalculatorController::updateProgrammerValues()
{
    if (m_mode != Mode::Programmer)
    {
        return;
    }

    constexpr int precision = 64;
    if (m_isError)
    {
        m_hex = m_dec = m_oct = m_bin = QString();
        m_bitValue = 0;
    }
    else
    {
        m_hex = ToQString(m_manager->GetResultForRadix(16, precision, true));
        if (m_hex.isEmpty())
        {
            m_hex = m_dec = m_oct = m_bin = m_displayValue;
        }
        else
        {
            m_dec = ToQString(m_manager->GetResultForRadix(10, precision, true));
            m_oct = ToQString(m_manager->GetResultForRadix(8, precision, true));
            m_bin = PadBinary(ToQString(m_manager->GetResultForRadix(2, precision, true)));
        }
        bool ok = false;
        m_bitValue = ToQString(m_manager->GetResultForRadix(2, precision, false)).toULongLong(&ok, 2);
        if (!ok)
        {
            m_bitValue = 0;
        }
    }
    Q_EMIT programmerValuesChanged();
}
