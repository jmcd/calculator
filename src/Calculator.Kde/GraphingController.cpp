// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "GraphingController.h"
#include "ReswStrings.h"

#include <QSettings>

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    // Minimum gap kept between a slider's min and max (VariableViewModel.DefaultMinMaxRange).
    constexpr double DefaultMinMaxRange = 10;
}

// ---------------------------------------------------------------------------
// EquationModel
// ---------------------------------------------------------------------------

EquationModel::EquationModel(GraphingController* controller)
    : QAbstractListModel(controller)
    , m_controller(controller)
{
}

int EquationModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_equations.size());
}

QVariant EquationModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount())
    {
        return {};
    }
    const Equation& equation = m_equations[static_cast<size_t>(index.row())];
    switch (role)
    {
    case TextRole:
        return equation.text;
    case ColorIndexRole:
        return equation.colorIndex;
    case ShownRole:
        return equation.shown;
    case ErrorRole:
        return equation.error;
    case ValidRole:
        return equation.expression.isValid();
    default:
        return {};
    }
}

QHash<int, QByteArray> EquationModel::roleNames() const
{
    return {
        { TextRole, "text" }, { ColorIndexRole, "colorIndex" }, { ShownRole, "shown" }, { ErrorRole, "error" }, { ValidRole, "valid" },
    };
}

// ---------------------------------------------------------------------------
// VariableModel
// ---------------------------------------------------------------------------

VariableModel::VariableModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int VariableModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_variables.size());
}

QVariant VariableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount())
    {
        return {};
    }
    const Variable& variable = m_variables[static_cast<size_t>(index.row())];
    switch (role)
    {
    case NameRole:
        return variable.name;
    case ValueRole:
        return variable.value;
    case MinRole:
        return variable.min;
    case MaxRole:
        return variable.max;
    case StepRole:
        return variable.step;
    default:
        return {};
    }
}

QHash<int, QByteArray> VariableModel::roleNames() const
{
    return { { NameRole, "name" }, { ValueRole, "value" }, { MinRole, "min" }, { MaxRole, "max" }, { StepRole, "step" } };
}

// ---------------------------------------------------------------------------
// GraphingController
// ---------------------------------------------------------------------------

GraphingController::GraphingController(QObject* parent)
    : GraphingController(true, parent)
{
}

GraphingController::GraphingController(bool persist, QObject* parent)
    : QObject(parent)
    , m_equations(this)
    , m_variables(this)
    , m_persist(persist)
{
    if (m_persist)
    {
        load();
    }
    ensureTrailingEmptyRow();
    updateVariables();
}

QColor GraphingController::equationColor(int colorIndex, bool darkBackground)
{
    // EquationBrush1..14 from the Windows app's App.xaml.
    static const QColor light[] = { QColor(QRgb(0x0063B1)), QColor(QRgb(0x00B7C3)), QColor(QRgb(0x6600CC)), QColor(QRgb(0x107C10)), QColor(QRgb(0x00CC6A)),
                                    QColor(QRgb(0x008055)), QColor(QRgb(0x58595B)), QColor(QRgb(0xE81123)), QColor(QRgb(0xE3008C)), QColor(QRgb(0xB31564)),
                                    QColor(QRgb(0xFFB900)), QColor(QRgb(0xF7630C)), QColor(QRgb(0x8E562E)), QColor(QRgb(0x000000)) };
    static const QColor dark[] = { QColor(QRgb(0x4D92C8)), QColor(QRgb(0x4DCDD5)), QColor(QRgb(0xA366E0)), QColor(QRgb(0x58A358)), QColor(QRgb(0x4DDB97)),
                                   QColor(QRgb(0x4DA688)), QColor(QRgb(0x8A8B8C)), QColor(QRgb(0xEF5865)), QColor(QRgb(0xEB4DAF)), QColor(QRgb(0xCA5B93)),
                                   QColor(QRgb(0xFFCE4D)), QColor(QRgb(0xF99255)), QColor(QRgb(0xB0896D)), QColor(QRgb(0xFFFFFF)) };
    const int i = std::clamp(colorIndex, 0, MaxEquations - 1);
    return darkBackground ? dark[i] : light[i];
}

int GraphingController::nextColorIndex() const
{
    for (int candidate = 0; candidate < MaxEquations; ++candidate)
    {
        const bool used = std::any_of(m_equations.m_equations.begin(), m_equations.m_equations.end(), [candidate](const auto& e) {
            return e.colorIndex == candidate;
        });
        if (!used)
        {
            return candidate;
        }
    }
    return 0;
}

