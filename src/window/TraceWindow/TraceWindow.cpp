/*

  Copyright (c) 2015, 2016 Hubert Denkmair <hubert@denkmair.de>

  This file is part of cangaroo.

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

#include "TraceWindow.h"
#include "ui_TraceWindow.h"

#include <QDomDocument>
#include <QSortFilterProxyModel>
#include "LinearTraceViewModel.h"
#include "AggregatedTraceViewModel.h"
#include "TraceFilterModel.h"
#include <core/Backend.h>


TraceWindow::TraceWindow(QWidget *parent, Backend &backend) :
    ConfigurableWidget(parent),
    ui(new Ui::TraceWindow),
    _backend(&backend),
    _mode(mode_aggregated),
    _timestampMode(timestamp_mode_absolute)
{
    ui->setupUi(this);

    _aggregatedTraceViewModel = new AggregatedTraceViewModel(backend);
    _aggregatedProxyModel = new QSortFilterProxyModel(this);
    _aggregatedProxyModel->setSourceModel(_aggregatedTraceViewModel);
    _aggregatedProxyModel->setDynamicSortFilter(true);

    _aggFilteredModel = new TraceFilterModel(this);
    _aggFilteredModel->setSourceModel(_aggregatedProxyModel);

    setMode(mode_aggregated);

    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    ui->tree->setFont(font);
    ui->tree->setAlternatingRowColors(true);

    ui->tree->setUniformRowHeights(true);
    ui->tree->setColumnWidth(BaseTraceViewModel::column_index, 70);
    ui->tree->setColumnWidth(BaseTraceViewModel::column_timestamp, 100);
    ui->tree->setColumnWidth(BaseTraceViewModel::column_channel, 120);
    ui->tree->setColumnWidth(BaseTraceViewModel::column_direction, 50);
    ui->tree->setColumnWidth(BaseTraceViewModel::column_type, 80);
    ui->tree->setColumnWidth(BaseTraceViewModel::column_canid, 110);
    ui->tree->setColumnWidth(BaseTraceViewModel::column_sender, 150);
    ui->tree->setColumnWidth(BaseTraceViewModel::column_name, 150);
    ui->tree->setColumnWidth(BaseTraceViewModel::column_dlc, 50);
    ui->tree->setColumnWidth(BaseTraceViewModel::column_data, 250);
    ui->tree->setColumnWidth(BaseTraceViewModel::column_comment, 120);
    ui->tree->sortByColumn(BaseTraceViewModel::column_index, Qt::AscendingOrder);

    ui->cbTimestampMode->addItem(tr("Absolute"), 0);
    ui->cbTimestampMode->addItem(tr("Relative"), 1);
    ui->cbTimestampMode->addItem(tr("Delta"), 2);
    setTimestampMode(timestamp_mode_delta);

    connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(on_cbFilterChanged()));

    connect(ui->TraceClearpushButton, SIGNAL(released()), this, SLOT(on_cbTraceClearpushButton()));

}

TraceWindow::~TraceWindow()
{
    delete ui;
    delete _aggregatedTraceViewModel;
}

void TraceWindow::setMode(TraceWindow::mode_t mode)
{
    _mode = mode;

    ui->tree->setSortingEnabled(true);
    ui->tree->setModel(_aggFilteredModel);
    ui->tree->sortByColumn(BaseTraceViewModel::column_canid, Qt::AscendingOrder);
    ui->tree->scrollToBottom();
}


void TraceWindow::setTimestampMode(int mode)
{
    timestamp_mode_t new_mode;
    if ( (mode>=0) && (mode<timestamp_modes_count) )
    {
        new_mode = (timestamp_mode_t) mode;
    }
    else
    {
        new_mode = timestamp_mode_absolute;
    }

    _aggregatedTraceViewModel->setTimestampMode(new_mode);

    if (new_mode != _timestampMode)
    {
        _timestampMode = new_mode;
        for (int i=0; i<ui->cbTimestampMode->count(); i++)
        {
            if (ui->cbTimestampMode->itemData(i).toInt() == new_mode)
            {
                ui->cbTimestampMode->setCurrentIndex(i);
            }
        }
        emit(settingsChanged(this));
    }
}

bool TraceWindow::saveXML(Backend &backend, QDomDocument &xml, QDomElement &root)
{
    if (!ConfigurableWidget::saveXML(backend, xml, root))
    {
        return false;
    }

    root.setAttribute("type", "TraceWindow");
    root.setAttribute("mode", "aggregated");
    root.setAttribute("TimestampMode", _timestampMode);

    QDomElement elAggregated = xml.createElement("AggregatedTraceView");
    elAggregated.setAttribute("SortColumn", _aggregatedProxyModel->sortColumn());
    root.appendChild(elAggregated);

    return true;
}

bool TraceWindow::loadXML(Backend &backend, QDomElement &el)
{
    if (!ConfigurableWidget::loadXML(backend, el))
    {
        return false;
    }

    setMode(mode_aggregated);
    setTimestampMode(el.attribute("TimestampMode", "0").toInt());

    QDomElement elAggregated = el.firstChildElement("AggregatedTraceView");
    int sortColumn = elAggregated.attribute("SortColumn", "-1").toInt();
    ui->tree->sortByColumn(sortColumn,Qt::SortOrder::AscendingOrder);
    //ui->tree->sortByColumn(sortColumn);

    return true;
}

void TraceWindow::rowsInserted(const QModelIndex &parent, int first, int last)
{
    (void) parent;
    (void) first;
    (void) last;

    if(_backend->getTrace()->size() > 1000000)
    {
        _backend->clearTrace();
    }
}



void TraceWindow::on_cbTimestampMode_currentIndexChanged(int index)
{
    setTimestampMode((timestamp_mode_t)ui->cbTimestampMode->itemData(index).toInt());
}

void TraceWindow::on_cbFilterChanged()
{
    _aggFilteredModel->setFilterText(ui->filterLineEdit->text());
    _aggFilteredModel->invalidate();
}

void TraceWindow::on_cbTraceClearpushButton()
{
    _backend->clearTrace();
    _backend->clearLog();
}
