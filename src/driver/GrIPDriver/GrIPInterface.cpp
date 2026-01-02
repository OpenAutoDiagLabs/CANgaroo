/*

  Copyright (c) 2022 Ethan Zonca

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

#include "GrIPInterface.h"
#include "qapplication.h"
#include "qdebug.h"

#include <core/Backend.h>
#include <core/MeasurementInterface.h>
#include <core/CanMessage.h>

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QThread>

#include "GrIP/GrIPHandler.h"


GrIPInterface::GrIPInterface(GrIPDriver *driver, int index, GrIPHandler *hdl, QString name, bool fd_support, uint32_t manufacturer)
  : CanInterface((CanDriver *)driver),
    _manufacturer(manufacturer),
    _idx(index),
    _isOpen(false),
    _isOffline(false),
    _serport(NULL),
    _name(name),
    _ts_mode(ts_mode_SIOCSHWTSTAMP),
    m_GrIPHandler(hdl)
{
    // Set defaults
    _settings.setBitrate(500000);
    _settings.setSamplePoint(875);

    _config.supports_canfd = fd_support;
    _config.supports_timing = false;

    if(fd_support)
    {
        _settings.setFdBitrate(2000000);
        _settings.setFdSamplePoint(750);
    }

    _status.can_state = state_bus_off;
    _status.rx_count = 0;
    _status.rx_errors = 0;
    _status.rx_overruns = 0;
    _status.tx_count = 0;
    _status.tx_errors = 0;
    _status.tx_dropped = 0;

    _readMessage_datetime = QDateTime::currentDateTime();

    _readMessage_datetime_run = QDateTime::currentDateTime();

    m_TxFrames.clear();
}

GrIPInterface::~GrIPInterface()
{
}

QString GrIPInterface::getDetailsStr() const
{
    if(_manufacturer == CANIL)
    {
        if(_config.supports_canfd)
        {
            return "CANIL with CANFD support";
        }
        else
        {
            return "CANIL with standard CAN support";
        }
    }
    else
    {
        return "Not Supported";
    }
}

QString GrIPInterface::getName() const
{
	return _name;
}

void GrIPInterface::setName(QString name)
{
    _name = name;
}

QList<CanTiming> GrIPInterface::getAvailableBitrates()
{
    QList<CanTiming> retval;
    QList<unsigned> bitrates;
    QList<unsigned> bitrates_fd;

    QList<unsigned> samplePoints;
    QList<unsigned> samplePoints_fd;

    if(_manufacturer == GrIPInterface::CANIL)
    {
        bitrates.append({10000, 20000, 50000, 83333, 100000, 125000, 250000, 500000, 800000, 1000000});
        bitrates_fd.append({2000000, 5000000});
        samplePoints.append({875});
        samplePoints_fd.append({750});
    }
    /*else if(_manufacturer == WeActStudio)
    {
    }*/

    unsigned i=0;
    foreach (unsigned br, bitrates)
    {
        foreach(unsigned br_fd, bitrates_fd)
        {
            foreach (unsigned sp, samplePoints)
            {
                foreach (unsigned sp_fd, samplePoints_fd)
                {
                    retval << CanTiming(i++, br, br_fd, sp,sp_fd);
                }
            }
        }
    }

    return retval;
}

void GrIPInterface::applyConfig(const MeasurementInterface &mi)
{
    // Save settings for port configuration
    _settings = mi;
}

bool GrIPInterface::updateStatus()
{
    return false;
}

bool GrIPInterface::readConfig()
{
    return false;
}

bool GrIPInterface::readConfigFromLink(rtnl_link *link)
{
    Q_UNUSED(link);
    return false;
}

bool GrIPInterface::supportsTimingConfiguration()
{
    return _config.supports_timing;
}

bool GrIPInterface::supportsCanFD()
{
    return _config.supports_canfd;
}

bool GrIPInterface::supportsTripleSampling()
{
    return false;
}

unsigned GrIPInterface::getBitrate()
{
    return _settings.bitrate();
}