QString GraphingController::errorText(GraphExpression::Error error)
{
    auto* strings = ReswStrings::instance();
    switch (error)
    {
    case GraphExpression::Error::None:
    case GraphExpression::Error::Empty:
        return {};
    case GraphExpression::Error::UnexpectedToken:
        return strings->get(QStringLiteral("UnexpectedToken"), QStringLiteral("Unexpected characters in the expression"));
    case GraphExpression::Error::InvalidToken:
    case GraphExpression::Error::UnknownFunction:
        return strings->get(QStringLiteral("InvalidToken"), QStringLiteral("Invalid characters in the expression"));
    case GraphExpression::Error::ParenthesisMismatch:
        return strings->get(QStringLiteral("ParenthesisMismatch"), QStringLiteral("The equation is missing an opening parenthesis"));
    case GraphExpression::Error::UnmatchedParenthesis:
        return strings->get(QStringLiteral("UnmatchedParenthesis"), QStringLiteral("The equation is missing a closing parenthesis"));
    case GraphExpression::Error::UnexpectedEnd:
        return strings->get(QStringLiteral("UnexpectedEndOfExpression"), QStringLiteral("Unexpected end of expression"));
    case GraphExpression::Error::DecimalPointWithoutDigits:
        return strings->get(QStringLiteral("DecimalPointWithoutDigits"), QStringLiteral("A decimal point is missing digits"));
    case GraphExpression::Error::TooManyDecimalPoints:
        return strings->get(QStringLiteral("TooManyDecimalPoints"), QStringLiteral("There are too many decimal points in a number"));
    case GraphExpression::Error::TooManyEquals:
        return strings->get(QStringLiteral("TooManyEquals"), QStringLiteral("There are too many equal signs"));
    case GraphExpression::Error::WrongArgumentCount:
        return strings->get(QStringLiteral("InvalidEquationSyntax"), QStringLiteral("Invalid expression"));
    case GraphExpression::Error::NotSupported:
        return strings->get(QStringLiteral("GE_NotSupported"), QStringLiteral("Graphing this equation is not supported"));
    }
    return strings->get(QStringLiteral("GeneralError"), QStringLiteral("The equation could not be graphed"));
}

void GraphingController::setAngleUnit(int unit)
{
    const auto value = static_cast<GraphExpression::AngleUnit>(std::clamp(unit, 0, 2));
    if (value == m_angleUnit)
    {
        return;
    }
    m_angleUnit = value;
    save();
    Q_EMIT angleUnitChanged();
    Q_EMIT graphChanged();
}

void GraphingController::setEquationText(int row, const QString& text)
{
    auto& equations = m_equations.m_equations;
    if (row < 0 || static_cast<size_t>(row) >= equations.size() || equations[static_cast<size_t>(row)].text == text)
    {
        return;
    }
    auto& equation = equations[static_cast<size_t>(row)];
    equation.text = text;
    equation.expression = GraphExpression::parse(text);
    equation.error = errorText(equation.expression.error());
    const QModelIndex index = m_equations.index(row);
    Q_EMIT m_equations.dataChanged(index, index);

    ensureTrailingEmptyRow();
    updateVariables();
    save();
    Q_EMIT graphChanged();
}

void GraphingController::setEquationShown(int row, bool shown)
{
    auto& equations = m_equations.m_equations;
    if (row < 0 || static_cast<size_t>(row) >= equations.size())
    {
        return;
    }
    equations[static_cast<size_t>(row)].shown = shown;
    const QModelIndex index = m_equations.index(row);
    Q_EMIT m_equations.dataChanged(index, index, { EquationModel::ShownRole });
    Q_EMIT graphChanged();
}

void GraphingController::removeEquation(int row)
{
    auto& equations = m_equations.m_equations;
    if (row < 0 || static_cast<size_t>(row) >= equations.size())
    {
        return;
    }
    if (static_cast<size_t>(row) == equations.size() - 1 && equations.back().text.isEmpty())
    {
        return;
    }
    m_equations.beginRemoveRows({}, row, row);
    equations.erase(equations.begin() + row);
    m_equations.endRemoveRows();

    ensureTrailingEmptyRow();
    updateVariables();
    save();
    Q_EMIT graphChanged();
}

void GraphingController::ensureTrailingEmptyRow()
{
    auto& equations = m_equations.m_equations;
    if (static_cast<int>(equations.size()) >= MaxEquations || (!equations.empty() && equations.back().text.isEmpty()))
    {
        return;
    }
    EquationModel::Equation equation;
    equation.colorIndex = nextColorIndex();
    const int row = static_cast<int>(equations.size());
    m_equations.beginInsertRows({}, row, row);
    equations.push_back(std::move(equation));
    m_equations.endInsertRows();
}

void GraphingController::updateVariables()
{
    // Parameters of all valid equations, in order of first appearance.
    QStringList names;
    for (const auto& equation : m_equations.m_equations)
    {
        for (const QString& name : equation.expression.parameters())
        {
            if (!names.contains(name))
            {
                names.append(name);
            }
        }
    }

    auto& variables = m_variables.m_variables;
    QStringList current;
    for (const auto& variable : variables)
    {
        current.append(variable.name);
    }

    if (names != current)
    {
        // Keep existing settings for variables that are still used.
        std::vector<VariableModel::Variable> updated;
        for (const QString& name : names)
        {
            auto it = std::find_if(variables.begin(), variables.end(), [&name](const auto& v) { return v.name == name; });
            if (it != variables.end())
            {
                updated.push_back(*it);
            }
            else
            {
                VariableModel::Variable variable;
                variable.name = name;
                updated.push_back(variable);
            }
        }
        m_variables.beginResetModel();
        variables = std::move(updated);
        m_variables.endResetModel();
    }

    // Per-equation parameter values, in each expression's own order.
    m_parameterValues.clear();
    for (const auto& equation : m_equations.m_equations)
    {
        std::vector<double> values;
        for (const QString& name : equation.expression.parameters())
        {
            auto it = std::find_if(variables.begin(), variables.end(), [&name](const auto& v) { return v.name == name; });
            values.push_back(it != variables.end() ? it->value : 1.0);
        }
        m_parameterValues.push_back(std::move(values));
    }
}

