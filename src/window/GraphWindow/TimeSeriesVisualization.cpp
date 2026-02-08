/*

  Copyright (c) 2026 Antigravity AI

  This file is part of cangaroo.

*/

#include "TimeSeriesVisualization.h"
#include <QVBoxLayout>
#include <float.h>
#include <QToolTip>
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

TimeSeriesVisualization::TimeSeriesVisualization(QWidget *parent, Backend &backend)
    : VisualizationWidget(parent, backend), _windowDuration(0), _autoScroll(true), _isUpdatingRange(false)
{
    _updateTimer = new QTimer(this);
    connect(_updateTimer, &QTimer::timeout, this, &TimeSeriesVisualization::onActivated);
    _updateTimer->start(100);
    _chart = new QChart();
    _chart->legend()->setVisible(true);
    _chart->legend()->setAlignment(Qt::AlignBottom);
    _chart->setTitle("Time Series Graph");

    _chartView = new QChartView(_chart);
    _chartView->setRenderHint(QPainter::Antialiasing);
    _chartView->setRubberBand(QChartView::HorizontalRubberBand);
    _chartView->setMouseTracking(true);
    _chartView->viewport()->setMouseTracking(true); // Ensure viewport tracks mouse
    _chartView->viewport()->installEventFilter(this); // Catch events on viewport
    _chartView->installEventFilter(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(_chartView);
    layout->setContentsMargins(0, 0, 0, 0);

    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText("Time [s]");
    _chart->addAxis(axisX, Qt::AlignBottom);
    connect(axisX, &QValueAxis::rangeChanged, this, &TimeSeriesVisualization::updateYAxisRange);
    connect(axisX, &QValueAxis::rangeChanged, this, &TimeSeriesVisualization::onAxisRangeChanged);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("Value");
    _chart->addAxis(axisY, Qt::AlignLeft);
    
    // Create cursor line
    _cursorLine = new QGraphicsLineItem(_chart);
    QPen cursorPen(Qt::gray, 1, Qt::DashLine);
    _cursorLine->setPen(cursorPen);
    _cursorLine->setZValue(1000); // High Z-value to be on top
    _cursorLine->hide();

    // Create Tooltip Box
    _tooltipBox = new QGraphicsRectItem(_chart);
    _tooltipBox->setBrush(QBrush(Qt::white));
    _tooltipBox->setPen(QPen(Qt::lightGray, 1));
    _tooltipBox->setZValue(2000); // Very high Z-value
    _tooltipBox->hide();

    _tooltipText = new QGraphicsTextItem(_tooltipBox);
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

TimeSeriesVisualization::~TimeSeriesVisualization()
{
}

void TimeSeriesVisualization::addMessage(const CanMessage &msg)
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
                QLineSeries *series = _seriesMap[signal];
                series->append(t, value);
                _signalBusMap[signal] = msg.getInterfaceId();
                
                // Rolling buffer: History preserved for "All" view. Only prune if memory safety is at risk.
                if (series->count() > 100000) {
                    series->remove(0);
                }
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

    updateYAxisRange();
}

void TimeSeriesVisualization::onActivated()
{
    if (!_autoScroll || _chart->axes(Qt::Horizontal).isEmpty()) return;

    double latestMsgT = 0;
    for (auto series : _seriesMap.values()) {
        if (series->count() > 0) {
            latestMsgT = qMax(latestMsgT, series->at(series->count() - 1).x());
        }
    }
    
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
}

void TimeSeriesVisualization::setSignalColor(CanDbSignal *signal, const QColor &color)
{
    VisualizationWidget::setSignalColor(signal, color);
    if (_seriesMap.contains(signal)) {
        _seriesMap[signal]->setColor(color);
    }
    if (_tracers.contains(signal)) {
        _tracers[signal]->setBrush(color);
    }
}

void TimeSeriesVisualization::updateYAxisRange()
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

void TimeSeriesVisualization::clear()
{
    for (auto series : _seriesMap.values()) {
        series->clear();
    }
    _startTime = -1;
}

void TimeSeriesVisualization::clearSignals()
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

void TimeSeriesVisualization::wheelEvent(QWheelEvent *event)
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
    
    updateYAxisRange();
    event->accept();
}

bool TimeSeriesVisualization::eventFilter(QObject *watched, QEvent *event)
{
    if ((watched == _chartView || watched == _chartView->viewport()) && event->type() == QEvent::MouseMove) {
        emit mouseMoved(static_cast<QMouseEvent*>(event));
    }
    return VisualizationWidget::eventFilter(watched, event);
}

void TimeSeriesVisualization::zoomIn()
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
    updateYAxisRange();
}

void TimeSeriesVisualization::zoomOut()
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
    updateYAxisRange();
}

void TimeSeriesVisualization::resetZoom()
{
    _autoScroll = true;
}

void TimeSeriesVisualization::setWindowDuration(int seconds)
{
    _windowDuration = seconds;
    _autoScroll = true;
}

void TimeSeriesVisualization::addSignal(CanDbSignal *signal)
{
    if (_seriesMap.contains(signal)) return;

    VisualizationWidget::addSignal(signal);

    QLineSeries *series = new QLineSeries();
    series->setName(signal->name());
    _chart->addSeries(series);
    
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

void TimeSeriesVisualization::onAxisRangeChanged(qreal min, qreal max)
{
    Q_UNUSED(min);
    Q_UNUSED(max);
    if (!_isUpdatingRange) {
        _autoScroll = false;
    }
}
