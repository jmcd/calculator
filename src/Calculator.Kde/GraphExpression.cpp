// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "GraphExpression.h"

#include <QRegularExpression>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace
{
    constexpr double NaN = std::numeric_limits<double>::quiet_NaN();

    enum class Func
    {
        Sin, Cos, Tan, Sec, Csc, Cot,
        Asin, Acos, Atan, Asec, Acsc, Acot,
        Sinh, Cosh, Tanh, Sech, Csch, Coth,
        Asinh, Acosh, Atanh,
        Sqrt, Cbrt, Exp, Ln, Log, Log2, Abs, Floor, Ceil, Round, Sign,
        Root, Min, Max, Mod,
    };

    struct FuncInfo
    {
        Func func;
        int minArgs;
        int maxArgs;
        // For trig functions, the inverse (used for "sin⁻¹").
        int inverse = -1;
    };

    // Names are matched longest-first when splitting identifiers like "xsin".
    const std::vector<std::pair<QString, FuncInfo>>& Functions()
    {
        static const std::vector<std::pair<QString, FuncInfo>> functions = [] {
            std::vector<std::pair<QString, FuncInfo>> f = {
                { QStringLiteral("sin"), { Func::Sin, 1, 1, int(Func::Asin) } },
                { QStringLiteral("cos"), { Func::Cos, 1, 1, int(Func::Acos) } },
                { QStringLiteral("tan"), { Func::Tan, 1, 1, int(Func::Atan) } },
                { QStringLiteral("sec"), { Func::Sec, 1, 1, int(Func::Asec) } },
                { QStringLiteral("csc"), { Func::Csc, 1, 1, int(Func::Acsc) } },
                { QStringLiteral("cot"), { Func::Cot, 1, 1, int(Func::Acot) } },
                { QStringLiteral("asin"), { Func::Asin, 1, 1 } },
                { QStringLiteral("acos"), { Func::Acos, 1, 1 } },
                { QStringLiteral("atan"), { Func::Atan, 1, 1 } },
                { QStringLiteral("asec"), { Func::Asec, 1, 1 } },
                { QStringLiteral("acsc"), { Func::Acsc, 1, 1 } },
                { QStringLiteral("acot"), { Func::Acot, 1, 1 } },
                { QStringLiteral("arcsin"), { Func::Asin, 1, 1 } },
                { QStringLiteral("arccos"), { Func::Acos, 1, 1 } },
                { QStringLiteral("arctan"), { Func::Atan, 1, 1 } },
                { QStringLiteral("arcsec"), { Func::Asec, 1, 1 } },
                { QStringLiteral("arccsc"), { Func::Acsc, 1, 1 } },
                { QStringLiteral("arccot"), { Func::Acot, 1, 1 } },
                { QStringLiteral("sinh"), { Func::Sinh, 1, 1, int(Func::Asinh) } },
                { QStringLiteral("cosh"), { Func::Cosh, 1, 1, int(Func::Acosh) } },
                { QStringLiteral("tanh"), { Func::Tanh, 1, 1, int(Func::Atanh) } },
                { QStringLiteral("sech"), { Func::Sech, 1, 1 } },
                { QStringLiteral("csch"), { Func::Csch, 1, 1 } },
                { QStringLiteral("coth"), { Func::Coth, 1, 1 } },
                { QStringLiteral("asinh"), { Func::Asinh, 1, 1 } },
                { QStringLiteral("acosh"), { Func::Acosh, 1, 1 } },
                { QStringLiteral("atanh"), { Func::Atanh, 1, 1 } },
                { QStringLiteral("arsinh"), { Func::Asinh, 1, 1 } },
                { QStringLiteral("arcosh"), { Func::Acosh, 1, 1 } },
                { QStringLiteral("artanh"), { Func::Atanh, 1, 1 } },
                { QStringLiteral("sqrt"), { Func::Sqrt, 1, 1 } },
                { QStringLiteral("cbrt"), { Func::Cbrt, 1, 1 } },
                { QStringLiteral("exp"), { Func::Exp, 1, 1 } },
                { QStringLiteral("ln"), { Func::Ln, 1, 1 } },
                { QStringLiteral("log"), { Func::Log, 1, 2 } },
                { QStringLiteral("log2"), { Func::Log2, 1, 1 } },
                { QStringLiteral("abs"), { Func::Abs, 1, 1 } },
                { QStringLiteral("floor"), { Func::Floor, 1, 1 } },
                { QStringLiteral("ceil"), { Func::Ceil, 1, 1 } },
                { QStringLiteral("round"), { Func::Round, 1, 1 } },
                { QStringLiteral("sign"), { Func::Sign, 1, 1 } },
                { QStringLiteral("sgn"), { Func::Sign, 1, 1 } },
                { QStringLiteral("root"), { Func::Root, 2, 2 } },
                { QStringLiteral("min"), { Func::Min, 1, 64 } },
                { QStringLiteral("max"), { Func::Max, 1, 64 } },
                { QStringLiteral("mod"), { Func::Mod, 2, 2 } },
            };
            std::stable_sort(f.begin(), f.end(), [](const auto& a, const auto& b) { return a.first.size() > b.first.size(); });
            return f;
        }();
        return functions;
    }

    const FuncInfo* FindFunction(QStringView name)
    {
        for (const auto& [n, info] : Functions())
        {
            if (n == name)
            {
                return &info;
            }
        }
        return nullptr;
    }

    bool IsTrig(Func f)
    {
        return f <= Func::Cot;
    }

    // Real n-th root: odd roots of negative numbers are negative.
    double RealRoot(double x, double n)
    {
        if (x < 0)
        {
            const double rounded = std::round(n);
            if (rounded == n && std::fmod(std::abs(rounded), 2.0) == 1.0)
            {
                return -std::pow(-x, 1.0 / n);
            }
            return NaN;
        }
        return std::pow(x, 1.0 / n);
    }

    // pow() with real results for negative bases and exponents like 1/3 or 2/3.
    double RealPow(double base, double exponent)
    {
        if (base >= 0 || std::round(exponent) == exponent)
        {
            return std::pow(base, exponent);
        }
        // exponent = p/q with odd q: (-a)^(p/q) is real.
        for (int q = 3; q <= 99; q += 2)
        {
            const double p = exponent * q;
            const double pr = std::round(p);
            if (std::abs(p - pr) < 1e-9)
            {
                const double magnitude = std::pow(-base, exponent);
                return std::fmod(std::abs(pr), 2.0) == 1.0 ? -magnitude : magnitude;
            }
        }
        return NaN;
    }
}