void GraphingController::setVariableValue(int row, double value)
{
    auto& variables = m_variables.m_variables;
    if (row < 0 || static_cast<size_t>(row) >= variables.size() || !std::isfinite(value))
    {
        return;
    }
    // Port of VariableViewModel.Value: widen the slider range to include the value.
    auto& variable = variables[static_cast<size_t>(row)];
    variable.min = std::min(variable.min, value);
    variable.max = std::max(variable.max, value);
    variable.value = value;
    const QModelIndex index = m_variables.index(row);
    Q_EMIT m_variables.dataChanged(index, index);
    updateVariables();
    Q_EMIT graphChanged();
}

void GraphingController::setVariableMin(int row, double value)
{
    auto& variables = m_variables.m_variables;
    if (row < 0 || static_cast<size_t>(row) >= variables.size() || !std::isfinite(value))
    {
        return;
    }
    auto& variable = variables[static_cast<size_t>(row)];
    if (value >= variable.max)
    {
        variable.max = value + DefaultMinMaxRange;
    }
    variable.min = value;
    variable.value = std::clamp(variable.value, variable.min, variable.max);
    const QModelIndex index = m_variables.index(row);
    Q_EMIT m_variables.dataChanged(index, index);
    updateVariables();
    Q_EMIT graphChanged();
}

void GraphingController::setVariableMax(int row, double value)
{
    auto& variables = m_variables.m_variables;
    if (row < 0 || static_cast<size_t>(row) >= variables.size() || !std::isfinite(value))
    {
        return;
    }
    auto& variable = variables[static_cast<size_t>(row)];
    if (value <= variable.min)
    {
        variable.min = value - DefaultMinMaxRange;
    }
    variable.max = value;
    variable.value = std::clamp(variable.value, variable.min, variable.max);
    const QModelIndex index = m_variables.index(row);
    Q_EMIT m_variables.dataChanged(index, index);
    updateVariables();
    Q_EMIT graphChanged();
}

void GraphingController::setVariableStep(int row, double value)
{
    auto& variables = m_variables.m_variables;
    if (row < 0 || static_cast<size_t>(row) >= variables.size() || !std::isfinite(value) || value <= 0)
    {
        return;
    }
    variables[static_cast<size_t>(row)].step = value;
    const QModelIndex index = m_variables.index(row);
    Q_EMIT m_variables.dataChanged(index, index, { VariableModel::StepRole });
}

double GraphingController::evaluate(int row, double x) const
{
    const auto& equations = m_equations.m_equations;
    if (row < 0 || static_cast<size_t>(row) >= equations.size() || static_cast<size_t>(row) >= m_parameterValues.size())
    {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return equations[static_cast<size_t>(row)].expression.evaluate(x, m_parameterValues[static_cast<size_t>(row)], m_angleUnit);
}

std::vector<int> GraphingController::drawableRows() const
{
    std::vector<int> rows;
    const auto& equations = m_equations.m_equations;
    for (size_t i = 0; i < equations.size(); ++i)
    {
        if (equations[i].shown && equations[i].expression.isValid())
        {
            rows.push_back(static_cast<int>(i));
        }
    }
    return rows;
}

void GraphingController::save() const
{
    if (!m_persist)
    {
        return;
    }
    QStringList texts;
    for (const auto& equation : m_equations.m_equations)
    {
        if (!equation.text.trimmed().isEmpty())
        {
            texts.append(equation.text);
        }
    }
    QSettings settings;
    settings.setValue(QStringLiteral("graphing/equations"), texts);
    settings.setValue(QStringLiteral("graphing/angleUnit"), int(m_angleUnit));
}

void GraphingController::load()
{
    QSettings settings;
    m_angleUnit = static_cast<GraphExpression::AngleUnit>(
        std::clamp(settings.value(QStringLiteral("graphing/angleUnit"), int(GraphExpression::AngleUnit::Radians)).toInt(), 0, 2));
    const QStringList texts = settings.value(QStringLiteral("graphing/equations")).toStringList();
    for (const QString& text : texts.mid(0, MaxEquations))
    {
        EquationModel::Equation equation;
        equation.text = text;
        equation.expression = GraphExpression::parse(text);
        equation.error = errorText(equation.expression.error());
        equation.colorIndex = static_cast<int>(m_equations.m_equations.size());
        m_equations.m_equations.push_back(std::move(equation));
    }
}
