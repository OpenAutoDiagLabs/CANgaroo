#include "UnifiedTraceViewModel.h"
#include <core/CanTrace.h>
#include <core/Backend.h>
#include <QColor>

UnifiedTraceViewModel::UnifiedTraceViewModel(Backend &backend)
    : BaseTraceViewModel(backend)
{
    m_rootItem = std::make_shared<UnifiedTraceItem>(CanMessage()); // Dummy root
    m_firstTimestamp = 0;
    m_previousRowTimestamp = 0;
    m_globalIndexCounter = 1;

    connect(backend.getTrace(), SIGNAL(beforeAppend(int)), this, SLOT(beforeAppend(int)));
    connect(backend.getTrace(), SIGNAL(afterAppend()), this, SLOT(afterAppend()));
    connect(backend.getTrace(), SIGNAL(beforeClear()), this, SLOT(beforeClear()));
    connect(backend.getTrace(), SIGNAL(afterClear()), this, SLOT(afterClear()));
}

UnifiedTraceViewModel::~UnifiedTraceViewModel()
{
}

QModelIndex UnifiedTraceViewModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    UnifiedTraceItem *parentItem;
    if (!parent.isValid())
        parentItem = m_rootItem.get();
    else
        parentItem = static_cast<UnifiedTraceItem*>(parent.internalPointer());

    std::shared_ptr<UnifiedTraceItem> childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem.get());
    else
        return QModelIndex();
}

QModelIndex UnifiedTraceViewModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    UnifiedTraceItem *childItem = static_cast<UnifiedTraceItem*>(child.internalPointer());
    UnifiedTraceItem *parentItem = childItem->parentItem();

    if (parentItem == m_rootItem.get())
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int UnifiedTraceViewModel::rowCount(const QModelIndex &parent) const
{
    UnifiedTraceItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = m_rootItem.get();
    else
        parentItem = static_cast<UnifiedTraceItem*>(parent.internalPointer());

    return parentItem->childCount();
}

int UnifiedTraceViewModel::columnCount(const QModelIndex &parent) const
{
    return column_count;
}

bool UnifiedTraceViewModel::hasChildren(const QModelIndex &parent) const
{
    return rowCount(parent) > 0;
}

QVariant UnifiedTraceViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    switch (role) {
        case Qt::DisplayRole:
            return data_DisplayRole(index);
        case Qt::ForegroundRole:
            return data_TextColorRole(index);
        case Qt::TextAlignmentRole:
            return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
        default:
            return BaseTraceViewModel::data(index, role);
    }
}

void UnifiedTraceViewModel::beforeAppend(int num_messages)
{
    // Integration with ProtocolManager happens here
    // But beginInsertRows needs to know how many PARENT rows we are adding.
    // This is tricky because one ProtocolMessage might consume multiple frames.
}

void UnifiedTraceViewModel::afterAppend()
{
    CanTrace *trace = backend()->getTrace();
    int size = trace->size();
    
    for (int i = m_lastProcessedIndex + 1; i < size; ++i) {
        const CanMessage *msg = trace->getMessage(i);
        if (!msg) continue;

        ProtocolMessage pmsg;
        DecodeStatus status = m_protocolManager.processFrame(*msg, pmsg);
        
        if (status == DecodeStatus::Completed) {
            // New protocol message completed. Add it as a parent row.
            beginInsertRows(QModelIndex(), m_rootItem->childCount(), m_rootItem->childCount());
            auto item = std::make_shared<UnifiedTraceItem>(pmsg, m_rootItem.get());
            
            if (m_firstTimestamp == 0) m_firstTimestamp = pmsg.timestamp;
            item->setTimestamp(pmsg.timestamp);
            item->setGlobalIndex(pmsg.globalIndex);
            
            m_rootItem->appendChild(item);
            endInsertRows();
            m_globalIndexCounter = pmsg.globalIndex + 1; // Keep counters synced
        } else if (status == DecodeStatus::Ignored) {
            // Not part of any protocol. Add as a standalone raw frame.
            beginInsertRows(QModelIndex(), m_rootItem->childCount(), m_rootItem->childCount());
            auto item = std::make_shared<UnifiedTraceItem>(*msg, m_rootItem.get());
            
            uint64_t ts = static_cast<uint64_t>(msg->getFloatTimestamp() * 1000000.0);
            if (m_firstTimestamp == 0) m_firstTimestamp = ts;
            item->setTimestamp(ts);
            item->setGlobalIndex(m_globalIndexCounter++);

            m_rootItem->appendChild(item);
            endInsertRows();
        }
 else if (status == DecodeStatus::Consumed) {
            // Part of an ongoing protocol sequence. 
            // We don't add it as a root level item, it will appear as a child once Completed.
        }
        m_lastProcessedIndex = i;
    }
}

void UnifiedTraceViewModel::beforeClear()
{
    beginResetModel();
}

void UnifiedTraceViewModel::afterClear()
{
    m_rootItem = std::make_shared<UnifiedTraceItem>(CanMessage());
    m_protocolManager.reset();
    m_lastProcessedIndex = -1;
    m_globalIndexCounter = 1;
    m_firstTimestamp = 0;
    m_previousRowTimestamp = 0;
    endResetModel();
}

