/*

  Copyright (c) 2026 Antigravity AI

  This file is part of cangaroo.

  cangaroo is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

*/

#pragma once

#include <QWidget>
#include <core/CanMessage.h>
#include <core/CanDbSignal.h>
#include <core/Backend.h>

class VisualizationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VisualizationWidget(QWidget *parent, Backend &backend) : QWidget(parent), _backend(backend) {}
    virtual ~VisualizationWidget() {}

    virtual void addMessage(const CanMessage &msg) = 0;
    virtual void clear() = 0;
    
    virtual void zoomIn() {}
    virtual void zoomOut() {}
    virtual void resetZoom() {}
    virtual void setWindowDuration(int seconds) { Q_UNUSED(seconds); }
    virtual void addSignal(CanDbSignal *signal) { if (!_signals.contains(signal)) _signals.append(signal); }
    virtual void addSignals(const QList<CanDbSignal*> &signalList) { 
        for (auto s : signalList) addSignal(s); 
    }
    virtual void removeSignal(CanDbSignal *signal) { _signals.removeAll(signal); }
    virtual void clearSignals() { _signals.clear(); }
    virtual QList<CanDbSignal*> getSignals() const { return _signals; }

    virtual void setGlobalStartTime(double t) { _startTime = t; }

    virtual void setSignalColor(CanDbSignal *signal, const QColor &color) {
        _signalColors[signal] = color;
    }
    
    QColor getSignalColor(CanDbSignal *signal) const {
        return _signalColors.value(signal, Qt::white);
    }

    virtual void onActivated() {}

protected:
    Backend &_backend;
    QList<CanDbSignal*> _signals;
    QMap<CanDbSignal*, QColor> _signalColors;
    double _startTime = -1.0;
};