struct GraphExpression::Node
{
    enum class Kind
    {
        Number,
        X,
        Parameter,
        Negate,
        Add,
        Subtract,
        Multiply,
        Divide,
        Power,
        Factorial,
        Function,
    };

    Kind kind;
    double value = 0;   // Number
    int parameter = -1; // Parameter: index into parameters()
    Func func = Func::Sin;
    std::vector<std::unique_ptr<Node>> children;

    explicit Node(Kind k)
        : kind(k)
    {
    }

    double eval(double x, const std::vector<double>& params, double angleScale) const
    {
        switch (kind)
        {
        case Kind::Number:
            return value;
        case Kind::X:
            return x;
        case Kind::Parameter:
            return params[static_cast<size_t>(parameter)];
        case Kind::Negate:
            return -children[0]->eval(x, params, angleScale);
        case Kind::Add:
            return children[0]->eval(x, params, angleScale) + children[1]->eval(x, params, angleScale);
        case Kind::Subtract:
            return children[0]->eval(x, params, angleScale) - children[1]->eval(x, params, angleScale);
        case Kind::Multiply:
            return children[0]->eval(x, params, angleScale) * children[1]->eval(x, params, angleScale);
        case Kind::Divide:
        {
            const double d = children[1]->eval(x, params, angleScale);
            return d == 0 ? NaN : children[0]->eval(x, params, angleScale) / d;
        }
        case Kind::Power:
            return RealPow(children[0]->eval(x, params, angleScale), children[1]->eval(x, params, angleScale));
        case Kind::Factorial:
        {
            const double v = children[0]->eval(x, params, angleScale);
            if (v < 0 && std::round(v) == v)
            {
                return NaN;
            }
            return std::tgamma(v + 1);
        }
        case Kind::Function:
            return evalFunction(x, params, angleScale);
        }
        return NaN;
    }