QVariant UnifiedTraceViewModel::data_DisplayRole(const QModelIndex &index) const
{
    UnifiedTraceItem *item = static_cast<UnifiedTraceItem*>(index.internalPointer());
    
    uint64_t current = item->timestamp();
    if (index.column() == column_index) {
        return (item->parentItem() == m_rootItem.get()) ? QVariant(item->globalIndex()) : QVariant();
    }

    if (item->isProtocol()) {
        const ProtocolMessage& pmsg = item->protocolMessage();
        switch (index.column()) {
            case column_timestamp: 
            {
                uint64_t prev = 0;
                if (index.row() > 0) {
                    auto prevItem = item->parentItem()->child(index.row() - 1);
                    if (prevItem) prev = prevItem->timestamp();
                } else {
                    prev = (item->parentItem() != m_rootItem.get()) ? item->parentItem()->timestamp() : 0;
                }
                return formatUnifiedTimestamp(current, prev);
            }
            case column_canid: 
                if (pmsg.protocol == "uds") return QString("0x%1").arg(pmsg.id, 2, 16, QChar('0'));
                if (pmsg.protocol == "j1939") return QString("pgn:0x%1").arg(pmsg.id, 0, 16);
                return pmsg.protocol;
            case column_name: return pmsg.name;
            case column_comment: return pmsg.description;
            case column_data: return pmsg.payload.toHex(' ');
            case column_dlc: return pmsg.payload.size();
            case column_direction: return pmsg.rawFrames.isEmpty() ? "" : (pmsg.rawFrames.first().isRX() ? "RX" : "TX");
            case column_channel: return pmsg.rawFrames.isEmpty() ? "" : backend()->getInterfaceName(pmsg.rawFrames.first().getInterfaceId());
            case column_sender:
                if (pmsg.protocol == "uds") {
                    return (pmsg.type == MessageType::Request) ? "Tester" : "ECU";
                }
                return "";
            default: return QVariant();
        }
    } else {
        const CanMessage& msg = item->rawFrame();
        CanDbMessage *dbmsg = backend()->findDbMessage(msg);
        switch (index.column()) {
            case column_index: 
                return (item->parentItem() == m_rootItem.get()) ? QVariant(item->globalIndex()) : QVariant();
            case column_timestamp: 
            {
                uint64_t prev = 0;
                if (index.row() > 0) {
                    auto prevItem = item->parentItem()->child(index.row() - 1);
                    if (prevItem) prev = prevItem->timestamp();
                } else {
                    prev = (item->parentItem() != m_rootItem.get()) ? item->parentItem()->timestamp() : 0;
                }
                return formatUnifiedTimestamp(current, prev);
            }
            case column_channel: return backend()->getInterfaceName(msg.getInterfaceId());
            case column_direction: return msg.isRX() ? "RX" : "TX";
            case column_type: return msg.isFD() ? "fd" : "can";
            case column_canid: return QString("0x%1").arg(msg.getId(), 0, 16);
            case column_dlc: return msg.getLength();
            case column_data: return msg.getDataHexString().toLower();
            case column_name: 
                if (item->parentItem() != m_rootItem.get()) {
                    // This is a child row - show transport layer info
                    uint8_t firstByte = msg.getByte(0);
                    uint8_t type = (firstByte >> 4) & 0x0F;
                    if (type == 0x0) return "Single Frame";
                    if (type == 0x1) return "First Frame";
                    if (type == 0x2) return QString("Consecutive Frame (SN: %1)").arg(firstByte & 0x0F);
                    if (type == 0x3) return "Flow Control";
                }
                return (dbmsg) ? dbmsg->getName() : "";
            case column_comment: return (dbmsg) ? dbmsg->getComment() : "";
            case column_sender: return ""; // Child rows don't show sender as per requirements
            default: return QVariant();
        }
    }
}

QVariant UnifiedTraceViewModel::data_TextColorRole(const QModelIndex &index) const
{
    UnifiedTraceItem *item = static_cast<UnifiedTraceItem*>(index.internalPointer());
    if (item->isProtocol()) {
        const ProtocolMessage& pmsg = item->protocolMessage();
        switch (pmsg.type) {
            case MessageType::Request: return QColor(0, 0, 139); // Dark Blue #00008B
            case MessageType::PositiveResponse: return QColor(0, 100, 0); // Dark Green #006400
            case MessageType::NegativeResponse: return QColor(139, 0, 0); // Dark Red #8B0000
            default: break;
        }
    }
    const CanMessage& msg = item->rawFrame();
    if (msg.isErrorFrame()) return QColor(Qt::red);
    return QVariant();
}

QString UnifiedTraceViewModel::formatUnifiedTimestamp(uint64_t ts, uint64_t prevTs) const
{
    double val = 0;
    switch (timestampMode()) {
        case timestamp_mode_absolute:
            return QDateTime::fromMSecsSinceEpoch(ts / 1000).toString("HH:mm:ss.zzz");
        case timestamp_mode_relative:
            val = (double)(ts - (uint64_t)(backend()->getTimestampAtMeasurementStart() * 1000000.0)) / 1000000.0;
            break;
        case timestamp_mode_delta:
            val = (prevTs > 0) ? (double)(ts - prevTs) / 1000000.0 : 0.0;
            break;
        default:
            return "0.000000";
    }
    return QString::number(val, 'f', 6);
}
