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

#pragma once

#include <core/Backend.h>
#include <core/ConfigurableWidget.h>
#include <core/MeasurementSetup.h>
#include "VisualizationWidget.h"

class QComboBox;
class QLabel;

namespace Ui {
class GraphWindow;
}

class QDomDocument;
class QDomElement;

class GraphWindow : public ConfigurableWidget
{
    Q_OBJECT

public:
    explicit GraphWindow(QWidget *parent, Backend &backend);
    ~GraphWindow();
    virtual bool saveXML(Backend &backend, QDomDocument &xml, QDomElement &root);
    virtual bool loadXML(Backend &backend, QDomElement &el);

private slots:
    void onViewTypeChanged(int index);
    void onAddSignalClicked();
    void onClearClicked();
    void onDurationChanged(int index);
    void onZoomInClicked();
    void onZoomOutClicked();
    void onResetZoomClicked();
    void onMessageEnqueued(int idx);
    void onMouseMove(QMouseEvent *event);
    void onLegendMarkerClicked();
    void onColumnSelectorChanged(int val);

private:
    void connectLegendMarkers(VisualizationWidget* v);
    Ui::GraphWindow *ui;
    QComboBox *_columnSelector = nullptr;
    QLabel *_columnLabel = nullptr;
    QWidget *_columnContainer = nullptr;
    Backend &_backend;
    double _sessionStartTime = -1.0;
    QList<VisualizationWidget*> _visualizations;
    VisualizationWidget* _activeVisualization;

    void setupVisualizations();
};