    double evalFunction(double x, const std::vector<double>& params, double angleScale) const
    {
        const double a = children[0]->eval(x, params, angleScale);
        // Trig functions take angles in the current unit; inverses return them.
        const double t = IsTrig(func) ? a * angleScale : a;
        auto angle = [angleScale](double radians) { return radians / angleScale; };

        switch (func)
        {
        case Func::Sin: return std::sin(t);
        case Func::Cos: return std::cos(t);
        case Func::Tan: return std::tan(t);
        case Func::Sec: return 1.0 / std::cos(t);
        case Func::Csc: return 1.0 / std::sin(t);
        case Func::Cot: return std::cos(t) / std::sin(t);
        case Func::Asin: return angle(std::asin(a));
        case Func::Acos: return angle(std::acos(a));
        case Func::Atan: return angle(std::atan(a));
        case Func::Asec: return angle(std::acos(1.0 / a));
        case Func::Acsc: return angle(std::asin(1.0 / a));
        case Func::Acot: return angle(a == 0 ? std::numbers::pi / 2 : std::atan(1.0 / a));
        case Func::Sinh: return std::sinh(a);
        case Func::Cosh: return std::cosh(a);
        case Func::Tanh: return std::tanh(a);
        case Func::Sech: return 1.0 / std::cosh(a);
        case Func::Csch: return 1.0 / std::sinh(a);
        case Func::Coth: return 1.0 / std::tanh(a);
        case Func::Asinh: return std::asinh(a);
        case Func::Acosh: return std::acosh(a);
        case Func::Atanh: return std::atanh(a);
        case Func::Sqrt: return std::sqrt(a);
        case Func::Cbrt: return std::cbrt(a);
        case Func::Exp: return std::exp(a);
        case Func::Ln: return a > 0 ? std::log(a) : NaN;
        case Func::Log:
            if (children.size() == 2)
            {
                // log(base, x)
                const double v = children[1]->eval(x, params, angleScale);
                return a > 0 && a != 1 && v > 0 ? std::log(v) / std::log(a) : NaN;
            }
            return a > 0 ? std::log10(a) : NaN;
        case Func::Log2: return a > 0 ? std::log2(a) : NaN;
        case Func::Abs: return std::abs(a);
        case Func::Floor: return std::floor(a);
        case Func::Ceil: return std::ceil(a);
        case Func::Round: return std::round(a);
        case Func::Sign: return a > 0 ? 1.0 : (a < 0 ? -1.0 : 0.0);
        case Func::Root: return RealRoot(a, children[1]->eval(x, params, angleScale));
        case Func::Mod:
        {
            const double b = children[1]->eval(x, params, angleScale);
            if (b == 0)
            {
                return NaN;
            }
            const double r = std::fmod(a, b);
            return r != 0 && ((r < 0) != (b < 0)) ? r + b : r;
        }
        case Func::Min:
        case Func::Max:
        {
            double result = a;
            for (size_t i = 1; i < children.size(); ++i)
            {
                const double v = children[i]->eval(x, params, angleScale);
                result = func == Func::Min ? std::min(result, v) : std::max(result, v);
            }
            return result;
        }
        }
        return NaN;
    }
};

// ---------------------------------------------------------------------------
// Tokenizer and parser
// ---------------------------------------------------------------------------

class GraphExpressionParser
{
public:
    using Node = GraphExpression::Node;
    using Error = GraphExpression::Error;

    struct Token
    {
        enum class Type
        {
            Number,
            X,
            Parameter,
            Constant,
            Function,
            Operator,
            LeftParen,
            RightParen,
            Comma,
            Bar,
            Inverse, // ⁻¹ after a function name
            End,
        };
        Token(Type t, int p)
            : type(t)
            , position(p)
        {
        }

        Type type;
        int position;
        double value = 0;
        QChar op;
        QString name;
        const FuncInfo* func = nullptr;
    };

    explicit GraphExpressionParser(GraphExpression& result)
        : m_result(result)
    {
    }

