#pragma once

#include "BaseTraceViewModel.h"
#include "UnifiedTraceItem.h"
#include <decoders/ProtocolManager.h>
#include <memory>

class UnifiedTraceViewModel : public BaseTraceViewModel
{
    Q_OBJECT

public:
    UnifiedTraceViewModel(Backend &backend);
    ~UnifiedTraceViewModel();

    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    virtual QModelIndex parent(const QModelIndex &child) const override;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

private slots:
    void beforeAppend(int num_messages);
    void afterAppend();
    void beforeClear();
    void afterClear();

private:
    std::shared_ptr<UnifiedTraceItem> m_rootItem;
    ProtocolManager m_protocolManager;
    int m_lastProcessedIndex = -1;
    uint32_t m_globalIndexCounter = 1;
    uint64_t m_firstTimestamp = 0;
    uint64_t m_previousRowTimestamp = 0;

    QVariant data_DisplayRole(const QModelIndex &index) const;
    QVariant data_TextColorRole(const QModelIndex &index) const;
    QString formatUnifiedTimestamp(uint64_t ts, uint64_t prevTs) const;
};
