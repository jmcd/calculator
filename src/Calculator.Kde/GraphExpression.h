// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <QString>
#include <QStringList>

#include <map>
#include <memory>
#include <vector>

// Parses and evaluates graphing equations such as "y = 2x^2 - 3", "sin(x)/x" or
// "a·x + b". This replaces the closed-source math engine the Windows app links
// against (its open-source build only ships mocks).
//
// Supported:
//   numbers (1, 2.5), constants pi/π and e, the variable x, and
//   single-letter parameters (a, b, k...) that become sliders;
//   + - * / ^ and the Unicode forms × · ÷ − √ ² ³; postfix ! and |abs| bars;
//   implicit multiplication: 2x, 3(x+1), (x+1)(x-1), 2sin x, x y;
//   functions: sin cos tan sec csc cot, their inverses (asin / arcsin / sin⁻¹),
//   sinh cosh tanh (and inverses), sqrt cbrt exp ln log log2/log₂ abs floor ceil
//   round sign, and two-argument log(base, x) / root(x, n) / min / max / mod.
class GraphExpression
{
public:
    enum class AngleUnit
    {
        Radians,
        Degrees,
        Gradians,
    };

    enum class Error
    {
        None,
        Empty,
        UnexpectedToken,
        InvalidToken,
        ParenthesisMismatch,   // a ')' without a matching '('
        UnmatchedParenthesis,  // a '(' or '|' that is never closed
        UnexpectedEnd,
        DecimalPointWithoutDigits,
        TooManyDecimalPoints,
        TooManyEquals,
        UnknownFunction,
        WrongArgumentCount,
        NotSupported, // e.g. implicit equations such as x^2 + y^2 = 1
    };

    struct Node;

    GraphExpression();
    ~GraphExpression();
    GraphExpression(GraphExpression&&) noexcept;
    GraphExpression& operator=(GraphExpression&&) noexcept;

    // Parses "y = <expr>", "f(x) = <expr>" or just "<expr>".
    static GraphExpression parse(const QString& text);

    bool isValid() const
    {
        return m_root != nullptr;
    }
    Error error() const
    {
        return m_error;
    }
    // Position in the input where the error was found, or -1.
    int errorPosition() const
    {
        return m_errorPosition;
    }

    // Single-letter parameters other than x, in order of first appearance.
    const QStringList& parameters() const
    {
        return m_parameters;
    }

    // Evaluates y for the given x. Returns NaN outside the domain.
    double evaluate(double x, const std::map<QString, double>& parameters, AngleUnit unit) const;
    // Faster form for plotting: values are given in the order of parameters().
    double evaluate(double x, const std::vector<double>& parameterValues, AngleUnit unit) const;

private:
    std::unique_ptr<Node> m_root;
    Error m_error = Error::Empty;
    int m_errorPosition = -1;
    QStringList m_parameters;

    friend class GraphExpressionParser;
};