    bool tokenize(const QString& text, int offset)
    {
        int i = 0;
        const int n = static_cast<int>(text.size());
        auto add = [this](Token::Type type, int pos) -> Token& {
            m_tokens.push_back(Token{ type, pos });
            return m_tokens.back();
        };

        while (i < n)
        {
            const QChar c = text[i];
            const int pos = offset + i;

            if (c.isSpace())
            {
                ++i;
                continue;
            }
            if (c.isDigit() || c == u'.')
            {
                int j = i;
                int points = 0;
                while (j < n && (text[j].isDigit() || text[j] == u'.'))
                {
                    points += text[j] == u'.' ? 1 : 0;
                    ++j;
                }
                const QStringView number = QStringView(text).mid(i, j - i);
                if (points > 1)
                {
                    return fail(Error::TooManyDecimalPoints, pos);
                }
                if (number == u".")
                {
                    return fail(Error::DecimalPointWithoutDigits, pos);
                }
                add(Token::Type::Number, pos).value = number.toDouble();
                i = j;
                continue;
            }
            if (c.isLetter() && c != u'\u03C0')
            {
                int j = i;
                while (j < n && text[j].isLetter() && text[j] != u'\u03C0')
                {
                    ++j;
                }
                QString word = text.mid(i, j - i).toLower();
                // log2(x) and log₂(x)
                if (word.endsWith(u"log") && j < n && (text[j] == u'2' || text[j] == u'\u2082') && (j + 1 >= n || !text[j + 1].isDigit()))
                {
                    word += u'2';
                    ++j;
                }
                if (!splitIdentifier(word, pos))
                {
                    return false;
                }
                i = j;
                continue;
            }

            switch (c.unicode())
            {
            case u'+':
            case u'-':
            case u'*':
            case u'/':
            case u'^':
            case u'!':
                add(Token::Type::Operator, pos).op = c;
                break;
            case u'−': // minus sign
            case u'–': // en dash
                add(Token::Type::Operator, pos).op = u'-';
                break;
            case u'×': // ×
            case u'·': // ·
            case u'⋅': // ⋅
            case u'∙': // ∙
                add(Token::Type::Operator, pos).op = u'*';
                break;
            case u'÷': // ÷
                add(Token::Type::Operator, pos).op = u'/';
                break;
            case u'²': // ²
            case u'³': // ³
                add(Token::Type::Operator, pos).op = u'^';
                add(Token::Type::Number, pos).value = c == u'²' ? 2 : 3;
                break;
            case u'√': // √
            {
                Token& t = add(Token::Type::Function, pos);
                t.func = FindFunction(u"sqrt");
                t.name = QStringLiteral("sqrt");
                break;
            }
            case u'π': // π
            {
                Token& t = add(Token::Type::Constant, pos);
                t.value = std::numbers::pi;
                break;
            }
            case u'⁻': // ⁻ (as in sin⁻¹)
                if (i + 1 < n && text[i + 1] == u'¹')
                {
                    add(Token::Type::Inverse, pos);
                    ++i;
                    break;
                }
                return fail(Error::InvalidToken, pos);
            case u'(':
            case u'[':
                add(Token::Type::LeftParen, pos);
                break;
            case u')':
            case u']':
                add(Token::Type::RightParen, pos);
                break;
            case u',':
            case u';':
                add(Token::Type::Comma, pos);
                break;
            case u'|':
                add(Token::Type::Bar, pos);
                break;
            default:
                return fail(Error::InvalidToken, pos);
            }
            ++i;
        }
        add(Token::Type::End, offset + n);
        return true;
    }

    // Splits a run of letters into function names, constants and single-letter
    // variables: "xsin" -> x, sin; "ab" -> a, b; "pi" -> π.
    bool splitIdentifier(const QString& word, int pos)
    {
        int i = 0;
        while (i < word.size())
        {
            const QStringView rest = QStringView(word).mid(i);
            bool matched = false;
            for (const auto& [name, info] : Functions())
            {
                if (rest.startsWith(name))
                {
                    m_tokens.push_back(Token{ Token::Type::Function, pos + i });
                    m_tokens.back().func = &info;
                    m_tokens.back().name = name;
                    i += static_cast<int>(name.size());
                    matched = true;
                    break;
                }
            }
            if (matched)
            {
                continue;
            }
            if (rest.startsWith(u"pi"))
            {
                m_tokens.push_back(Token{ Token::Type::Constant, pos + i });
                m_tokens.back().value = std::numbers::pi;
                i += 2;
                continue;
            }

            const QChar c = word[i];
            if (c == u'x')
            {
                m_tokens.push_back(Token{ Token::Type::X, pos + i });
            }
            else if (c == u'e')
            {
                m_tokens.push_back(Token{ Token::Type::Constant, pos + i });
                m_tokens.back().value = std::numbers::e;
            }
            else if (c == u'y')
            {
                // y on the right-hand side means an implicit equation.
                return fail(Error::NotSupported, pos + i);
            }
            else if (c >= u'a' && c <= u'z')
            {
                m_tokens.push_back(Token{ Token::Type::Parameter, pos + i });
                m_tokens.back().name = QString(c);
            }
            else
            {
                return fail(Error::InvalidToken, pos + i);
            }
            ++i;
        }
        return true;
    }

