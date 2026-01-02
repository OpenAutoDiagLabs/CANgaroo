#include "GrIPHandler.h"
#include "CRC.h"
#include <chrono>
#include <cstring>


#define SYSTEM_REPORT_INFO      0u
#define SYSTEM_SET_STATUS       1u

#define SYSTEM_SEND_CAN_CFG     10u
#define SYSTEM_SEND_LIN_CFG     11u
#define SYSTEM_START_CAN        12u
#define SYSTEM_START_LIN        13u
#define SYSTEM_ADD_CAN_FRAME    14u
#define SYSTEM_ADD_LIN_FRAME    15u
#define SYSTEM_SEND_CAN_FRAME   20u

// CAN Msg flags
#define CAN_FLAGS_STD_ID            0x01
#define CAN_FLAGS_EXT_ID            0x02
#define CAN_FLAGS_FD                0x04
#define CAN_FLAGS_RTR               0x08


GrIPHandler::GrIPHandler(const QString &name)
{
    m_Exit = false;
    m_ChannelsCAN = 0;
    m_ChannelsCANFD = 0;

    CRC_Init();

    m_SerialPort = new QSerialPort();
    m_SerialPort->setPortName(name);
    m_SerialPort->setBaudRate(1000000);
    m_SerialPort->setDataBits(QSerialPort::Data8);
    m_SerialPort->setParity(QSerialPort::NoParity);
    m_SerialPort->setStopBits(QSerialPort::OneStop);
    m_SerialPort->setFlowControl(QSerialPort::NoFlowControl);
    m_SerialPort->setReadBufferSize(2048);

    GrIP_Init(*m_SerialPort);
}


GrIPHandler::~GrIPHandler()
{
    Stop();
    delete m_SerialPort;
}


bool GrIPHandler::Start()
{
    std::unique_lock<std::mutex> lck(m_MutexSerial);

    if(!m_SerialPort->open(QIODevice::ReadWrite))
    {
        perror("Serport connect failed!");
        return false;
    }

    m_SerialPort->flush();
    m_SerialPort->clear();

    m_Exit = false;
    m_pWorkerThread = std::make_unique<std::thread>(std::thread(&GrIPHandler::WorkerThread, this));

    return true;
}


void GrIPHandler::Stop()
{
    m_Exit = true;

    std::unique_lock<std::mutex> lck(m_MutexSerial);

    if(m_pWorkerThread->joinable())
    {
        m_pWorkerThread->join();
    }
    m_pWorkerThread.reset();

    if (m_SerialPort->isOpen())
    {
        m_SerialPort->waitForBytesWritten(20);
        m_SerialPort->waitForReadyRead(10);
        m_SerialPort->clear();
        m_SerialPort->close();
    }
}


void GrIPHandler::RequestVersion()
{
    uint8_t msg = SYSTEM_REPORT_INFO;
    GrIP_Pdu_t p = {&msg, 1};

    // Clear previous version
    m_Version.clear();

    m_ChannelsCAN = 0;
    m_ChannelsCANFD = 0;

    std::unique_lock<std::mutex> lck(m_MutexSerial);

    GrIP_Transmit(PROT_GrIP, MSG_SYSTEM_CMD, RET_OK, &p);
    m_SerialPort->waitForReadyRead(10);
}


std::string GrIPHandler::GetVersion() const
{
    return m_Version;
}


int GrIPHandler::Channels_CAN() const
{
    return m_ChannelsCAN;
}


int GrIPHandler::Channels_CANFD() const
{
    return m_ChannelsCANFD;
}


void GrIPHandler::Send(GrIP_ProtocolType_e ProtType, GrIP_MessageType_e MsgType, GrIP_ReturnType_e ReturnCode, const GrIP_Pdu_t *pdu)
{
    //SendPacket_t packet = {ProtType, MsgType, ReturnCode, *pdu};

    std::unique_lock<std::mutex> lck(m_MutexSerial);
    GrIP_Transmit(ProtType, MsgType, ReturnCode, pdu);
}


void GrIPHandler::Send(GrIP_ProtocolType_e ProtType, GrIP_MessageType_e MsgType, GrIP_ReturnType_e ReturnCode, const uint8_t *data, uint16_t len)
{
    GrIP_Pdu_t pdu = {(uint8_t*)data, len};
    Send(ProtType, MsgType, ReturnCode, &pdu);
}


void GrIPHandler::EnableChannel(uint8_t ch, bool enable)
{
    if(ch < m_Channel_StatusCAN.size())
    {
        m_Channel_StatusCAN[ch] = enable;
    }
}


bool GrIPHandler::CanAvailable(uint8_t ch) const
{
    std::unique_lock<std::mutex> lck(m_MutexCanQueue);

    if(m_ReceiveQueue.size() > ch)
    {
        return (m_ReceiveQueue[ch].size() > 0);
    }

    return false;
}


CanMessage GrIPHandler::ReceiveCan(uint8_t ch)
{
    std::unique_lock<std::mutex> lck(m_MutexCanQueue);

    if(m_ReceiveQueue.size() > ch)
    {
        if(m_ReceiveQueue[ch].size())
        {
            auto front = m_ReceiveQueue[ch].front();
            m_ReceiveQueue[ch].pop();
            return front;
        }
    }

    return CanMessage();
}


