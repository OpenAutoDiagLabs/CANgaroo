/*

  Copyright (c) 2026 Jayachandran Dharuman

  This file is part of CANgaroo.

  cangaroo is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  cangaroo is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with cangaroo.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "GraphWindow.h"
#include "ui_GraphWindow.h"

#include <QDomDocument>
#include <QInputDialog>
#include <QMessageBox>
#include <QColorDialog>
#include <QLabel>
#include <QComboBox>
#include <QtCharts/QLegendMarker>

#include <core/Backend.h>
#include <core/CanTrace.h>
#include <core/MeasurementSetup.h>
#include <core/MeasurementNetwork.h>

#include "TimeSeriesVisualization.h"
#include "ScatterVisualization.h"
#include "TextVisualization.h"
#include "GaugeVisualization.h"

#include "SignalSelectorDialog.h"

GraphWindow::GraphWindow(QWidget *parent, Backend &backend) :
    ConfigurableWidget(parent),
    ui(new Ui::GraphWindow),
    _backend(backend),
    _activeVisualization(nullptr)
{
    ui->setupUi(this);

    setupVisualizations();

    connect(ui->viewSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(onViewTypeChanged(int)));
    connect(ui->durationSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(onDurationChanged(int)));
    connect(ui->addSignalButton, &QPushButton::clicked, this, &GraphWindow::onAddSignalClicked);
    connect(ui->clearButton, &QPushButton::clicked, this, &GraphWindow::onClearClicked);
    connect(ui->zoomInButton, &QPushButton::clicked, this, &GraphWindow::onZoomInClicked);
    connect(ui->zoomOutButton, &QPushButton::clicked, this, &GraphWindow::onZoomOutClicked);
    connect(ui->resetZoomButton, &QPushButton::clicked, this, &GraphWindow::onResetZoomClicked);

    // Register with Trace for new messages
    connect(_backend.getTrace(), SIGNAL(messageEnqueued(int)), this, SLOT(onMessageEnqueued(int)));

    // Dynamic Column Selector for Gauges (Grouped in a container)
    _columnContainer = new QWidget(this);
    QHBoxLayout *colLayout = new QHBoxLayout(_columnContainer);
    colLayout->setContentsMargins(0, 0, 0, 0);
    colLayout->setSpacing(5);

    _columnLabel = new QLabel("Columns:", _columnContainer);
    _columnSelector = new QComboBox(_columnContainer);
    _columnSelector->addItems({"1", "2", "3", "4"});
    _columnSelector->setCurrentIndex(1); // Default to 2
    
    colLayout->addWidget(_columnLabel);
    colLayout->addWidget(_columnSelector);
    
    // Insert into toolbar layout (on the far right using a spring/stretch)
    // We use a spring to push the following widgets to the absolute right
    ui->toolbarLayout->addStretch(1); 
    ui->toolbarLayout->addWidget(_columnContainer);
    
    _columnContainer->hide();
    
    connect(_columnSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(onColumnSelectorChanged(int)));

    // Initial view - call this LAST after all UI elements are initialized
    onViewTypeChanged(0);
}

GraphWindow::~GraphWindow()
{
    delete ui;
}

void GraphWindow::setupVisualizations()
{
    // Add visualizations to list and stacked widget
    _visualizations.append(new TimeSeriesVisualization(ui->stackedWidget, _backend));
    _visualizations.append(new ScatterVisualization(ui->stackedWidget, _backend));
    _visualizations.append(new TextVisualization(ui->stackedWidget, _backend));
    _visualizations.append(new GaugeVisualization(ui->stackedWidget, _backend));

    QStringList names = {"Graph (Time-series)", "Scatter Chart", "Text", "Gauge"};
    for (int i = 0; i < _visualizations.size(); ++i) {
        ui->stackedWidget->addWidget(_visualizations[i]);
        ui->viewSelector->addItem(names[i]);

        // Connect mouse move for visualizations that support it
        if (auto tsv = qobject_cast<TimeSeriesVisualization*>(_visualizations[i])) {
            connect(tsv, &TimeSeriesVisualization::mouseMoved, this, &GraphWindow::onMouseMove);
            connectLegendMarkers(tsv);
        } else if (auto sv = qobject_cast<ScatterVisualization*>(_visualizations[i])) {
            connect(sv, &ScatterVisualization::mouseMoved, this, &GraphWindow::onMouseMove);
            connectLegendMarkers(sv);
        }
    }
}

void GraphWindow::connectLegendMarkers(VisualizationWidget* v)
{
    QChart *chart = nullptr;
    if (auto tsv = qobject_cast<TimeSeriesVisualization*>(v)) chart = tsv->chart();
    else if (auto sv = qobject_cast<ScatterVisualization*>(v)) chart = sv->chart();

    if (chart) {
        for (auto marker : chart->legend()->markers()) {
            disconnect(marker, &QLegendMarker::clicked, this, &GraphWindow::onLegendMarkerClicked);
            connect(marker, &QLegendMarker::clicked, this, &GraphWindow::onLegendMarkerClicked);
        }
    }
}

void GraphWindow::onViewTypeChanged(int index)
{
    if (index >= 0 && index < _visualizations.size()) {
        _activeVisualization = _visualizations[index];
        ui->stackedWidget->setCurrentWidget(_activeVisualization);

        _activeVisualization->onActivated();
        
        // Show column selector only for Gauge view (index 3)
        bool isGauge = (index == 3);
        if (_columnContainer) _columnContainer->setVisible(isGauge);
    }
}

void GraphWindow::onDurationChanged(int index)
{
    int seconds = 0;
    switch (index) {
        case 1: seconds = 60; break;
        case 2: seconds = 300; break;
        case 3: seconds = 600; break;
        case 4: seconds = 900; break;
        case 5: seconds = 1800; break;
        default: seconds = 0; break;
    }

    for (auto v : _visualizations) {
        v->setWindowDuration(seconds);
    }
    
    if (_activeVisualization) {
        _activeVisualization->onActivated();
    }
}

void GraphWindow::onAddSignalClicked()
{
    SignalSelectorDialog dlg(this, _backend);
    dlg.setSelectedSignals(_activeVisualization->getSignals());
    if (dlg.exec() == QDialog::Accepted) {
        QList<CanDbSignal*> signalList = dlg.getSelectedSignals();
        for (auto v : _visualizations) {
            v->clearSignals();
            for (auto signal : signalList) {
                v->addSignal(signal);
            }
            connectLegendMarkers(v);
        }
    }
}

void GraphWindow::onClearClicked()
{
    _sessionStartTime = -1.0;
    for (auto v : _visualizations) {
        v->clear();
    }
}

void GraphWindow::onZoomInClicked()
{
    _activeVisualization->zoomIn();
}

void GraphWindow::onZoomOutClicked()
{
    _activeVisualization->zoomOut();
}

void GraphWindow::onResetZoomClicked()
{
    _activeVisualization->resetZoom();
}

void GraphWindow::onMessageEnqueued(int idx)
{
    CanMessage msg = _backend.getTrace()->getMessage(idx);
    // Basic check for valid/non-empty message:
    if (msg.getId() == 0 && msg.getLength() == 0 && !msg.isExtended()) {
        // This might be an empty message from return CanMessage();
        // However, ID 0 DLC 0 is valid in CAN. 
        // Better to check size() or similar, but for now this is safer than a null pointer.
    }

    if (_sessionStartTime < 0) {
        _sessionStartTime = msg.getFloatTimestamp();
        for (auto v : _visualizations) {
            v->setGlobalStartTime(_sessionStartTime);
        }
    }
    for (auto v : _visualizations) {
        v->addMessage(msg);
    }
}

void GraphWindow::onMouseMove(QMouseEvent *event)
{
    auto tsv = qobject_cast<TimeSeriesVisualization*>(_activeVisualization);
    auto sv = qobject_cast<ScatterVisualization*>(_activeVisualization);

    if (!tsv && !sv) {
        // Hide overlays for all
        for (auto v : _visualizations) {
            if (auto t = qobject_cast<TimeSeriesVisualization*>(v)) {
                t->cursorLine()->hide();
                t->tooltipBox()->hide();
                for (auto tracer : t->tracers().values()) tracer->hide();
            } else if (auto s = qobject_cast<ScatterVisualization*>(v)) {
                s->cursorLine()->hide();
                s->tooltipBox()->hide();
                for (auto tracer : s->tracers().values()) tracer->hide();
            }
        }
        return;
    }

    QChartView *view = tsv ? tsv->chartView() : sv->chartView();
    QChart *chart = tsv ? tsv->chart() : sv->chart();
    QGraphicsLineItem *cursor = tsv ? tsv->cursorLine() : sv->cursorLine();
    QGraphicsRectItem *tooltipBox = tsv ? tsv->tooltipBox() : sv->tooltipBox();
    QGraphicsTextItem *tooltipText = tsv ? tsv->tooltipText() : sv->tooltipText();

    // Mapping from viewport to chart coordinates
    QPointF scenePos = view->mapToScene(event->pos());
    QPointF chartPos = chart->mapFromScene(scenePos);

    if (!chart->plotArea().contains(chartPos)) {
        cursor->hide();
        tooltipBox->hide();
        if (tsv) for (auto tracer : tsv->tracers().values()) tracer->hide();
        if (sv) for (auto tracer : sv->tracers().values()) tracer->hide();
        return;
    }

    double t = chart->mapToValue(chartPos).x();

    // Update cursor line (position is in chart coordinates)
    cursor->setLine(chartPos.x(), chart->plotArea().top(), chartPos.x(), chart->plotArea().bottom());
    cursor->show();

    // Absolute Timestamp
    uint64_t startUsecs = _backend.getUsecsAtMeasurementStart();
    uint64_t currentUsecs = startUsecs + (uint64_t)(t * 1000000.0);
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(currentUsecs / 1000);
    QString timeStr = dt.toString("yyyy-MM-dd HH:mm:ss.zzz t");

    QString html = QString("<div style='font-family: Arial; font-size: 11px; padding: 5px; background: white;'>"
                           "<b>%1</b><br/><br/>").arg(timeStr);
    
    bool foundAny = false;
    
    if (tsv) {
        auto seriesMap = tsv->seriesMap();
        auto tracers = tsv->tracers();
        for (auto it = seriesMap.begin(); it != seriesMap.end(); ++it) {
            CanDbSignal *sig = it.key();
            QXYSeries *series = it.value();
            QGraphicsEllipseItem *tracer = tracers.value(sig);

            const QList<QPointF> points = series->points();
            if (points.isEmpty()) { if (tracer) tracer->hide(); continue; }

            int left = 0, right = points.size() - 1;
            while (left < right) {
                int mid = (left + right) / 2;
                if (points[mid].x() < t) left = mid + 1;
                else right = mid;
            }
            int nearestIdx = (left > 0 && qAbs(points[left-1].x() - t) < qAbs(points[left].x() - t)) ? left - 1 : left;

            QValueAxis *axisX = qobject_cast<QValueAxis*>(chart->axes(Qt::Horizontal).first());
            double xRange = axisX->max() - axisX->min();
            
            if (qAbs(points[nearestIdx].x() - t) < xRange * 0.1) {
                html += QString("<span style='color:%1; font-size: 14px;'>●</span> (Bus %2) %3: <b>%4</b> %5<br/>")
                        .arg(series->color().name()).arg(tsv->getBusId(sig)).arg(sig->name())
                        .arg(points[nearestIdx].y(), 0, 'f', 2).arg(sig->getUnit());
                foundAny = true;
                if (tracer) { tracer->setPos(chart->mapToPosition(points[nearestIdx])); tracer->show(); }
            } else { if (tracer) tracer->hide(); }
        }
    } else if (sv) {
        auto seriesMap = sv->seriesMap();
        auto tracers = sv->tracers();
        for (auto it = seriesMap.begin(); it != seriesMap.end(); ++it) {
            CanDbSignal *sig = it.key();
            QXYSeries *series = it.value();
            QGraphicsEllipseItem *tracer = tracers.value(sig);

            const QList<QPointF> points = series->points();
            if (points.isEmpty()) { if (tracer) tracer->hide(); continue; }

            // Scatter points might not be sorted by X if it's a "Distribution View" but we added them in temporal order.
            // Let's assume temporal order for now.
            int left = 0, right = points.size() - 1;
            while (left < right) {
                int mid = (left + right) / 2;
                if (points[mid].x() < t) left = mid + 1;
                else right = mid;
            }
            int nearestIdx = (left > 0 && qAbs(points[left-1].x() - t) < qAbs(points[left].x() - t)) ? left - 1 : left;

            QValueAxis *axisX = qobject_cast<QValueAxis*>(chart->axes(Qt::Horizontal).first());
            double xRange = axisX->max() - axisX->min();
            
            // For scatter, maybe use a smaller/stricter proximity?
            if (qAbs(points[nearestIdx].x() - t) < xRange * 0.05) {
                html += QString("<span style='color:%1; font-size: 14px;'>●</span> (Bus %2) %3: <b>%4</b> %5<br/>")
                        .arg(series->color().name()).arg(sv->getBusId(sig)).arg(sig->name())
                        .arg(points[nearestIdx].y(), 0, 'f', 2).arg(sig->getUnit());
                foundAny = true;
                if (tracer) { tracer->setPos(chart->mapToPosition(points[nearestIdx])); tracer->show(); }
            } else { if (tracer) tracer->hide(); }
        }
    }

    html += "</div>";

    if (foundAny) {
        tooltipText->setHtml(html);
        QRectF textRect = tooltipText->boundingRect();
        tooltipBox->setRect(0, 0, textRect.width() + 4, textRect.height() + 4);
        QPointF tooltipPos = chartPos + QPointF(15, -textRect.height() - 15);
        if (tooltipPos.x() + tooltipBox->rect().width() > chart->plotArea().right()) tooltipPos.setX(chartPos.x() - tooltipBox->rect().width() - 15);
        if (tooltipPos.y() < chart->plotArea().top()) tooltipPos.setY(chartPos.y() + 15);
        tooltipBox->setPos(tooltipPos);
        tooltipBox->show();
    } else {
        tooltipBox->hide();
    }
}

void GraphWindow::onLegendMarkerClicked()
{
    if (!_activeVisualization) return;
    
    QLegendMarker* marker = qobject_cast<QLegendMarker*>(sender());
    if (!marker) return;

    QXYSeries *series = qobject_cast<QXYSeries*>(marker->series());
    if (!series) return;

    // Find the signal associated with this series
    CanDbSignal *targetSignal = nullptr;
    for (auto v : _visualizations) {
        if (!v) continue;
        if (auto tsv = qobject_cast<TimeSeriesVisualization*>(v)) {
            auto seriesMap = tsv->seriesMap();
            for (auto it = seriesMap.begin(); it != seriesMap.end(); ++it) {
                if (it.value() == series) { targetSignal = it.key(); break; }
            }
        } else if (auto sv = qobject_cast<ScatterVisualization*>(v)) {
            auto seriesMap = sv->seriesMap();
            for (auto it = seriesMap.begin(); it != seriesMap.end(); ++it) {
                if (it.value() == series) { targetSignal = it.key(); break; }
            }
        }
        if (targetSignal) break;
    }

    if (targetSignal) {
        QColor color = QColorDialog::getColor(series->color(), this, "Select Signal Color");
        if (color.isValid()) {
            for (auto v : _visualizations) {
                if (v) v->setSignalColor(targetSignal, color);
            }
        }
    }
}

void GraphWindow::onColumnSelectorChanged(int index)
{
    if (auto gv = qobject_cast<GaugeVisualization*>(_visualizations[3])) {
        gv->setColumnCount(index + 1);
    }
}

bool GraphWindow::saveXML(Backend &backend, QDomDocument &xml, QDomElement &root)
{
    if (!ConfigurableWidget::saveXML(backend, xml, root)) { return false; }
    root.setAttribute("type", "GraphWindow");
    root.setAttribute("viewType", ui->viewSelector->currentIndex());
    return true;
}

bool GraphWindow::loadXML(Backend &backend, QDomElement &el)
{
    if (!ConfigurableWidget::loadXML(backend, el)) { return false; }
    int index = el.attribute("viewType", "0").toInt();
    ui->viewSelector->setCurrentIndex(index);
    return true;
}
