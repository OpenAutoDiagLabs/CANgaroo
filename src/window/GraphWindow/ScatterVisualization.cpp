/*

  Copyright (c) 2026 Antigravity AI

  This file is part of cangaroo.

*/

#include "ScatterVisualization.h"
#include <QVBoxLayout>
#include <float.h>
#include <QMouseEvent>
#include <QEvent>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGraphicsDropShadowEffect>
#include <QPen>
#include <QBrush>
#include <QDateTime>
#include <QTimer>

ScatterVisualization::ScatterVisualization(QWidget *parent, Backend &backend)
    : VisualizationWidget(parent, backend), _windowDuration(0), _autoScroll(true), _isUpdatingRange(false)
{
    _updateTimer = new QTimer(this);
    connect(_updateTimer, &QTimer::timeout, this, &ScatterVisualization::onActivated);
    _updateTimer->start(100); // 10Hz sync
    _chart = new QChart();
    _chart->legend()->setVisible(true);
    _chart->legend()->setAlignment(Qt::AlignBottom);
    _chart->setTitle("Scatter (Distribution View)");

    _chartView = new QChartView(_chart);
    _chartView->setRenderHint(QPainter::Antialiasing);
    _chartView->setRubberBand(QChartView::HorizontalRubberBand);
    _chartView->setMouseTracking(true);
    _chartView->viewport()->setMouseTracking(true);
    _chartView->viewport()->installEventFilter(this);
    _chartView->installEventFilter(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(_chartView);
    layout->setContentsMargins(0, 0, 0, 0);

    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText("Time [s]");
    _chart->addAxis(axisX, Qt::AlignBottom);
    connect(axisX, &QValueAxis::rangeChanged, this, &ScatterVisualization::updateAxes);
    connect(axisX, &QValueAxis::rangeChanged, this, &ScatterVisualization::onAxisRangeChanged);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("Value");
    _chart->addAxis(axisY, Qt::AlignLeft);

    // Create cursor line
    _cursorLine = new QGraphicsLineItem(_chart);
    QPen cursorPen(palette().color(QPalette::WindowText), 1, Qt::DashLine);
    _cursorLine->setPen(cursorPen);
    _cursorLine->setZValue(1000);
    _cursorLine->hide();

    // Create Tooltip Box
    _tooltipBox = new QGraphicsRectItem(_chart);
    _tooltipBox->setBrush(QBrush(palette().color(QPalette::ToolTipBase)));
    _tooltipBox->setPen(QPen(palette().color(QPalette::ToolTipText), 1));
    _tooltipBox->setZValue(2000);
    _tooltipBox->hide();

    _tooltipText = new QGraphicsTextItem(_tooltipBox);
    _tooltipText->setDefaultTextColor(palette().color(QPalette::ToolTipText));
    _tooltipText->setTextInteractionFlags(Qt::NoTextInteraction);
    _tooltipText->setZValue(2001);

    // Add shadow effect to tooltip box
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(10);
    shadow->setOffset(3, 3);
    shadow->setColor(QColor(0, 0, 0, 80));
    _tooltipBox->setGraphicsEffect(shadow);

    setMouseTracking(true);
}

ScatterVisualization::~ScatterVisualization()
{
}

void ScatterVisualization::addMessage(const CanMessage &msg)
{
    double timestamp = msg.getFloatTimestamp();

    if (_startTime < 0) {
        setGlobalStartTime(timestamp);
    }

    double t = timestamp - _startTime;

    for (CanDbSignal *signal : _signals) {
        if (signal->isPresentInMessage(msg)) {
            double value = signal->extractPhysicalFromMessage(msg);
            if (_seriesMap.contains(signal)) {
                QScatterSeries *series = _seriesMap[signal];
                series->append(t, value);
                _signalBusMap[signal] = msg.getInterfaceId();
                
                // Rolling buffer: History preserved for "All" view. Only prune if memory safety is at risk.
                if (series->count() > MAX_POINTS) {
                    series->remove(0);
                }
                // No pruning in "All" mode - let the user see everything!
            }
        }
    }

    if (_autoScroll && !_chart->axes(Qt::Horizontal).isEmpty()) {
        _isUpdatingRange = true;
        QAbstractAxis *axisX = _chart->axes(Qt::Horizontal).first();
        if (_windowDuration > 0) {
            double windowSize = (double)_windowDuration;
            if (t > windowSize) {
                // strict sliding: Tail moves with Head
                axisX->setRange(t - windowSize, t);
            } else {
                // Startup Phase: Stay pinned at 0 to Duration until filled
                axisX->setRange(0, windowSize);
            }
        } else {
            // "All" View: Expand from 0 to current time
            axisX->setRange(0, qMax(10.0, t));
        }
        _isUpdatingRange = false;
    }

    updateAxes();
}

void ScatterVisualization::onActivated()
{
    if (!_autoScroll || _chart->axes(Qt::Horizontal).isEmpty()) return;
    
    // Find latest message time for comparison
    double latestMsgT = 0;
    for (auto series : _seriesMap.values()) {
        if (series->count() > 0) {
            latestMsgT = qMax(latestMsgT, series->at(series->count() - 1).x());
        }
    }
    
    // Use the latest message time for strict data-driven sliding
    double t = latestMsgT;

    _isUpdatingRange = true;
    QAbstractAxis *axisX = _chart->axes(Qt::Horizontal).first();
    if (_windowDuration > 0) {
        double windowSize = (double)_windowDuration;
        if (t > windowSize) {
            axisX->setRange(t - windowSize, t);
        } else {
            axisX->setRange(0, windowSize);
        }
    } else {
        // "All" Mode: Show from 0 to current timestamp
        axisX->setRange(0, qMax(10.0, t));
    }
    _isUpdatingRange = false;
    
    updateAxes(); // Trigger Y auto-fit
}

void ScatterVisualization::setSignalColor(CanDbSignal *signal, const QColor &color)
{
    VisualizationWidget::setSignalColor(signal, color);
    if (_seriesMap.contains(signal)) {
        _seriesMap[signal]->setColor(color);
        _seriesMap[signal]->setBrush(QBrush(color));
    }
    if (_tracers.contains(signal)) {
        _tracers[signal]->setBrush(color);
    }
}

void ScatterVisualization::updateAxes()
{
    if (_chart->axes(Qt::Vertical).isEmpty() || _seriesMap.isEmpty()) return;

    double minY = DBL_MAX;
    double maxY = -DBL_MAX;
    bool hasData = false;

    QValueAxis *axisX = qobject_cast<QValueAxis*>(_chart->axes(Qt::Horizontal).first());
    double minX = axisX->min();
    double maxX = axisX->max();

    for (auto series : _seriesMap.values()) {
        for (const QPointF &p : series->points()) {
            if (p.x() >= minX && p.x() <= maxX) {
                minY = qMin(minY, (double)p.y());
                maxY = qMax(maxY, (double)p.y());
                hasData = true;
            }
        }
    }

    if (hasData) {
        double range = maxY - minY;
        if (range < 0.1) {
            minY -= 0.5;
            maxY += 0.5;
        } else {
            minY -= range * 0.1;
            maxY += range * 0.1;
        }
        _chart->axes(Qt::Vertical).first()->setRange(minY, maxY);
    }
}

void ScatterVisualization::clear()
{
    for (auto series : _seriesMap.values()) {
        series->clear();
    }
    _startTime = -1;
}

void ScatterVisualization::clearSignals()
{
    for (auto series : _seriesMap.values()) {
        _chart->removeSeries(series);
        delete series;
    }
    for (auto tracer : _tracers.values()) {
        delete tracer;
    }
    _tracers.clear();
    _seriesMap.clear();
    _signals.clear();
    _signalBusMap.clear();
    _startTime = -1;
}

void ScatterVisualization::wheelEvent(QWheelEvent *event)
{
    _autoScroll = false;
    QValueAxis *axisX = qobject_cast<QValueAxis*>(_chart->axes(Qt::Horizontal).first());
    if (axisX) {
        double min = axisX->min();
        double max = axisX->max();
        double center = min + (max - min) * 0.5;
        double factor = (event->angleDelta().y() > 0) ? 0.8 : 1.2;
        double newRange = (max - min) * factor;
        _isUpdatingRange = true;
        axisX->setRange(center - newRange * 0.5, center + newRange * 0.5);
        _isUpdatingRange = false;
    }
    event->accept();
}

bool ScatterVisualization::eventFilter(QObject *watched, QEvent *event)
{
    if ((watched == _chartView || watched == _chartView->viewport()) && event->type() == QEvent::MouseMove) {
        emit mouseMoved(static_cast<QMouseEvent*>(event));
    }
    return VisualizationWidget::eventFilter(watched, event);
}

void ScatterVisualization::zoomIn()
{
    _autoScroll = false;
    QValueAxis *axisX = qobject_cast<QValueAxis*>(_chart->axes(Qt::Horizontal).first());
    if (axisX) {
        double min = axisX->min();
        double max = axisX->max();
        double newRange = (max - min) * 0.8;
        double center = min + (max - min) * 0.5;
        _isUpdatingRange = true;
        axisX->setRange(center - newRange * 0.5, center + newRange * 0.5);
        _isUpdatingRange = false;
    }
}

void ScatterVisualization::zoomOut()
{
    _autoScroll = false;
    QValueAxis *axisX = qobject_cast<QValueAxis*>(_chart->axes(Qt::Horizontal).first());
    if (axisX) {
        double min = axisX->min();
        double max = axisX->max();
        double newRange = (max - min) * 1.2;
        double center = min + (max - min) * 0.5;
        _isUpdatingRange = true;
        axisX->setRange(center - newRange * 0.5, center + newRange * 0.5);
        _isUpdatingRange = false;
    }
}

void ScatterVisualization::resetZoom()
{
    _autoScroll = true;
}

void ScatterVisualization::setWindowDuration(int seconds)
{
    _windowDuration = seconds;
    _autoScroll = true;
}

void ScatterVisualization::addSignal(CanDbSignal *signal)
{
    if (_seriesMap.contains(signal)) return;

    VisualizationWidget::addSignal(signal);

    QScatterSeries *series = new QScatterSeries();
    series->setName(signal->name());
    series->setMarkerShape(QScatterSeries::MarkerShapeCircle);
    series->setMarkerSize(7.2);
    
    // Assign a color from a list if chart doesn't provide a distinct one yet
    static const QList<Qt::GlobalColor> palette = { Qt::red, Qt::blue, Qt::green, Qt::magenta, Qt::darkYellow, Qt::cyan };
    QColor color = palette[_seriesMap.size() % palette.size()];
    series->setColor(color);
    
    _chart->addSeries(series);
    
    // Explicitly set color to ensure brush is visible
    series->setBrush(QBrush(color));
    series->setPen(QPen(Qt::NoPen)); // Clean scatter look
    
    if (!_chart->axes(Qt::Horizontal).isEmpty()) {
        series->attachAxis(_chart->axes(Qt::Horizontal).first());
    }
    if (!_chart->axes(Qt::Vertical).isEmpty()) {
        series->attachAxis(_chart->axes(Qt::Vertical).first());
    }

    _seriesMap[signal] = series;

    QGraphicsEllipseItem *tracer = new QGraphicsEllipseItem(-4, -4, 8, 8, _chart);
    tracer->setBrush(series->color());
    tracer->setPen(QPen(Qt::white, 1));
    tracer->setZValue(1500);
    tracer->hide();
    _tracers[signal] = tracer;
}

void ScatterVisualization::onAxisRangeChanged(qreal min, qreal max)
{
    Q_UNUSED(min);
    Q_UNUSED(max);
    if (!_isUpdatingRange) {
        _autoScroll = false;
    }
}