    std::unique_ptr<Node> parseAll()
    {
        if (m_tokens.size() == 1)
        {
            fail(Error::Empty, m_tokens[0].position);
            return nullptr;
        }
        auto node = parseExpression();
        if (!node)
        {
            return nullptr;
        }
        if (peek().type == Token::Type::RightParen)
        {
            fail(Error::ParenthesisMismatch, peek().position);
            return nullptr;
        }
        if (peek().type != Token::Type::End)
        {
            fail(Error::UnexpectedToken, peek().position);
            return nullptr;
        }
        return node;
    }

    bool fail(Error error, int position)
    {
        if (m_result.m_error == Error::None)
        {
            m_result.m_error = error;
            m_result.m_errorPosition = position;
        }
        return false;
    }

private:
    const Token& peek() const
    {
        return m_tokens[m_pos];
    }
    const Token& next()
    {
        return m_tokens[m_pos++];
    }
    bool isOp(QChar op) const
    {
        return peek().type == Token::Type::Operator && peek().op == op;
    }

    static std::unique_ptr<Node> binary(Node::Kind kind, std::unique_ptr<Node> a, std::unique_ptr<Node> b)
    {
        auto node = std::make_unique<Node>(kind);
        node->children.push_back(std::move(a));
        node->children.push_back(std::move(b));
        return node;
    }

    // Whether the next token can start a factor for implicit multiplication.
    bool startsImplicitFactor() const
    {
        switch (peek().type)
        {
        case Token::Type::Number:
        case Token::Type::X:
        case Token::Type::Parameter:
        case Token::Type::Constant:
        case Token::Type::Function:
        case Token::Type::LeftParen:
            return true;
        case Token::Type::Bar:
            // A bar opens a new |...| only when we're not inside one.
            return m_absDepth == 0;
        default:
            return false;
        }
    }

    std::unique_ptr<Node> parseExpression()
    {
        auto left = parseTerm();
        while (left && (isOp(u'+') || isOp(u'-')))
        {
            const bool add = next().op == u'+';
            auto right = parseTerm();
            if (!right)
            {
                return nullptr;
            }
            left = binary(add ? Node::Kind::Add : Node::Kind::Subtract, std::move(left), std::move(right));
        }
        return left;
    }

    std::unique_ptr<Node> parseTerm()
    {
        auto left = parseUnary();
        while (left)
        {
            if (isOp(u'*') || isOp(u'/'))
            {
                const bool multiply = next().op == u'*';
                auto right = parseUnary();
                if (!right)
                {
                    return nullptr;
                }
                left = binary(multiply ? Node::Kind::Multiply : Node::Kind::Divide, std::move(left), std::move(right));
            }
            else if (startsImplicitFactor())
            {
                auto right = parsePower();
                if (!right)
                {
                    return nullptr;
                }
                left = binary(Node::Kind::Multiply, std::move(left), std::move(right));
            }
            else
            {
                break;
            }
        }
        return left;
    }

    std::unique_ptr<Node> parseUnary()
    {
        if (isOp(u'-'))
        {
            next();
            auto operand = parseUnary();
            if (!operand)
            {
                return nullptr;
            }
            auto node = std::make_unique<Node>(Node::Kind::Negate);
            node->children.push_back(std::move(operand));
            return node;
        }
        if (isOp(u'+'))
        {
            next();
            return parseUnary();
        }
        return parsePower();
    }

    std::unique_ptr<Node> parsePower()
    {
        auto base = parsePostfix();
        if (base && isOp(u'^'))
        {
            next();
            // Right-associative; allows 2^-x.
            auto exponent = parseUnary();
            if (!exponent)
            {
                return nullptr;
            }
            return binary(Node::Kind::Power, std::move(base), std::move(exponent));
        }
        return base;
    }