uint32_t GrIPInterface::getCapabilities()
{
    uint32_t retval = 0;

    if(_manufacturer == GrIPInterface::CANIL)
    {
        retval =
            CanInterface::capability_auto_restart |
            CanInterface::capability_listen_only;
    }
    /*else if(_manufacturer == WeActStudio)
    {
        retval =
            // CanInterface::capability_config_os |
            // CanInterface::capability_auto_restart |
            CanInterface::capability_listen_only |
            CanInterface::capability_custom_bitrate |
            CanInterface::capability_custom_canfd_bitrate;
    }*/

    if (supportsCanFD())
    {
        retval |= CanInterface::capability_canfd;
    }

    if (supportsTripleSampling())
    {
        retval |= CanInterface::capability_triple_sampling;
    }

    return retval;
}

bool GrIPInterface::updateStatistics()
{
    return updateStatus();
}

uint32_t GrIPInterface::getState()
{
    return _status.can_state;
}

int GrIPInterface::getNumRxFrames()
{
    return _status.rx_count;
}

int GrIPInterface::getNumRxErrors()
{
    return _status.rx_errors;
}

int GrIPInterface::getNumTxFrames()
{
    return _status.tx_count;
}

int GrIPInterface::getNumTxErrors()
{
    return _status.tx_errors;
}

int GrIPInterface::getNumRxOverruns()
{
    return _status.rx_overruns;
}

int GrIPInterface::getNumTxDropped()
{
    return _status.tx_dropped;
}

int GrIPInterface::getIfIndex()
{
    return _idx;
}

QString GrIPInterface::getVersion()
{
    return _version;
}

