// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "GraphExpression.h"

#include <QAbstractListModel>
#include <QColor>
#include <QObject>
#include <QtQml/qqmlregistration.h>

#include <vector>

class GraphingController;

// The equations being graphed. There is always one empty row at the end to
// type a new equation into, as in the Windows app (up to 14 equations).
class EquationModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ANONYMOUS

public:
    enum Role
    {
        TextRole = Qt::UserRole + 1,
        ColorIndexRole,
        ShownRole,
        ErrorRole,
        ValidRole,
    };

    explicit EquationModel(GraphingController* controller);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    struct Equation
    {
        QString text;
        GraphExpression expression;
        int colorIndex = 0;
        bool shown = true;
        QString error;
    };
    const std::vector<Equation>& equations() const
    {
        return m_equations;
    }

private:
    friend class GraphingController;
    GraphingController* m_controller;
    std::vector<Equation> m_equations;
};

// Parameters found in the equations (a, b, k...), each with a slider.
class VariableModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ANONYMOUS

public:
    enum Role
    {
        NameRole = Qt::UserRole + 1,
        ValueRole,
        MinRole,
        MaxRole,
        StepRole,
    };

    explicit VariableModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    struct Variable
    {
        QString name;
        double value = 1;
        double min = -10;
        double max = 10;
        double step = 0.1;
    };
    const std::vector<Variable>& variables() const
    {
        return m_variables;
    }

private:
    friend class GraphingController;
    std::vector<Variable> m_variables;
};

// Plays the role of GraphingCalculatorViewModel in the Windows app: owns the
// equations and variables that GraphView draws.
class GraphingController : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Graphing)
    QML_SINGLETON

    Q_PROPERTY(EquationModel* equations READ equations CONSTANT)
    Q_PROPERTY(VariableModel* variables READ variables CONSTANT)
    Q_PROPERTY(int angleUnit READ angleUnit WRITE setAngleUnit NOTIFY angleUnitChanged)

public:
    static constexpr int MaxEquations = 14;

    enum AngleUnit
    {
        Radians = int(GraphExpression::AngleUnit::Radians),
        Degrees = int(GraphExpression::AngleUnit::Degrees),
        Gradians = int(GraphExpression::AngleUnit::Gradians),
    };
    Q_ENUM(AngleUnit)

    explicit GraphingController(QObject* parent = nullptr);
    // For tests: doesn't load or save the user's equations.
    GraphingController(bool persist, QObject* parent);

    EquationModel* equations()
    {
        return &m_equations;
    }
    VariableModel* variables()
    {
        return &m_variables;
    }
    int angleUnit() const
    {
        return int(m_angleUnit);
    }
    void setAngleUnit(int unit);

    Q_INVOKABLE void setEquationText(int row, const QString& text);
    Q_INVOKABLE void setEquationShown(int row, bool shown);
    Q_INVOKABLE void removeEquation(int row);
    Q_INVOKABLE void setVariableValue(int row, double value);
    Q_INVOKABLE void setVariableMin(int row, double value);
    Q_INVOKABLE void setVariableMax(int row, double value);
    Q_INVOKABLE void setVariableStep(int row, double value);

    // Windows' equation colors, for light and dark backgrounds.
    Q_INVOKABLE static QColor equationColor(int colorIndex, bool darkBackground);

    // y for equation 'row' at x, or NaN. Used by GraphView.
    double evaluate(int row, double x) const;
    // Rows that should be drawn.
    std::vector<int> drawableRows() const;

Q_SIGNALS:
    void angleUnitChanged();
    // Emitted whenever the graph needs repainting.
    void graphChanged();

private:
    void ensureTrailingEmptyRow();
    void updateVariables();
    int nextColorIndex() const;
    void save() const;
    void load();
    static QString errorText(GraphExpression::Error error);

    EquationModel m_equations;
    VariableModel m_variables;
    GraphExpression::AngleUnit m_angleUnit = GraphExpression::AngleUnit::Radians;
    bool m_persist = true;
    // Parameter values for each equation, in the order of its parameters().
    std::vector<std::vector<double>> m_parameterValues;
};