    std::unique_ptr<Node> parsePostfix()
    {
        auto node = parsePrimary();
        while (node && isOp(u'!'))
        {
            next();
            auto factorial = std::make_unique<Node>(Node::Kind::Factorial);
            factorial->children.push_back(std::move(node));
            node = std::move(factorial);
        }
        return node;
    }

    std::unique_ptr<Node> parsePrimary()
    {
        const Token& token = peek();
        switch (token.type)
        {
        case Token::Type::Number:
        case Token::Type::Constant:
        {
            next();
            auto node = std::make_unique<Node>(Node::Kind::Number);
            node->value = token.value;
            return node;
        }
        case Token::Type::X:
            next();
            return std::make_unique<Node>(Node::Kind::X);
        case Token::Type::Parameter:
        {
            next();
            auto node = std::make_unique<Node>(Node::Kind::Parameter);
            qsizetype index = m_result.m_parameters.indexOf(token.name);
            if (index < 0)
            {
                m_result.m_parameters.append(token.name);
                index = m_result.m_parameters.size() - 1;
            }
            node->parameter = static_cast<int>(index);
            return node;
        }
        case Token::Type::LeftParen:
        {
            next();
            const int savedAbs = m_absDepth;
            m_absDepth = 0;
            auto inner = parseExpression();
            m_absDepth = savedAbs;
            if (!inner)
            {
                return nullptr;
            }
            if (peek().type != Token::Type::RightParen)
            {
                fail(peek().type == Token::Type::End ? Error::UnmatchedParenthesis : Error::UnexpectedToken, peek().position);
                return nullptr;
            }
            next();
            return inner;
        }
        case Token::Type::Bar:
        {
            next();
            ++m_absDepth;
            auto inner = parseExpression();
            --m_absDepth;
            if (!inner)
            {
                return nullptr;
            }
            if (peek().type != Token::Type::Bar)
            {
                fail(Error::UnmatchedParenthesis, peek().position);
                return nullptr;
            }
            next();
            auto node = std::make_unique<Node>(Node::Kind::Function);
            node->func = Func::Abs;
            node->children.push_back(std::move(inner));
            return node;
        }
        case Token::Type::Function:
            return parseFunction();
        case Token::Type::End:
            fail(Error::UnexpectedEnd, token.position);
            return nullptr;
        case Token::Type::RightParen:
            fail(Error::ParenthesisMismatch, token.position);
            return nullptr;
        default:
            fail(Error::UnexpectedToken, token.position);
            return nullptr;
        }
    }

    std::unique_ptr<Node> parseFunction()
    {
        const Token& token = next();
        const FuncInfo* info = token.func;

        // sin⁻¹ means arcsin.
        if (peek().type == Token::Type::Inverse)
        {
            next();
            if (info->inverse < 0)
            {
                fail(Error::UnexpectedToken, token.position);
                return nullptr;
            }
            for (const auto& [name, candidate] : Functions())
            {
                if (int(candidate.func) == info->inverse)
                {
                    info = &candidate;
                    break;
                }
            }
        }

        // sin²x means (sin x)².
        std::unique_ptr<Node> power;
        if (isOp(u'^') && m_tokens[m_pos + 1].type == Token::Type::Number)
        {
            next();
            power = std::make_unique<Node>(Node::Kind::Number);
            power->value = next().value;
        }

        auto node = std::make_unique<Node>(Node::Kind::Function);
        node->func = info->func;

        if (peek().type == Token::Type::LeftParen)
        {
            next();
            const int savedAbs = m_absDepth;
            m_absDepth = 0;
            while (true)
            {
                auto arg = parseExpression();
                if (!arg)
                {
                    return nullptr;
                }
                node->children.push_back(std::move(arg));
                if (peek().type == Token::Type::Comma)
                {
                    next();
                    continue;
                }
                break;
            }
            m_absDepth = savedAbs;
            if (peek().type != Token::Type::RightParen)
            {
                fail(peek().type == Token::Type::End ? Error::UnmatchedParenthesis : Error::UnexpectedToken, peek().position);
                return nullptr;
            }
            next();
        }
        else
        {
            // Without parentheses the argument runs over implicitly multiplied
            // factors up to the next function: sin 2x = sin(2x), sin x cos x = sin(x)·cos(x).
            auto arg = parsePower();
            if (!arg)
            {
                return nullptr;
            }
            while (startsImplicitFactor() && peek().type != Token::Type::Function)
            {
                auto factor = parsePower();
                if (!factor)
                {
                    return nullptr;
                }
                arg = binary(Node::Kind::Multiply, std::move(arg), std::move(factor));
            }
            node->children.push_back(std::move(arg));
        }

        const int count = static_cast<int>(node->children.size());
        if (count < info->minArgs || count > info->maxArgs)
        {
            fail(Error::WrongArgumentCount, token.position);
            return nullptr;
        }

        // log₂ written as log2(x) is handled by the name; log(b, x) by the argument count.
        if (power)
        {
            return binary(Node::Kind::Power, std::move(node), std::move(power));
        }
        return node;
    }