void GrIPInterface::open()
{
    /*
    if (_serport->open(QIODevice::ReadWrite))
    {
        //perror("Serport connected!");
        qRegisterMetaType<QSerialPort::SerialPortError>("SerialThread");
        //connect(_serport, static_cast<void (QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error),  this, &SLCANInterface::handleSerialError);
        //connect(_serport, SIGNAL(readyRead()),this,SLOT(serport_readyRead()));
    }
    else
    {
        perror("Serport connect failed!");
        _serport_mutex.unlock();
        _isOpen = false;
        _isOffline = true;
        return;
    }*/

    if(m_GrIPHandler == nullptr)
    {
        _isOpen = false;
        _isOffline = true;
        return;
    }

    // Get Version
    for(int i = 0; i < 15; i++)
    {
        _version = QString::fromStdString(m_GrIPHandler->GetVersion());
        if(_version.size() == 0)
        {
            QThread::msleep(2);
        }
        else
        {
            break;
        }
    }

    // Close CAN port
    /*_serport->write("C\r", 2);
    _serport->flush();
    _serport->waitForBytesWritten(100);
    _serport->waitForReadyRead(50);*/

    // Get Version
    /*_serport->clear(QSerialPort::Input);
    _serport->write("V\r", 2);
    _serport->flush();
    _serport->waitForBytesWritten(100);*/
    /*if(_serport->waitForReadyRead(50))
    {
        qApp->processEvents();

        if(_serport->bytesAvailable())
        {
            // This is called when readyRead() is emitted
            QByteArray datas = _serport->readLine();
            _version = QString(datas).trimmed();
        }
    }*/

    /*if(_settings.isCustomBitrate())
    {
        QString _custombitrate = QString("%1").arg(_settings.customBitrate(), 6, 16,QLatin1Char('0')).toUpper();
        std::string _custombitrate_std= 'S' + _custombitrate.toStdString() + '\r';
        _serport->write(_custombitrate_std.c_str(), _custombitrate_std.length());
        _serport->flush();
    }
    else
    {
        // Set the classic CAN bitrate
        switch(_settings.bitrate())
        {
            case 1000000:
                _serport->write("S8\r", 3);
                _serport->flush();
                break;
            case 800000:
                _serport->write("S7\r", 3);
                _serport->flush();
                break;
            case 500000:
                _serport->write("S6\r", 3);
                _serport->flush();
                break;
            case 250000:
                _serport->write("S5\r", 3);
                _serport->flush();
                break;
            case 125000:
                _serport->write("S4\r", 3);
                _serport->flush();
                break;
            case 100000:
                _serport->write("S3\r", 3);
                _serport->flush();
                break;
            case 83333:
                _serport->write("S9\r", 3);
                _serport->flush();
                break;
            case 75000:
                _serport->write("SA\r", 3);
                _serport->flush();
                break;
            case 62500:
                _serport->write("SB\r", 3);
                _serport->flush();
                break;
            case 50000:
                _serport->write("S2\r", 3);
                _serport->flush();
                break;
            case 33333:
                _serport->write("SC\r", 3);
                _serport->flush();
                break;
            case 20000:
                _serport->write("S1\r", 3);
                _serport->flush();
                break;
            case 10000:
                _serport->write("S0\r", 3);
                _serport->flush();
                break;
            case 5000:
                _serport->write("SD\r", 3);
                _serport->flush();
                break;
            default:
                // Default to 10k
                _serport->write("S0\r", 3);
                _serport->flush();
                break;
        }
    }

    _serport->waitForBytesWritten(200);

    // Set configured BRS rate
    if(_config.supports_canfd)
    {
        if(_settings.isCustomFdBitrate())
        {
            QString _customfdbitrate = QString("%1").arg(_settings.customFdBitrate(), 6, 16,QLatin1Char('0')).toUpper();
            std::string _customfdbitrate_std= 'Y' + _customfdbitrate.toStdString() + '\r';
            _serport->write(_customfdbitrate_std.c_str(), _customfdbitrate_std.length());
            _serport->flush();
        }
        else
        {
            switch(_settings.fdBitrate())
            {
                case 1000000:
                    _serport->write("Y1\r", 3);
                    _serport->flush();
                    break;
                case 2000000:
                    _serport->write("Y2\r", 3);
                    _serport->flush();
                    break;
                case 3000000:
                    _serport->write("Y3\r", 3);
                    _serport->flush();
                    break;
                case 4000000:
                    _serport->write("Y4\r", 3);
                    _serport->flush();
                    break;
                case 5000000:
                    _serport->write("Y5\r", 3);
                    _serport->flush();
                    break;
            }
        }
    }
    _serport->waitForBytesWritten(100);

    // Set Listen Only Mode
    if(_settings.isListenOnlyMode())
    {
        _serport->write("M1\r", 3);
        _serport->flush();
    }
    else
    {
        _serport->write("M0\r", 3);
        _serport->flush();
    }
    _serport->waitForBytesWritten(100);

    // Open the port
    _serport->write("O\r", 2);
    _serport->flush();
    _serport->waitForBytesWritten(100);

    // Clear serial port receiver
    if(_serport->waitForReadyRead(10))
    {
        qApp->processEvents();

        if(_serport->bytesAvailable())
        {
            // This is called when readyRead() is emitted
            _serport->readAll();
        }
    }*/

    m_GrIPHandler->EnableChannel(_idx, true);
    m_TxFrames.clear();

    _isOpen = true;
    _isOffline = false;
    _status.can_state = state_ok;
    _status.rx_count = 0;
    _status.rx_errors = 0;
    _status.rx_overruns = 0;
    _status.tx_count = 0;
    _status.tx_errors = 0;
    _status.tx_dropped = 0;

    // Release port mutex
    //_serport_mutex.unlock();
}

