/*

  Copyright (c) 2026 Antigravity AI

  This file is part of cangaroo.

*/

#pragma once

#include "VisualizationWidget.h"
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QMap>

class TextVisualization : public VisualizationWidget
{
    Q_OBJECT
public:
    explicit TextVisualization(QWidget *parent, Backend &backend);
    virtual ~TextVisualization();

    virtual void addMessage(const CanMessage &msg) override;
    virtual void clear() override;
    virtual void onActivated() override;
    virtual void addSignal(CanDbSignal *signal) override;
    virtual void clearSignals() override;
    virtual void setSignalColor(CanDbSignal *signal, const QColor &color) override;

protected:
    virtual void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateUi();

private:
    void updateFontScaling();
    void createSignalCard(CanDbSignal *signal);

    struct SignalData {
        double value;
        bool updated;
        QLabel *valueLabel;
        QWidget *card;
    };

    QScrollArea *_scrollArea;
    QWidget *_container;
    QVBoxLayout *_containerLayout;
    QTimer *_updateTimer;
    QMap<CanDbSignal*, SignalData> _signalDataMap;
};
