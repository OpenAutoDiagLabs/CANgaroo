/*

  Copyright (c) 2026 Antigravity AI

  This file is part of cangaroo.

*/

#pragma once

#include <QDialog>
#include <QTreeWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <core/Backend.h>
#include <core/CanDbSignal.h>

class SignalSelectorDialog : public QDialog {
    Q_OBJECT
public:
    explicit SignalSelectorDialog(QWidget *parent, Backend &backend);
    QList<CanDbSignal*> getSelectedSignals() const;
    void setSelectedSignals(const QList<CanDbSignal*> &sigList);

private slots:
    void onSearchTextChanged(const QString &text);
    void onShowSelectedOnlyToggled(bool checked);

private:
    Backend &_backend;
    QLineEdit *_searchEdit;
    QTreeWidget *_tree;
    QCheckBox *_showSelectedOnly;

    void populateTree();
    void filterTree(const QString &searchText, bool showSelectedOnly);
    bool shouldShowItem(QTreeWidgetItem *item, const QString &searchText, bool showSelectedOnly);
};