void GrIPInterface::handleSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError)
    {
        perror("error");

        _isOffline = true;
    }

    QString  ERRORString = "";
    switch (error) {
    case QSerialPort::NoError:
        ERRORString=  "No Error";
        break;
    case QSerialPort::DeviceNotFoundError:
        ERRORString= "Device Not Found";
        break;
    case QSerialPort::PermissionError:
        ERRORString= "Permission Denied";
        break;
    case QSerialPort::OpenError:
        ERRORString= "Open Error";
        break;
    /*case QSerialPort::ParityError:
        ERRORString= "Parity Error";
        break;
    case QSerialPort::FramingError:
        ERRORString= "Framing Error";
        break;
    case QSerialPort::BreakConditionError:
        ERRORString= "Break Condition";
        break;
    case QSerialPort::WriteError:
        ERRORString= "Write Error";
        break;*/
    case QSerialPort::ReadError:
        ERRORString= "Read Error";
        break;
    case QSerialPort::ResourceError:
        ERRORString= "Resource Error";
        break;
    case QSerialPort::UnsupportedOperationError:
        ERRORString= "Unsupported Operation";
        break;
    case QSerialPort::UnknownError:
        ERRORString= "Unknown Error";
        break;
    case QSerialPort::TimeoutError:
        //ERRORString= "Timeout Error";
        break;
    case QSerialPort::NotOpenError:
        ERRORString= "Not Open Error";
        break;
    default:
        ERRORString= "Other Error";
    }
    if(ERRORString.size())
        std::cout << "SerialPortWorker::errorOccurred  ,info is  " << ERRORString.toStdString() << std::endl;
}

void GrIPInterface::close()
{
    _isOpen = false;
    _status.can_state = state_bus_off;

    m_GrIPHandler->EnableChannel(_idx, false);

    m_TxFrames.clear();
}

bool GrIPInterface::isOpen()
{
    return _isOpen;
}

void GrIPInterface::sendMessage(const CanMessage &msg)
{
    _serport_mutex.lock();

    if(m_GrIPHandler->CanTransmit(_idx, msg))
    {
        _status.tx_count++;
        _status.can_state = state_tx_success;

        if(msg.isShow())
        {
            m_TxFrames.append(msg);
        }
    }
    else
    {
        _status.tx_errors++;
        _status.can_state = state_tx_fail;
    }

    _serport_mutex.unlock();
}

bool GrIPInterface::readMessage(QList<CanMessage> &msglist, unsigned int timeout_ms)
{
    QDateTime datetime;

    Q_UNUSED(timeout_ms);

    datetime = QDateTime::currentDateTime();
    if(datetime.toMSecsSinceEpoch() - _readMessage_datetime_run.toMSecsSinceEpoch() >= 1)
    {
        _readMessage_datetime_run = QDateTime::currentDateTime().addMSecs(1);
    }
    else
    {
        return false;
    }

    // Add TX frames to trace window
    if(m_TxFrames.size())
    {
        msglist.append(m_TxFrames);
        m_TxFrames.clear();
    }

    // Read all RX frames
    while(m_GrIPHandler->CanAvailable(_idx))
    {
        auto msg = m_GrIPHandler->ReceiveCan(_idx);
        if(msg.getId() != 0)
        {
            msg.setErrorFrame(0);
            msg.setInterfaceId(getId());
            msg.setBRS(false);

            msglist.append(msg);
            _status.rx_count++;
        }
    }

    // Don't saturate the thread. Read the buffer every 1ms.
    QThread().msleep(1);

    if(_isOffline == true)
    {
        if(_isOpen)
            close();

        return false;
    }
    else
    {
        datetime = QDateTime::currentDateTime();
        if(datetime.toMSecsSinceEpoch() - _readMessage_datetime.toMSecsSinceEpoch() > 3000)
        {
            _status.can_state = state_ok;
        }
    }

    // RX doesn't work on windows unless we call this for some reason
    /*_rxbuf_mutex.lock();
    if(_serport->waitForReadyRead(0))
    {
        qApp->processEvents();
    }
    _rxbuf_mutex.unlock();*/

    return true;
}

