// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "GraphView.h"
#include "GraphingController.h"

#include <QCursor>
#include <QFontMetricsF>
#include <QLocale>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>
#include <cmath>

namespace
{
    constexpr double DefaultHalfWidth = 10; // x from -10 to 10
    constexpr double MinScale = 1e-12;
    constexpr double MaxScale = 1e12;
    constexpr double TraceSnapDistance = 40; // pixels
    constexpr double TargetGridSpacing = 80; // pixels between major grid lines

    // A "nice" step (1, 2 or 5 × 10ⁿ) near 'raw'.
    double NiceStep(double raw)
    {
        const double magnitude = std::pow(10.0, std::floor(std::log10(raw)));
        const double normalized = raw / magnitude;
        if (normalized < 1.5)
        {
            return magnitude;
        }
        if (normalized < 3.5)
        {
            return 2 * magnitude;
        }
        if (normalized < 7.5)
        {
            return 5 * magnitude;
        }
        return 10 * magnitude;
    }
}

GraphView::GraphView(QQuickItem* parent)
    : QQuickPaintedItem(parent)
{
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
    setAntialiasing(true);
    setFlag(ItemIsFocusScope, false);
}

void GraphView::setController(GraphingController* controller)
{
    if (m_controller == controller)
    {
        return;
    }
    if (m_controller)
    {
        disconnect(m_controller, nullptr, this, nullptr);
    }
    m_controller = controller;
    if (m_controller)
    {
        connect(m_controller, &GraphingController::graphChanged, this, &GraphView::onGraphChanged);
    }
    Q_EMIT controllerChanged();
    update();
}

void GraphView::onGraphChanged()
{
    if (tracing())
    {
        updateTrace(m_pointer);
    }
    update();
}

// ---------------------------------------------------------------------------
// View
// ---------------------------------------------------------------------------

double GraphView::xMin() const
{
    return m_centerX - width() / 2 * m_scaleX;
}
double GraphView::xMax() const
{
    return m_centerX + width() / 2 * m_scaleX;
}
double GraphView::yMin() const
{
    return m_centerY - height() / 2 * m_scaleY;
}
double GraphView::yMax() const
{
    return m_centerY + height() / 2 * m_scaleY;
}

double GraphView::toPixelX(double x) const
{
    return width() / 2 + (x - m_centerX) / m_scaleX;
}
double GraphView::toPixelY(double y) const
{
    return height() / 2 - (y - m_centerY) / m_scaleY;
}
double GraphView::fromPixelX(double px) const
{
    return m_centerX + (px - width() / 2) * m_scaleX;
}
double GraphView::fromPixelY(double py) const
{
    return m_centerY - (py - height() / 2) * m_scaleY;
}

void GraphView::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    if (!m_initialized && newGeometry.width() > 0 && newGeometry.height() > 0)
    {
        resetView();
        return;
    }
    // Keep the center and scale; more or less of the plane becomes visible.
    if (newGeometry.size() != oldGeometry.size())
    {
        clearTrace();
        Q_EMIT viewChanged();
    }
}

void GraphView::resetView()
{
    if (width() <= 0 || height() <= 0)
    {
        return;
    }
    m_initialized = true;
    m_centerX = 0;
    m_centerY = 0;
    m_scaleX = 2 * DefaultHalfWidth / width();
    m_scaleY = m_scaleX;
    clearTrace();
    Q_EMIT viewChanged();
    update();
}

bool GraphView::setWindow(double x0, double x1, double y0, double y1)
{
    if (!(x1 > x0) || !(y1 > y0) || width() <= 0 || height() <= 0)
    {
        return false;
    }
    m_initialized = true;
    m_centerX = (x0 + x1) / 2;
    m_centerY = (y0 + y1) / 2;
    m_scaleX = std::clamp((x1 - x0) / width(), MinScale, MaxScale);
    m_scaleY = std::clamp((y1 - y0) / height(), MinScale, MaxScale);
    clearTrace();
    Q_EMIT viewChanged();
    update();
    return true;
}

void GraphView::zoom(double factor)
{
    zoomAt(factor, width() / 2, height() / 2);
}

