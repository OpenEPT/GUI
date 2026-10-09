#include "controllink.h"
#include <QtGlobal>
#include <QHostAddress>
#include <QTimer>
#include <QEventLoop>

#ifdef Q_OS_WIN
#include "Ws2tcpip.h"
#include "WinSock2.h"
#elif defined(Q_OS_LINUX)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

ControlLink::ControlLink(QObject *parent)
    : QObject{parent}
{
    tcpSocket       =   new QTcpSocket(this);
    tcpSocket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    serialPort        = nullptr;
    transport         = CONTROL_LINK_TRANSPORT_TCP;
    serialResponseOk       = false;
    serialResponseGot      = false;
    serialResponseIsBinary = false;
    ipAddress       =   "0.0.0.0";
    serialPortName  =   "";
    portNumber      =   0;
    linkStatus      =   CONTROL_LINK_STATUS_DISABLED;
    reconnectTimer  = new QTimer(this);
    connect(reconnectTimer, SIGNAL(timeout()), this, SLOT(reconnect()));

}

ControlLink::~ControlLink()
{
    if(linkStatus == CONTROL_LINK_STATUS_ESTABLISHED)
    {
        if(transport == CONTROL_LINK_TRANSPORT_SERIAL)
        {
            if(serialPort != nullptr && serialPort->isOpen()) serialPort->close();
        }
        else
        {
            tcpSocket->disconnect();
        }
    }
}

