/*

  Copyright (c) 2026 Antigravity AI

  This file is part of cangaroo.

*/

#pragma once

#include "VisualizationWidget.h"
#include <QScrollArea>
#include <QGridLayout>
#include <QWidget>
#include <QPainter>

class GaugeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GaugeWidget(QWidget *parent = nullptr);
    void setSignalName(const QString &name) { _name = name; update(); }
    void setValue(double value);
    void setRange(double min, double max);
    void setUnit(const QString &unit) { _unit = unit; update(); }
    void setColor(const QColor &color) { _color = color; update(); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString _name;
    QString _unit;
    double _value;
    double _min;
    double _max;
    QColor _color;
};

class GaugeVisualization : public VisualizationWidget
{
    Q_OBJECT
public:
    explicit GaugeVisualization(QWidget *parent, Backend &backend);
    virtual ~GaugeVisualization();

    virtual void addMessage(const CanMessage &msg) override;
    virtual void clear() override;
    virtual void onActivated() override;
    virtual void addSignal(CanDbSignal *signal) override;
    virtual void clearSignals() override;
    virtual void setSignalColor(CanDbSignal *signal, const QColor &color) override;

    void setColumnCount(int count);

private:
    QScrollArea *_scrollArea;
    QWidget *_container;
    QGridLayout *_containerLayout;
    QMap<CanDbSignal*, GaugeWidget*> _gaugeMap;
    int _columnCount;
};
