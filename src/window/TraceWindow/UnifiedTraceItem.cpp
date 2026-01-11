#include "UnifiedTraceItem.h"

UnifiedTraceItem::UnifiedTraceItem(const CanMessage& frame, UnifiedTraceItem* parent)
    : m_parentItem(parent), m_isProtocol(false), m_rawFrame(frame), m_globalIndex(0)
{
    m_timestamp = static_cast<uint64_t>(frame.getFloatTimestamp() * 1000000.0);
}

UnifiedTraceItem::UnifiedTraceItem(const ProtocolMessage& msg, UnifiedTraceItem* parent)
    : m_parentItem(parent), m_isProtocol(true), m_protocolMessage(msg)
{
    // Create children for each raw frame
    for (const auto& frame : msg.rawFrames) {
        appendChild(std::make_shared<UnifiedTraceItem>(frame, this));
    }
}

UnifiedTraceItem::~UnifiedTraceItem()
{
}

void UnifiedTraceItem::appendChild(std::shared_ptr<UnifiedTraceItem> child)
{
    m_childItems.append(child);
}

std::shared_ptr<UnifiedTraceItem> UnifiedTraceItem::child(int row)
{
    if (row < 0 || row >= m_childItems.size())
        return nullptr;
    return m_childItems.at(row);
}

int UnifiedTraceItem::childCount() const
{
    return m_childItems.size();
}

int UnifiedTraceItem::row() const
{
    if (m_parentItem) {
        for (int i = 0; i < m_parentItem->m_childItems.size(); ++i) {
            if (m_parentItem->m_childItems.at(i).get() == this) {
                return i;
            }
        }
    }
    return 0;
}

UnifiedTraceItem* UnifiedTraceItem::parentItem()
{
    return m_parentItem;
}