control_link_status_t   ControlLink::establishLink(QString aIpAddress, QString aPortNumber)
{
    QHostAddress    hostAddress(aIpAddress);
    qint16          hostPort = aPortNumber.toUShort();
    linkStatus = CONTROL_LINK_STATUS_DISABLED;
    ipAddress = aIpAddress;
    portNumber = hostPort;
    connect(tcpSocket, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
    connect(tcpSocket, SIGNAL(connected()), this, SLOT(onReconnected()));
    tcpSocket->connectToHost(hostAddress, hostPort);
    tcpSocket->waitForConnected(1000);
    return linkStatus;
}

control_link_status_t   ControlLink::establishSerialLink(QString aPortName, qint32 aBaudRate)
{
    transport       = CONTROL_LINK_TRANSPORT_SERIAL;
    linkStatus      = CONTROL_LINK_STATUS_DISABLED;
    serialPortName  = aPortName;
    ipAddress       = aPortName;   /* reuse for display (getDeviceIP_Addr) */

    if(serialPort == nullptr)
    {
        serialPort = new QSerialPort(this);
        connect(serialPort, SIGNAL(errorOccurred(QSerialPort::SerialPortError)),
                this, SLOT(onSerialErrorOccurred(QSerialPort::SerialPortError)));
        connect(serialPort, SIGNAL(readyRead()), this, SLOT(onSerialReadyRead()));
    }

    if(serialPort->isOpen()) serialPort->close();

    serialRxBuffer.clear();
    serialResponseGot = false;

    serialPort->setPortName(aPortName);
    serialPort->setBaudRate(aBaudRate);                 /* ignored by USB CDC, required by API */
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if(!serialPort->open(QIODevice::ReadWrite))
    {
        linkStatus = CONTROL_LINK_STATUS_DISABLED;
        return linkStatus;
    }

    /* Assert DTR so the firmware's CDC driver enables the TX path (dtrActive) */
    serialPort->setDataTerminalReady(true);
    serialPort->clear();

    linkStatus = CONTROL_LINK_STATUS_ESTABLISHED;

    /* Mirror TCP timing: deliver sigConnected through the event loop, after the
     * caller has assigned this link to the Device and connected the signal. */
    QTimer::singleShot(0, this, [this](){ emit sigConnected(); });

    return linkStatus;
}

bool ControlLink::prvReadSerialResponse(QString* response, int timeout)
{
    /* Responses arrive asynchronously in onSerialReadyRead(), which sets
     * serialResponseGot and emits sigSerialResponseReady(). Wait for it (or the
     * timeout) via a local event loop, so readyRead keeps firing and any
     * asynchronous notifications continue to be dispatched meanwhile. */
    if(!serialResponseGot)
    {
        QEventLoop  loop;
        QTimer      timer;
        timer.setSingleShot(true);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        connect(this, &ControlLink::sigSerialResponseReady, &loop, &QEventLoop::quit);
        timer.start(timeout);
        loop.exec();
    }

    if(serialResponseGot)
    {
        if(!serialResponseIsBinary && response != nullptr)
            *response = serialResponsePayload;
        return serialResponseOk;
    }

    if(response != nullptr) *response = "Unable to read data";
    return false;
}

void ControlLink::onSerialReadyRead()
{
    if(serialPort == nullptr) return;

    serialRxBuffer += serialPort->readAll();

    for(;;)
    {
        /* Binary board-data response frame: 0xA5 0xA5 'R' <sizeLo> <sizeHi> <data> */
        if(serialRxBuffer.size() >= 3 &&
           (quint8)serialRxBuffer.at(0) == 0xA5 &&
           (quint8)serialRxBuffer.at(1) == 0xA5 &&
           serialRxBuffer.at(2) == 'R')
        {
            if(serialRxBuffer.size() < 5) return;          /* need the size field */
            quint16 size = (quint8)serialRxBuffer.at(3) | ((quint16)(quint8)serialRxBuffer.at(4) << 8);
            if(serialRxBuffer.size() < 5 + (int)size) return;  /* wait for all data */

            serialBinaryPayload    = serialRxBuffer.mid(5, size);
            serialRxBuffer.remove(0, 5 + size);
            serialResponseOk       = true;
            serialResponseGot      = true;
            serialResponseIsBinary = true;
            emit sigSerialResponseReady();
            continue;
        }

        int nlIndex = serialRxBuffer.indexOf('\n');
        if(nlIndex < 0) return;

        QByteArray lineBytes = serialRxBuffer.left(nlIndex);
        serialRxBuffer.remove(0, nlIndex + 1);

        QString line = QString::fromUtf8(lineBytes).trimmed();
        if(line.isEmpty()) continue;

        if(line.startsWith("STATUS"))
        {
            serialResponsePayload  = line.mid(6).trimmed();   /* payload after "STATUS" */
            serialResponseOk       = true;
            serialResponseGot      = true;
            serialResponseIsBinary = false;
            emit sigSerialResponseReady();
        }
        else if(line.startsWith("ERROR"))
        {
            serialResponsePayload  = "ERROR";
            serialResponseOk       = false;
            serialResponseGot      = true;
            serialResponseIsBinary = false;
            emit sigSerialResponseReady();
        }
        else
        {
            /* Asynchronous status-link notification (e.g. charger mode change) */
            emit sigAsyncMessage(line);
        }
    }
}

bool ControlLink::transferBdWrite(const QByteArray& data, int timeout)
{
    if(transport != CONTROL_LINK_TRANSPORT_SERIAL) return false;
    if(serialPort == nullptr || !serialPort->isOpen() || linkStatus != CONTROL_LINK_STATUS_ESTABLISHED) return false;

    quint16 size = (quint16)data.size();

    QByteArray frame;
    frame.append((char)0xA5);
    frame.append((char)0xA5);
    frame.append('W');
    frame.append((char)(size & 0xFF));
    frame.append((char)((size >> 8) & 0xFF));
    frame.append(data);

    serialResponseGot      = false;
    serialResponseIsBinary = false;

    if(serialPort->write(frame) == -1)              return false;
    if(!serialPort->waitForBytesWritten(timeout))   return false;

    QString response;
    if(!prvReadSerialResponse(&response, timeout))  return false;
    return (response == "OK");
}

bool ControlLink::transferBdRead(QByteArray* out, int timeout)
{
    if(out == nullptr) return false;
    if(transport != CONTROL_LINK_TRANSPORT_SERIAL) return false;
    if(serialPort == nullptr || !serialPort->isOpen() || linkStatus != CONTROL_LINK_STATUS_ESTABLISHED) return false;

    serialResponseGot      = false;
    serialResponseIsBinary = false;

    QByteArray cmd("charger bd read");
    cmd.append('\r');
    if(serialPort->write(cmd) == -1)                return false;
    if(!serialPort->waitForBytesWritten(timeout))   return false;

    QString dummy;
    if(!prvReadSerialResponse(&dummy, timeout))     return false;
    if(!serialResponseIsBinary)                     return false;   /* got an error line instead */

    *out = serialBinaryPayload;
    return true;
}

bool ControlLink::prvReadResponse(QString* response, int timeout)
{
    QByteArray receivedData;

    while(true)
    {
        if(!tcpSocket->waitForReadyRead(timeout))
        {
            *response = "Unable to read data";
            return false;
        }

        receivedData += tcpSocket->readAll();

        /* ===== MIN HEADER SIZE ===== */
        if(receivedData.size() < 4)
            continue;

        if(receivedData[0] != 'O' || receivedData[1] != 'K' || receivedData[2] != ' ')
        {
            *response = "ERROR";
            return false;
        }

        char format = receivedData[3];

        /* ===== HEX / TEXT MODE ===== */
        if(format == 'H')
        {
            int endIndex = receivedData.indexOf("\r\n");
            if(endIndex == -1)
                continue;

            QByteArray payload = receivedData.mid(4, endIndex - 4);
            *response = QString::fromUtf8(payload);
            return true;
        }

        /* ===== BINARY MODE ===== */
        else if(format == 'B')
        {
            /* need at least header + size */
            if(receivedData.size() < 6)
                continue;

            uint16_t payloadSize = ((uint8_t)receivedData[4] << 8) | (uint8_t)receivedData[5];

            int totalSize = 4 + 2 + payloadSize + 2; // OK B + size + payload + CRLF

            if(receivedData.size() < totalSize)
                continue;

            QByteArray payload = receivedData.mid(6, payloadSize);

            *response = payload.toHex().toUpper();
            return true;
        }

        else
        {
            *response = "Unknown format";
            return false;
        }
    }
}


void ControlLink::reconnect()
{
    if(transport == CONTROL_LINK_TRANSPORT_SERIAL) return;   /* serial auto-reconnect is Phase 2 */
    tcpSocket->connectToHost(ipAddress, portNumber);
    tcpSocket->waitForConnected(10);
}

void ControlLink::onSerialErrorOccurred(QSerialPort::SerialPortError error)
{
    if(error == QSerialPort::NoError) return;

    /* Treat fatal transport errors (device unplugged, etc.) as a disconnect. */
    if(error == QSerialPort::ResourceError ||
       error == QSerialPort::PermissionError ||
       error == QSerialPort::DeviceNotFoundError)
    {
        if(linkStatus != CONTROL_LINK_STATUS_DISABLED)
        {
            linkStatus = CONTROL_LINK_STATUS_DISABLED;
            if(serialPort != nullptr && serialPort->isOpen()) serialPort->close();
            emit sigDisconnected();
        }
    }
}

bool                    ControlLink::getDeviceName(QString *deviceName)
{
    QString response;
    if(!executeCommand(QString("device hello"), &response, CONTROL_LINK_COMMAND_TIMEOUT)) return false;
    *deviceName = response;
    return true;
}
bool ControlLink::executeCommand(QString command, QString* response, int timeout)
{
    if(response == nullptr)
        return false;

    response->clear();

    if(linkStatus != CONTROL_LINK_STATUS_ESTABLISHED)
    {
        *response = "Control link not established";
        return false;
    }

    /* ===== SERIAL (Charger): plain text terminated by CR ===== */
    if(transport == CONTROL_LINK_TRANSPORT_SERIAL)
    {
        if(serialPort == nullptr || !serialPort->isOpen())
        {
            *response = "Control link not established";
            return false;
        }

        /* Arm response capture before sending (onSerialReadyRead fills it in). */
        serialResponseGot = false;

        QByteArray packet = command.toUtf8();
        packet.append('\r');

        if(serialPort->write(packet) == -1)
        {
            *response = "Write failed";
            return false;
        }
        if(!serialPort->waitForBytesWritten(timeout))
        {
            *response = "Write timeout";
            return false;
        }

        return prvReadSerialResponse(response, timeout);
    }

    /* ===== TCP (EPP): binary 0xA5 framing ===== */
    QByteArray packet;

    /* HEADER */
    packet.append((char)0xA5);
    packet.append((char)0xA5);
    packet.append('H');

    /* PAYLOAD */
    packet.append(command.toUtf8());

    tcpSocket->flush();
    tcpSocket->write(packet);
    tcpSocket->waitForBytesWritten(timeout);

    return prvReadResponse(response, timeout);
}

bool ControlLink::executeCommand(QByteArray request, QString* response, int timeout)
{
    if(response == nullptr)
        return false;

    response->clear();

    if(linkStatus != CONTROL_LINK_STATUS_ESTABLISHED)
    {
        *response = "Control link not established";
        return false;
    }

    /* Binary (0xA5 'B') framing is EPP/TCP-only. The serial charger uses a
     * text BD protocol which is a Phase-2 item. */
    if(transport == CONTROL_LINK_TRANSPORT_SERIAL)
    {
        *response = "Binary commands not supported over serial";
        return false;
    }

    QByteArray packet;

    /* HEADER */
    packet.append((char)0xA5);
    packet.append((char)0xA5);
    packet.append('B');

    /* LENGTH (uint16_t, big endian) */
    uint16_t len = request.size();
    packet.append((char)(len & 0xFF));
    packet.append((char)((len >> 8) & 0xFF));

    /* PAYLOAD */
    packet.append(request);

    tcpSocket->flush();

    if(tcpSocket->write(packet) == -1)
    {
        *response = "Write failed";
        return false;
    }

    if(!tcpSocket->waitForBytesWritten(timeout))
    {
        *response = "Write timeout";
        return false;
    }

    return prvReadResponse(response, timeout);
}

QString ControlLink::getDeviceIP_Addr()
{
    return ipAddress;
}

quint16 ControlLink::getDeviceIP_Port()
{
    return portNumber;
}
bool   ControlLink::setSocketKeepAlive()
{
    char enableKeepAlive = 1;
    qintptr sd = tcpSocket->socketDescriptor();
    int response;
#ifdef Q_OS_WIN
     response = setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, &enableKeepAlive, sizeof(enableKeepAlive));
     if(response != 0) return false;

     int maxIdle = 1; /* seconds */
     response = setsockopt(sd, IPPROTO_TCP, TCP_KEEPIDLE, (const char*)&maxIdle, 4);
     if(response != 0) return false;

     int count = 1;  // send up to 2 keepalive packets out, then disconnect if no response
     response = setsockopt(sd, IPPROTO_TCP , TCP_KEEPCNT, (const char*)&count, 4);
     if(response != 0) return false;

     int interval = 2;   // send a keepalive packet out every 2 seconds (after the 5 second idle period)
     response = setsockopt(sd, IPPROTO_TCP, TCP_KEEPINTVL, (const char*)&interval, 4);
     if(response != 0) return false;
#elif defined(Q_OS_LINUX)
    if (setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, &enableKeepAlive, sizeof(enableKeepAlive)) < 0)
        return false;

    int maxIdle = 1;  // Seconds before starting to send keepalive probes
    if (setsockopt(sd, IPPROTO_TCP, TCP_KEEPIDLE, &maxIdle, sizeof(maxIdle)) < 0)
        return false;

    int count = 1;  // Number of keepalive probes before considering the connection dead
    if (setsockopt(sd, IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count)) < 0)
        return false;

    int interval = 2;  // Interval between individual keepalive probes
    if (setsockopt(sd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval)) < 0)
        return false;
#endif

    return true;
}
void ControlLink::onDisconnected()
{
    linkStatus = CONTROL_LINK_STATUS_DISABLED;
    emit sigDisconnected();
    linkStatus = CONTROL_LINK_STATUS_RECONNECTING;
    reconnectTimer->start(CONTROL_LINK_RECONNECT_PERIOD);
}

void ControlLink::onReconnected()
{
    if(linkStatus == CONTROL_LINK_STATUS_RECONNECTING)
    {
        reconnectTimer->stop();
    }
    linkStatus = CONTROL_LINK_STATUS_ESTABLISHED;
    setSocketKeepAlive();
    emit sigConnected();

}
