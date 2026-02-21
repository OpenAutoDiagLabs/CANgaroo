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

    connect(_tree, &QTreeWidget::itemChanged, this, &SignalSelectorDialog::onItemChanged);

    connect(this, &SignalSelectorDialog::finished, [this]() {
        disconnect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &SignalSelectorDialog::applyTheme);
    });

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &SignalSelectorDialog::applyTheme);
    applyTheme(ThemeManager::instance().currentTheme());

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
                msgItem->setCheckState(0, Qt::Unchecked); // Select All checkbox
                
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

                    // Add colored legend icon (deterministic color based on name)
                    QPixmap pix(12, 12);
                    uint h = qHash(sig->name());
                    QColor c = QColor::fromHsl(h % 360, 180, 150);
                    pix.fill(c);
                    sigItem->setIcon(0, QIcon(pix));
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

void SignalSelectorDialog::onItemChanged(QTreeWidgetItem *item, int column)
{
    if (column != 0) return;

    _tree->blockSignals(true);
    Qt::CheckState state = item->checkState(0);

    // If it's a message/network/node (container), toggle all children
    for (int i = 0; i < item->childCount(); ++i) {
        item->child(i)->setCheckState(0, state);
        // Recursively handle sub-children if any
        for (int j = 0; j < item->child(i)->childCount(); ++j) {
            item->child(i)->child(j)->setCheckState(0, state);
        }
    }

    // If it's a child, update parent's state (PartiallyChecked logic)
    QTreeWidgetItem *parent = item->parent();
    if (parent) {
        int checkedCount = 0;
        for (int i = 0; i < parent->childCount(); ++i) {
            if (parent->child(i)->checkState(0) != Qt::Unchecked) {
                checkedCount++;
            }
        }
        
        if (checkedCount == 0) parent->setCheckState(0, Qt::Unchecked);
        else if (checkedCount == parent->childCount()) parent->setCheckState(0, Qt::Checked);
        else parent->setCheckState(0, Qt::PartiallyChecked);
    }

    _tree->blockSignals(false);
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

void SignalSelectorDialog::applyTheme(ThemeManager::Theme theme)
{
    bool isDark = (theme == ThemeManager::Dark);
    
    // Revert dialog-level background styling to keep original view
    this->setStyleSheet("");

    if (isDark) {
        // Targeted styling for tree indicators (CAN messages and signals)
        QString treeStyle = 
            "QTreeWidget::indicator, QTreeView::indicator {"
            "  width: 16px;"
            "  height: 16px;"
            "  border: 2px solid #FFFFFF;" // Pure white border for maximum visibility
            "  border-radius: 3px;"
            "  background-color: transparent;"
            "}"
            "QTreeWidget::indicator:checked, QTreeView::indicator:checked {"
            "  background-color: #00FF00;"
            "}"
            "QTreeWidget::indicator:indeterminate, QTreeView::indicator:indeterminate {"
            "  background-color: #555;"
            "}";

        // Targeted styling for the standalone "Show selection only" checkbox
        QString checkStyle = 
            "QCheckBox::indicator {"
            "  width: 16px;"
            "  height: 16px;"
            "  border: 2px solid #FFFFFF;"
            "  border-radius: 3px;"
            "  background-color: transparent;"
            "}"
            "QCheckBox::indicator:checked {"
            "  background-color: #00FF00;"
            "}";

        _tree->setStyleSheet(treeStyle);
        _showSelectedOnly->setStyleSheet(checkStyle);
    } else {
        // Restore standard Light Mode styling
        _tree->setStyleSheet("");
        _showSelectedOnly->setStyleSheet("");
    }
}