    GraphExpression& m_result;
    std::vector<Token> m_tokens;
    size_t m_pos = 0;
    int m_absDepth = 0;
};

// ---------------------------------------------------------------------------

GraphExpression::GraphExpression() = default;
GraphExpression::~GraphExpression() = default;
GraphExpression::GraphExpression(GraphExpression&&) noexcept = default;
GraphExpression& GraphExpression::operator=(GraphExpression&&) noexcept = default;

GraphExpression GraphExpression::parse(const QString& input)
{
    GraphExpression result;
    result.m_error = Error::None;

    const QString text = input;
    const qsizetype equals = text.indexOf(u'=');
    QString body = text;
    int offset = 0;

    if (equals >= 0)
    {
        if (text.indexOf(u'=', equals + 1) >= 0)
        {
            result.m_error = Error::TooManyEquals;
            result.m_errorPosition = static_cast<int>(text.indexOf(u'=', equals + 1));
            return result;
        }
        const QString left = text.left(equals).trimmed();
        const QString right = text.mid(equals + 1).trimmed();
        static const QRegularExpression functionName(QStringLiteral("^[A-Za-z]\\s*\\(\\s*x\\s*\\)$"));

        if (left == u"y" || functionName.match(left).hasMatch())
        {
            body = text.mid(equals + 1);
            offset = static_cast<int>(equals + 1);
        }
        else if (right == u"y")
        {
            body = text.left(equals);
        }
        else
        {
            // e.g. x^2 + y^2 = 1 or x = 3: implicit equations aren't supported yet.
            result.m_error = Error::NotSupported;
            result.m_errorPosition = static_cast<int>(equals);
            return result;
        }
    }

    if (body.trimmed().isEmpty())
    {
        result.m_error = Error::Empty;
        result.m_errorPosition = offset;
        return result;
    }

    GraphExpressionParser parser(result);
    if (!parser.tokenize(body, offset))
    {
        return result;
    }
    result.m_root = parser.parseAll();
    if (!result.m_root)
    {
        result.m_parameters.clear();
        if (result.m_error == Error::None)
        {
            result.m_error = Error::UnexpectedToken;
        }
    }
    return result;
}

double GraphExpression::evaluate(double x, const std::map<QString, double>& parameters, AngleUnit unit) const
{
    if (!m_root)
    {
        return NaN;
    }
    std::vector<double> values;
    values.reserve(static_cast<size_t>(m_parameters.size()));
    for (const QString& name : m_parameters)
    {
        auto it = parameters.find(name);
        values.push_back(it != parameters.end() ? it->second : 1.0);
    }
    return evaluate(x, values, unit);
}

double GraphExpression::evaluate(double x, const std::vector<double>& parameterValues, AngleUnit unit) const
{
    if (!m_root || parameterValues.size() < static_cast<size_t>(m_parameters.size()))
    {
        return NaN;
    }
    double angleScale = 1.0;
    switch (unit)
    {
    case AngleUnit::Degrees:
        angleScale = std::numbers::pi / 180.0;
        break;
    case AngleUnit::Gradians:
        angleScale = std::numbers::pi / 200.0;
        break;
    case AngleUnit::Radians:
        break;
    }
    const double y = m_root->eval(x, parameterValues, angleScale);
    return std::isfinite(y) ? y : NaN;
}