bool GrIPHandler::CanTransmit(uint8_t ch, const CanMessage &msg)
{
    uint8_t arr[21] = {};
    GrIP_Pdu_t p = {arr, 21};

    uint32_t ID = msg.getId();

    // Set cmd
    arr[0] = SYSTEM_SEND_CAN_FRAME;

    arr[1] = ch;

    arr[2] = (ID >> 24) & 0xFF;
    arr[3] = (ID >> 16) & 0xFF;
    arr[4] = (ID >> 8) & 0xFF;
    arr[5] = (ID) & 0xFF;

    arr[6] = msg.getLength();

    arr[7] = 0;//can->Flags;
    if(msg.isExtended())
    {
        arr[7] |= CAN_FLAGS_EXT_ID;
    }
    if(msg.isFD())
    {
        arr[7] |= CAN_FLAGS_FD;
    }
    if(msg.isRTR())
    {
        arr[7] |= CAN_FLAGS_RTR;
    }

    /*msg[8] = (can->Time >> 24) & 0xFF;
    msg[9] = (can->Time >> 16) & 0xFF;
    msg[10] = (can->Time >> 8) & 0xFF;
    msg[11] = (can->Time) & 0xFF;*/

    for(int i = 0; i < msg.getLength(); i++)
    {
        arr[12+i] = msg.getByte(i);
    }

    std::unique_lock<std::mutex> lck(m_MutexSerial);

    return (GrIP_Transmit(PROT_GrIP, MSG_SYSTEM_CMD, RET_OK, &p) == 0);
}


void GrIPHandler::ProcessData(GrIP_Packet_t &packet)
{
    switch(packet.RX_Header.MsgType)
    {
    case MSG_SYSTEM_CMD:
        switch(packet.Data[0])
        {
        case 0:
        {
            // System info
            uint8_t major = packet.Data[1];
            uint8_t minor = packet.Data[2];
            uint8_t hw = packet.Data[3];

            uint8_t can = packet.Data[4];
            uint8_t canfd = packet.Data[5];
            /*uint8_t lin = packet.Data[6];
            uint8_t adc = packet.Data[7];
            uint8_t gpio = packet.Data[8];*/

            char date[128] = {};
            strncpy(date, (char*)&packet.Data[9], 120);

            char buffer[256]{};
            sprintf(buffer, "%d.%d-<%s>", major, minor, date);
            m_Version = buffer;

            m_ChannelsCAN = can;
            m_ChannelsCANFD = canfd;

            m_ReceiveQueue.clear();
            m_Channel_StatusCAN.clear();

            for(int i = 0; i < can; i++)
            {
                m_Channel_StatusCAN.push_back(false);
                m_ReceiveQueue.push_back({});
            }
            for(int i = 0; i < canfd; i++)
            {
                m_Channel_StatusCAN.push_back(false);
                m_ReceiveQueue.push_back({});
            }

            //fprintf(stderr, "SYS INFO: %s\n", m_Version.c_str());
            break;
        }

        case 1:
            break;

        case 2:
            break;

        default:
            break;
        }
        break;

    case MSG_REALTIME_CMD:
        break;

    case MSG_DATA:
    case MSG_DATA_NO_RESPONSE:
        switch(packet.Data[0])
        {
        case 0: // DATA_REPORT_CAN_MSG
        {
            uint8_t ch = packet.Data[1];

            //uint32_t time = packet.Data[2]<<24 | packet.Data[3]<<16 | packet.Data[4]<<8 | packet.Data[5];

            uint32_t id = packet.Data[6]<<24 | packet.Data[7]<<16 | packet.Data[8]<<8 | packet.Data[9];

            uint8_t dlc = packet.Data[10];
            uint8_t flags = packet.Data[11];

            uint8_t data[8] = {};
            memcpy(data, &packet.Data[12], dlc);

            struct timeval tv;
            gettimeofday(&tv,NULL);

            CanMessage msg(id);
            msg.setLength(dlc);
            msg.setRX(true);
            msg.setTimestamp(tv);

            if(flags & CAN_FLAGS_EXT_ID)
            {
                msg.setExtended(true);
            }
            if(flags & CAN_FLAGS_FD)
            {
                msg.setFD(true);
            }
            if(flags & CAN_FLAGS_RTR)
            {
                msg.setRTR(true);
            }

            for(int i = 0; i < dlc; i++)
            {
                msg.setByte(i, data[i]);
            }

            std::unique_lock<std::mutex> lck(m_MutexCanQueue);

            if(m_Channel_StatusCAN[ch])
            {
                m_ReceiveQueue[ch].push(msg);
            }

            break;
        }

        case 1: // DATA_REPORT_LIN_MSG
        {
            /*uint8_t ch = packet.Data[1] + 1;

            uint32_t time = packet.Data[2]<<24 | packet.Data[3]<<16 | packet.Data[4]<<8 | packet.Data[5];

            uint8_t alive = packet.Data[6];
            uint8_t valid = packet.Data[7];

            uint8_t id = packet.Data[8];
            uint8_t len = packet.Data[9];
            //uint8_t crc = dat.Data[10];

            uint8_t data[8] = {};
            memcpy(data, &packet.Data[11], len);

            QVector<std::string> v;
            for(uint8_t i: data)
            {
                v.append(uint8_to_hex(i));
            }*/

            fprintf(stderr, "LIN MSG\n");
            break;
        }

        default:
            break;
        }
        break;

    case MSG_NOTIFICATION:
    {
        uint8_t type = packet.Data[0];
        Q_UNUSED(type);
        fprintf(stderr, "DEV: %s\n", (char*)&packet.Data[1]);

        break;
    }

    case MSG_RESPONSE:
        break;

    case MSG_ERROR:
        break;

    default:
        break;
    }
}


void GrIPHandler::WorkerThread()
{
    while(!m_Exit)
    {
        // Update GrIP
        for(int i = 0; i < 32; i++)
            GrIP_Update();

        // Check for new packet
        GrIP_Packet_t dat;
        if(GrIP_Receive(&dat))
        {
            ProcessData(dat);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}