void GraphView::zoomAt(double factor, double px, double py)
{
    if (!(factor > 0))
    {
        return;
    }
    // Keep the point under (px, py) fixed.
    const double x = fromPixelX(px);
    const double y = fromPixelY(py);
    m_scaleX = std::clamp(m_scaleX * factor, MinScale, MaxScale);
    m_scaleY = std::clamp(m_scaleY * factor, MinScale, MaxScale);
    m_centerX = x - (px - width() / 2) * m_scaleX;
    m_centerY = y + (py - height() / 2) * m_scaleY;
    if (tracing())
    {
        updateTrace(m_pointer);
    }
    Q_EMIT viewChanged();
    update();
}

void GraphView::pan(double dx, double dy)
{
    m_centerX -= dx * m_scaleX;
    m_centerY += dy * m_scaleY;
    Q_EMIT viewChanged();
    update();
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void GraphView::mousePressEvent(QMouseEvent* event)
{
    m_dragging = true;
    m_lastDragPosition = event->position();
    clearTrace();
    setCursor(Qt::ClosedHandCursor);
    event->accept();
}

void GraphView::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_dragging)
    {
        return;
    }
    const QPointF delta = event->position() - m_lastDragPosition;
    m_lastDragPosition = event->position();
    pan(delta.x(), delta.y());
    event->accept();
}

void GraphView::mouseReleaseEvent(QMouseEvent* event)
{
    m_dragging = false;
    unsetCursor();
    m_pointer = event->position();
    updateTrace(m_pointer);
    event->accept();
}

void GraphView::wheelEvent(QWheelEvent* event)
{
    const double notches = event->angleDelta().y() / 120.0;
    if (notches != 0)
    {
        zoomAt(std::pow(0.85, notches), event->position().x(), event->position().y());
    }
    event->accept();
}

void GraphView::hoverMoveEvent(QHoverEvent* event)
{
    m_pointer = event->position();
    if (!m_dragging)
    {
        updateTrace(m_pointer);
    }
}

void GraphView::hoverLeaveEvent(QHoverEvent*)
{
    m_pointer = { -1, -1 };
    clearTrace();
}

void GraphView::updateTrace(QPointF position)
{
    const int previous = m_traceRow;
    m_traceRow = -1;
    if (m_controller && position.x() >= 0 && position.y() >= 0 && position.x() <= width() && position.y() <= height())
    {
        const double x = fromPixelX(position.x());
        double best = TraceSnapDistance;
        for (int row : m_controller->drawableRows())
        {
            const double y = m_controller->evaluate(row, x);
            if (!std::isfinite(y))
            {
                continue;
            }
            const double distance = std::abs(toPixelY(y) - position.y());
            if (distance < best)
            {
                best = distance;
                m_traceRow = row;
                m_traceX = x;
                m_traceY = y;
            }
        }
    }
    if (m_traceRow >= 0 || previous >= 0)
    {
        Q_EMIT traceChanged();
        update();
    }
}

void GraphView::clearTrace()
{
    if (m_traceRow >= 0)
    {
        m_traceRow = -1;
        Q_EMIT traceChanged();
        update();
    }
}

QString GraphView::traceText() const
{
    if (!tracing())
    {
        return {};
    }
    // Enough decimals to distinguish neighboring pixels.
    const double step = std::min(m_scaleX, m_scaleY);
    return QStringLiteral("(%1, %2)").arg(formatNumber(m_traceX, step), formatNumber(m_traceY, step));
}

// ---------------------------------------------------------------------------
// Appearance
// ---------------------------------------------------------------------------

void GraphView::setBackgroundColor(const QColor& color)
{
    if (m_background != color)
    {
        m_background = color;
        Q_EMIT colorsChanged();
        update();
    }
}

void GraphView::setTextColor(const QColor& color)
{
    if (m_text != color)
    {
        m_text = color;
        Q_EMIT colorsChanged();
        update();
    }
}

void GraphView::setLineWidth(double width)
{
    width = std::clamp(width, 0.5, 8.0);
    if (m_lineWidth != width)
    {
        m_lineWidth = width;
        Q_EMIT lineWidthChanged();
        update();
    }
}

