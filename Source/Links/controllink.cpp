#include "controllink.h"
#include <QtGlobal>
#include <QHostAddress>

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
    ipAddress       =   "0.0.0.0";
    portNumber      =   0;
    linkStatus      =   CONTROL_LINK_STATUS_DISABLED;
    reconnectTimer  = new QTimer(this);
    connect(reconnectTimer, SIGNAL(timeout()), this, SLOT(reconnect()));

}

ControlLink::~ControlLink()
{
    if(linkStatus == CONTROL_LINK_STATUS_ESTABLISHED)
    {
        tcpSocket->disconnect();
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
    tcpSocket->connectToHost(ipAddress, portNumber);
    tcpSocket->waitForConnected(10);
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
