/*

  Copyright (c) 2026 Antigravity AI

  This file is part of cangaroo.

*/

#pragma once

#include "VisualizationWidget.h"
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QDateTime>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>

#ifdef QT_CHARTS_USE_NAMESPACE
QT_CHARTS_USE_NAMESPACE
#endif

class TimeSeriesVisualization : public VisualizationWidget
{
    Q_OBJECT
public:
    explicit TimeSeriesVisualization(QWidget *parent, Backend &backend);
    virtual ~TimeSeriesVisualization();

    virtual void addMessage(const CanMessage &msg) override;
    virtual void clear() override;
    virtual void addSignal(CanDbSignal *signal) override;
    virtual void clearSignals() override;
    virtual void setSignalColor(CanDbSignal *signal, const QColor &color) override;
    virtual void zoomIn() override;
    virtual void zoomOut() override;
    virtual void resetZoom() override;
    virtual void setWindowDuration(int seconds) override;
public slots:
    virtual void onActivated() override;

    // Exposed for GraphWindow management
    QChartView* chartView() const { return _chartView; }
    QChart* chart() const { return _chart; }
    QGraphicsLineItem* cursorLine() const { return _cursorLine; }
    QGraphicsRectItem* tooltipBox() const { return _tooltipBox; }
    QGraphicsTextItem* tooltipText() const { return _tooltipText; }
    QMap<CanDbSignal*, QLineSeries*> seriesMap() const { return _seriesMap; }
    QMap<CanDbSignal*, QGraphicsEllipseItem*> tracers() const { return _tracers; }
    int getBusId(CanDbSignal* sig) const { return _signalBusMap.value(sig, 0); }

signals:
    void mouseMoved(QMouseEvent *event);

protected:
    virtual void wheelEvent(QWheelEvent *event) override;
    virtual bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QChartView *_chartView;
    QChart *_chart;
    QMap<CanDbSignal*, QLineSeries*> _seriesMap;
    QTimer *_updateTimer;
    int _windowDuration; // in seconds, 0 = all
    bool _autoScroll;
    bool _isUpdatingRange;

    void updateYAxisRange();

    QGraphicsLineItem *_cursorLine;
    QMap<CanDbSignal*, QGraphicsEllipseItem*> _tracers;
    QGraphicsRectItem *_tooltipBox;
    QGraphicsTextItem *_tooltipText;
    QMap<CanDbSignal*, int> _signalBusMap;

private slots:
    void onAxisRangeChanged(qreal min, qreal max);
};