QString GraphView::formatNumber(double value, double step) const
{
    const int decimals = std::clamp(static_cast<int>(-std::floor(std::log10(step))), 0, 12);
    if (std::abs(value) >= 1e7 || (std::abs(value) < 1e-5 && value != 0 && decimals > 6))
    {
        return QLocale().toString(value, 'g', 4);
    }
    QString text = QLocale().toString(std::abs(value) < step / 2 ? 0.0 : value, 'f', decimals);
    return text;
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void GraphView::paint(QPainter* painter)
{
    if (!m_initialized)
    {
        return;
    }
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->fillRect(boundingRect(), m_background);

    drawGrid(painter);

    if (m_controller)
    {
        for (int row : m_controller->drawableRows())
        {
            drawCurve(painter, row);
        }
    }

    if (tracing())
    {
        const QColor color = GraphingController::equationColor(m_controller->equations()->equations()[static_cast<size_t>(m_traceRow)].colorIndex, darkBackground());
        const QPointF point(toPixelX(m_traceX), toPixelY(m_traceY));

        painter->setPen(QPen(m_background, 2));
        painter->setBrush(color);
        painter->drawEllipse(point, 5, 5);

        const QString text = traceText();
        const QFontMetricsF metrics(painter->font());
        QRectF box = metrics.boundingRect(text).adjusted(-6, -3, 6, 3);
        box.moveBottomLeft(point + QPointF(10, -10));
        // Keep the label inside the view.
        if (box.right() > width())
        {
            box.moveRight(point.x() - 10);
        }
        if (box.top() < 0)
        {
            box.moveTop(point.y() + 10);
        }
        QColor boxColor = m_background;
        boxColor.setAlphaF(0.9);
        painter->setPen(QPen(color, 1));
        painter->setBrush(boxColor);
        painter->drawRoundedRect(box, 4, 4);
        painter->setPen(m_text);
        painter->drawText(box, Qt::AlignCenter, text);
    }
}

void GraphView::drawGrid(QPainter* painter) const
{
    const double w = width();
    const double h = height();
    const double stepX = NiceStep(TargetGridSpacing * m_scaleX);
    const double stepY = NiceStep(TargetGridSpacing * m_scaleY);

    QColor minor = m_text;
    minor.setAlphaF(0.07);
    QColor major = m_text;
    major.setAlphaF(0.18);
    QColor axis = m_text;
    axis.setAlphaF(0.75);

    auto lines = [&](double gridStep, const QColor& color, bool vertical) {
        painter->setPen(QPen(color, 1));
        const double lo = vertical ? xMin() : yMin();
        const double hi = vertical ? xMax() : yMax();
        for (double v = std::ceil(lo / gridStep) * gridStep; v <= hi; v += gridStep)
        {
            if (vertical)
            {
                const double px = std::round(toPixelX(v)) + 0.5;
                painter->drawLine(QPointF(px, 0), QPointF(px, h));
            }
            else
            {
                const double py = std::round(toPixelY(v)) + 0.5;
                painter->drawLine(QPointF(0, py), QPointF(w, py));
            }
        }
    };
    lines(stepX / 5, minor, true);
    lines(stepY / 5, minor, false);
    lines(stepX, major, true);
    lines(stepY, major, false);

    // Axes, or the edge of the view when an axis is off-screen.
    const double axisX = std::round(std::clamp(toPixelY(0), 0.0, h - 1)) + 0.5; // the x-axis' pixel row
    const double axisY = std::round(std::clamp(toPixelX(0), 0.0, w - 1)) + 0.5; // the y-axis' pixel column
    painter->setPen(QPen(axis, 1.5));
    if (toPixelY(0) >= 0 && toPixelY(0) <= h)
    {
        painter->drawLine(QPointF(0, axisX), QPointF(w, axisX));
    }
    if (toPixelX(0) >= 0 && toPixelX(0) <= w)
    {
        painter->drawLine(QPointF(axisY, 0), QPointF(axisY, h));
    }

    // Labels.
    QFont font = painter->font();
    font.setPointSizeF(font.pointSizeF() * 0.85);
    painter->setFont(font);
    const QFontMetricsF metrics(font);
    QColor label = m_text;
    label.setAlphaF(0.8);
    painter->setPen(label);

    const bool labelsBelow = axisX + metrics.height() + 4 < h;
    for (double x = std::ceil(xMin() / stepX) * stepX; x <= xMax(); x += stepX)
    {
        if (std::abs(x) < stepX / 2)
        {
            continue;
        }
        const QString text = formatNumber(x, stepX);
        const double px = toPixelX(x);
        const double textWidth = metrics.horizontalAdvance(text);
        const double y = labelsBelow ? axisX + 3 + metrics.ascent() : axisX - 4;
        // Keep labels at the edges fully visible.
        const double left = std::clamp(px - textWidth / 2, 2.0, w - textWidth - 2);
        painter->drawText(QPointF(left, y), text);
    }

    for (double y = std::ceil(yMin() / stepY) * stepY; y <= yMax(); y += stepY)
    {
        if (std::abs(y) < stepY / 2)
        {
            continue;
        }
        const QString text = formatNumber(y, stepY);
        const double textWidth = metrics.horizontalAdvance(text);
        const double py = toPixelY(y) + metrics.ascent() / 2 - 1;
        const double px = axisY - textWidth - 5 >= 0 ? axisY - textWidth - 5 : axisY + 5;
        painter->drawText(QPointF(px, py), text);
    }

    // Origin.
    if (toPixelX(0) >= 0 && toPixelX(0) <= w && toPixelY(0) >= 0 && toPixelY(0) <= h)
    {
        const QString zero = QLocale().toString(0);
        painter->drawText(QPointF(axisY - metrics.horizontalAdvance(zero) - 5, axisX + 3 + metrics.ascent()), zero);
    }
}

void GraphView::drawCurve(QPainter* painter, int row) const
{
    const auto& equation = m_controller->equations()->equations()[static_cast<size_t>(row)];
    QPen pen(GraphingController::equationColor(equation.colorIndex, darkBackground()), m_lineWidth);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    const double w = width();
    const double h = height();
    // Points far outside the view are clamped so painting stays fast and stable.
    const double limit = 4 * h;
    auto clampY = [&](double py) { return std::clamp(py, -limit, h + limit); };
    auto f = [&](double x) { return m_controller->evaluate(row, x); };

    // Finds where the function stops being defined between a (defined) and b (not).
    auto domainEdge = [&](double a, double b) {
        for (int i = 0; i < 24; ++i)
        {
            const double mid = (a + b) / 2;
            if (std::isfinite(f(mid)))
            {
                a = mid;
            }
            else
            {
                b = mid;
            }
        }
        return a;
    };

    QPainterPath path;
    bool penDown = false;
    constexpr double stepPixels = 0.5;
    double previousX = fromPixelX(-stepPixels);
    double previousY = f(previousX);

    for (double px = 0; px <= w + stepPixels; px += stepPixels)
    {
        const double x = fromPixelX(px);
        const double y = f(x);
        const bool defined = std::isfinite(y);
        const bool previousDefined = std::isfinite(previousY);

        if (defined && !previousDefined)
        {
            // Entering the domain: start exactly at its edge.
            const double edge = domainEdge(x, previousX);
            path.moveTo(toPixelX(edge), clampY(toPixelY(f(edge))));
            penDown = true;
        }
        else if (!defined && previousDefined)
        {
            // Leaving the domain: end exactly at its edge.
            const double edge = domainEdge(previousX, x);
            if (penDown)
            {
                path.lineTo(toPixelX(edge), clampY(toPixelY(f(edge))));
            }
            penDown = false;
        }

        if (defined)
        {
            const double py = toPixelY(y);
            if (!penDown)
            {
                path.moveTo(px, clampY(py));
                penDown = true;
            }
            else
            {
                const double previousPy = toPixelY(previousY);
                // A big jump might be an asymptote (tan x, 1/x): if the midpoint
                // isn't between the two values, the curve isn't continuous here.
                bool broken = false;
                if (std::abs(py - previousPy) > h / 2)
                {
                    const double mid = f((x + previousX) / 2);
                    const double lo = std::min(y, previousY);
                    const double hi = std::max(y, previousY);
                    broken = !std::isfinite(mid) || mid < lo || mid > hi;
                }
                if (broken)
                {
                    path.moveTo(px, clampY(py));
                }
                else
                {
                    path.lineTo(px, clampY(py));
                }
            }
        }

        previousX = x;
        previousY = y;
    }

    painter->drawPath(path);
}
