/*

  Copyright (c) 2026 Antigravity AI

  This file is part of cangaroo.

*/

#include "SignalSelectorDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QTreeWidgetItemIterator>
#include <core/MeasurementSetup.h>
#include <core/MeasurementNetwork.h>
#include <core/CanDbMessage.h>

SignalSelectorDialog::SignalSelectorDialog(QWidget *parent, Backend &backend)
    : QDialog(parent), _backend(backend)
{
    setWindowTitle(tr("Select Data"));
    setMinimumSize(600, 500);

    QVBoxLayout *layout = new QVBoxLayout(this);

    // Search and Filters
    QHBoxLayout *filterLayout = new QHBoxLayout();
    _searchEdit = new QLineEdit(this);
    _searchEdit->setPlaceholderText(tr("Search signals or messages..."));
    filterLayout->addWidget(_searchEdit);

    _showSelectedOnly = new QCheckBox(tr("Show selection only"), this);
    filterLayout->addWidget(_showSelectedOnly);
    
    layout->addLayout(filterLayout);

    // Tree
    _tree = new QTreeWidget(this);
    _tree->setHeaderLabels({tr("Name"), tr("Details"), tr("Comment")});
    _tree->setColumnWidth(0, 250);
    layout->addWidget(_tree);

    populateTree();

    // Buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox);

    connect(_searchEdit, &QLineEdit::textChanged, this, &SignalSelectorDialog::onSearchTextChanged);
    connect(_showSelectedOnly, &QCheckBox::toggled, this, &SignalSelectorDialog::onShowSelectedOnlyToggled);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void SignalSelectorDialog::populateTree()
{
    MeasurementSetup &setup = _backend.getSetup();
    
    QTreeWidgetItem *networksItem = new QTreeWidgetItem(_tree);
    networksItem->setText(0, tr("CAN Networks"));
    networksItem->setExpanded(true);

    for (MeasurementNetwork *network : setup.getNetworks()) {
        QTreeWidgetItem *netItem = new QTreeWidgetItem(networksItem);
        netItem->setText(0, network->name());
        
        for (pCanDb db : network->_canDbs) {
            for (CanDbMessage *msg : db->getMessageList().values()) {
                QTreeWidgetItem *msgItem = new QTreeWidgetItem(netItem);
                msgItem->setText(0, QString("%1 (0x%2)").arg(msg->getName()).arg(msg->getRaw_id(), 0, 16));
                
                for (CanDbSignal *sig : msg->getSignals()) {
                    QTreeWidgetItem *sigItem = new QTreeWidgetItem(msgItem);
                    sigItem->setText(0, sig->name());
                    
                    QString details = QString("%1 | %2..%3 %4")
                        .arg(sig->isUnsigned() ? tr("Unsigned") : tr("Signed"))
                        .arg(sig->getMinimumValue())
                        .arg(sig->getMaximumValue())
                        .arg(sig->getUnit());
                    
                    sigItem->setText(1, details);
                    sigItem->setText(2, sig->comment());
                    sigItem->setCheckState(0, Qt::Unchecked);
                    sigItem->setData(0, Qt::UserRole, QVariant::fromValue((void*)sig));
                }
            }
        }
    }
}

QList<CanDbSignal*> SignalSelectorDialog::getSelectedSignals() const
{
    QList<CanDbSignal*> selected;
    QTreeWidgetItemIterator it(_tree);
    while (*it) {
        if ((*it)->checkState(0) == Qt::Checked) {
            void* sigPtr = (*it)->data(0, Qt::UserRole).value<void*>();
            if (sigPtr) {
                selected.append((CanDbSignal*)sigPtr);
            }
        }
        ++it;
    }
    return selected;
}

void SignalSelectorDialog::setSelectedSignals(const QList<CanDbSignal*> &sigList)
{
    QTreeWidgetItemIterator it(_tree);
    while (*it) {
        void* sigPtr = (*it)->data(0, Qt::UserRole).value<void*>();
        if (sigPtr && sigList.contains((CanDbSignal*)sigPtr)) {
            (*it)->setCheckState(0, Qt::Checked);
            
            // Expand parents
            QTreeWidgetItem *p = (*it)->parent();
            while (p) {
                p->setExpanded(true);
                p = p->parent();
            }
        }
        ++it;
    }
}

void SignalSelectorDialog::onSearchTextChanged(const QString &text)
{
    filterTree(text, _showSelectedOnly->isChecked());
}

void SignalSelectorDialog::onShowSelectedOnlyToggled(bool checked)
{
    filterTree(_searchEdit->text(), checked);
}

void SignalSelectorDialog::filterTree(const QString &searchText, bool showSelectedOnly)
{
    QTreeWidgetItemIterator it(_tree);
    while (*it) {
        QTreeWidgetItem *item = *it;
        bool visible = shouldShowItem(item, searchText, showSelectedOnly);
        item->setHidden(!visible);
        
        // Ensure parents are visible if children are
        if (visible) {
            QTreeWidgetItem *p = item->parent();
            while (p) {
                p->setHidden(false);
                // p->setExpanded(true); // Don't auto-expand everything, just show path
                p = p->parent();
            }
        }
        ++it;
    }
}

bool SignalSelectorDialog::shouldShowItem(QTreeWidgetItem *item, const QString &searchText, bool showSelectedOnly)
{
    // If it's a signal (has data)
    void* sigPtr = item->data(0, Qt::UserRole).value<void*>();
    if (sigPtr) {
        bool matchSearch = searchText.isEmpty() || item->text(0).contains(searchText, Qt::CaseInsensitive);
        bool matchSelected = !showSelectedOnly || (item->checkState(0) == Qt::Checked);
        return matchSearch && matchSelected;
    }

    // For containers, we check if they have any visible children
    for (int i = 0; i < item->childCount(); ++i) {
        if (shouldShowItem(item->child(i), searchText, showSelectedOnly)) {
            return true;
        }
    }

    // Also show if the group itself matches the search
    if (!searchText.isEmpty() && item->text(0).contains(searchText, Qt::CaseInsensitive)) {
        return true;
    }

    return false;
}