/*bool GrIPInterface::parseMessage(CanMessage &msg)
{
    // Set timestamp to current time
    struct timeval tv;
    gettimeofday(&tv,NULL);
    msg.setTimestamp(tv);

    // Defaults
    msg.setErrorFrame(0);
    msg.setInterfaceId(getId());
    msg.setId(0);
    msg.setRTR(false);
    msg.setFD(false);
    msg.setBRS(false);
    msg.setRX(true);

    // Convert from ASCII (2nd character to end)
    for (int i = 1; i < _rx_linbuf_ctr; i++)
    {
        // Lowercase letters
        if(_rx_linbuf[i] >= 'a')
            _rx_linbuf[i] = _rx_linbuf[i] - 'a' + 10;
        // Uppercase letters
        else if(_rx_linbuf[i] >= 'A')
            _rx_linbuf[i] = _rx_linbuf[i] - 'A' + 10;
        // Numbers
        else
            _rx_linbuf[i] = _rx_linbuf[i] - '0';
    }

    bool is_extended = false;
    bool is_rtr = false;

    // Handle each incoming command
    switch(_rx_linbuf[0])
    {
        // Transmit data frame command
        case 't':
        {
            is_extended = false;
        }
        break;
        case 'T':
        {
            is_extended = true;
        }
        break;

        // Transmit remote frame command
        case 'r':
        {
            is_extended = false;
            is_rtr = true;
        }
        break;
        case 'R':
        {
            is_extended = true;
            is_rtr = true;
        }
        break;

        // CANFD transmit - no BRS
        case 'd':
        {
            is_extended = false;
            msg.setFD(true);
            msg.setBRS(false);
        }
        break;
        case 'D':
        {
            is_extended = true;
            msg.setFD(true);
            msg.setBRS(false);
        }
        break;

        // CANFD transmit - with BRS
        case 'b':
        {
            is_extended = false;
            msg.setFD(true);
            msg.setBRS(true);
        }
        break;
        case 'B':
        {
            is_extended = true;
            msg.setFD(true);
            msg.setBRS(true);
        }
        break;

        // Invalid command
        default:
        {
            // Reset buffer
            _rx_linbuf_ctr = 0;
            _rx_linbuf[0] = '\0';
            return false;
        }
    }

    // Start parsing at second byte (skip command byte)
    uint8_t parse_loc = 1;

    // Default to standard id len
    uint8_t id_len = GRIP_STD_ID_LEN;

    // Update length if message is extended ID
    if(is_extended)
        id_len = GRIP_EXT_ID_LEN;

    uint32_t id_tmp = 0;

    // Iterate through ID bytes
    while(parse_loc <= id_len)
    {
        id_tmp <<= 4;
        id_tmp += _rx_linbuf[parse_loc++];
    }

    msg.setId(id_tmp);
    msg.setExtended(is_extended);
    msg.setRTR(is_rtr);

    // Attempt to parse DLC and check sanity
    uint8_t dlc_code_raw = _rx_linbuf[parse_loc++];

    // If dlc is too long for an FD frame
    if(msg.isFD() && dlc_code_raw > 0xF)
    {
        return false;
    }
    if(!msg.isFD() && dlc_code_raw > 0x8)
    {
        return false;
    }

    if(dlc_code_raw > 0x8)
    {
        switch(dlc_code_raw)
        {
        case 0x9:
            dlc_code_raw = 12;
            break;
        case 0xA:
            dlc_code_raw = 16;
            break;
        case 0xB:
            dlc_code_raw = 20;
            break;
        case 0xC:
            dlc_code_raw = 24;
            break;
        case 0xD:
            dlc_code_raw = 32;
            break;
        case 0xE:
            dlc_code_raw = 48;
            break;
        case 0xF:
            dlc_code_raw = 64;
            break;
        default:
            dlc_code_raw = 0;
            perror("Invalid length");
            break;
        }
    }

    msg.setLength(dlc_code_raw);

    // Calculate number of bytes we expect in the message
    int8_t bytes_in_msg = dlc_code_raw;

    if(bytes_in_msg < 0)
    {
        perror("Invalid length < 0");
        return false;
    }
    if(bytes_in_msg > 64)
    {
        perror("Invalid length > 64");
        return false;
    }

    // Parse data
    // TODO: Guard against walking off the end of the string!
    for (uint8_t i = 0; i < bytes_in_msg; i++)
    {
        msg.setByte(i,  (_rx_linbuf[parse_loc] << 4) + _rx_linbuf[parse_loc+1]);
        parse_loc += 2;
    }

    // Reset buffer
    _rx_linbuf_ctr = 0;
    _rx_linbuf[0] = '\0';

    return true;
}*/
