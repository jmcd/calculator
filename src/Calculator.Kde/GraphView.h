// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "GraphingController.h"

#include <QColor>
#include <QPointF>
#include <QPointer>
#include <QQuickPaintedItem>
#include <QtQml/qqmlregistration.h>

// Draws the equations of a GraphingController: grid, axes and curves, with
// drag to pan, wheel/pinch to zoom, and a trace point that follows the curve
// nearest the pointer. Replaces the Windows app's DirectX graph control.
class GraphView : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(GraphingController* controller READ controller WRITE setController NOTIFY controllerChanged)
    Q_PROPERTY(double xMin READ xMin NOTIFY viewChanged)
    Q_PROPERTY(double xMax READ xMax NOTIFY viewChanged)
    Q_PROPERTY(double yMin READ yMin NOTIFY viewChanged)
    Q_PROPERTY(double yMax READ yMax NOTIFY viewChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY colorsChanged)
    Q_PROPERTY(bool darkBackground READ darkBackground NOTIFY colorsChanged)
    Q_PROPERTY(double lineWidth READ lineWidth WRITE setLineWidth NOTIFY lineWidthChanged)
    Q_PROPERTY(bool tracing READ tracing NOTIFY traceChanged)
    Q_PROPERTY(QString traceText READ traceText NOTIFY traceChanged)

public:
    explicit GraphView(QQuickItem* parent = nullptr);

    GraphingController* controller() const
    {
        return m_controller;
    }
    void setController(GraphingController* controller);

    double xMin() const;
    double xMax() const;
    double yMin() const;
    double yMax() const;

    QColor backgroundColor() const
    {
        return m_background;
    }
    void setBackgroundColor(const QColor& color);
    QColor textColor() const
    {
        return m_text;
    }
    void setTextColor(const QColor& color);
    bool darkBackground() const
    {
        return m_background.lightnessF() < 0.5;
    }
    double lineWidth() const
    {
        return m_lineWidth;
    }
    void setLineWidth(double width);
    bool tracing() const
    {
        return m_traceRow >= 0;
    }
    QString traceText() const;

    // Shows x and y in the given ranges. Returns false if a range is empty.
    Q_INVOKABLE bool setWindow(double xMin, double xMax, double yMin, double yMax);
    // x from -10 to 10, centered on the origin, with square units.
    Q_INVOKABLE void resetView();
    // factor < 1 zooms in. Zooms about the center, or about (x, y) in pixels.
    Q_INVOKABLE void zoom(double factor);
    Q_INVOKABLE void zoomAt(double factor, double x, double y);
    // Moves the view by a number of pixels.
    Q_INVOKABLE void pan(double dx, double dy);

    void paint(QPainter* painter) override;

    // Coordinate conversion (public for tests).
    double toPixelX(double x) const;
    double toPixelY(double y) const;
    double fromPixelX(double px) const;
    double fromPixelY(double py) const;

Q_SIGNALS:
    void controllerChanged();
    void viewChanged();
    void colorsChanged();
    void lineWidthChanged();
    void traceChanged();

protected:
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void hoverMoveEvent(QHoverEvent* event) override;
    void hoverLeaveEvent(QHoverEvent* event) override;

private:
    void updateTrace(QPointF position);
    void clearTrace();
    void onGraphChanged();
    void drawGrid(QPainter* painter) const;
    void drawCurve(QPainter* painter, int row) const;
    QString formatNumber(double value, double step) const;

    QPointer<GraphingController> m_controller;

    // The view: center and units per pixel on each axis.
    double m_centerX = 0;
    double m_centerY = 0;
    double m_scaleX = 0;
    double m_scaleY = 0;
    bool m_initialized = false;

    QColor m_background = Qt::white;
    QColor m_text = Qt::black;
    double m_lineWidth = 2;

    bool m_dragging = false;
    QPointF m_lastDragPosition;
    QPointF m_pointer{ -1, -1 };

    int m_traceRow = -1;
    double m_traceX = 0;
    double m_traceY = 0;
};
