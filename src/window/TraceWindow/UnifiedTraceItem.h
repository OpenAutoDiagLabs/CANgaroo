#pragma once

#include <QVector>
#include <memory>
#include <core/CanMessage.h>
#include <decoders/ProtocolTypes.h>

class UnifiedTraceItem {
public:
    UnifiedTraceItem(const CanMessage& frame, UnifiedTraceItem* parent = nullptr);
    UnifiedTraceItem(const ProtocolMessage& msg, UnifiedTraceItem* parent = nullptr);
    ~UnifiedTraceItem();

    void appendChild(std::shared_ptr<UnifiedTraceItem> child);
    std::shared_ptr<UnifiedTraceItem> child(int row);
    int childCount() const;
    int row() const;
    UnifiedTraceItem* parentItem();

    bool isProtocol() const { return m_isProtocol; }
    const CanMessage& rawFrame() const { return m_rawFrame; }
    const ProtocolMessage& protocolMessage() const { return m_protocolMessage; }

    uint32_t globalIndex() const { return m_globalIndex; }
    void setGlobalIndex(uint32_t index) { m_globalIndex = index; }
    uint64_t timestamp() const { return m_timestamp; }
    void setTimestamp(uint64_t ts) { m_timestamp = ts; }

private:
    QVector<std::shared_ptr<UnifiedTraceItem>> m_childItems;
    UnifiedTraceItem* m_parentItem;
    
    bool m_isProtocol;
    CanMessage m_rawFrame;
    ProtocolMessage m_protocolMessage;

    uint32_t m_globalIndex = 0;
    uint64_t m_timestamp = 0;
};
