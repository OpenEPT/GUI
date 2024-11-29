#include "controllink.h"
#include "Ws2tcpip.h"
#include "WinSock2.h"
#include <QHostAddress>

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

void ControlLink::reconnect()
{
    tcpSocket->connectToHost(ipAddress, portNumber);
    tcpSocket->waitForConnected(10);
}

bool                    ControlLink::getDeviceName(QString *deviceName)
{
    QString response;
    if(!executeCommand("device hello", &response, CONTROL_LINK_COMMAND_TIMEOUT)) return false;
    *deviceName = response;
    return true;
}
bool                    ControlLink::executeCommand(QString command, QString* response, int timeout)
{
    QByteArray  receivedData;
    QByteArray  dataToSend(command.toUtf8());
    QString     receivedResponse = "";
    receivedData.clear();
    if(linkStatus != CONTROL_LINK_STATUS_ESTABLISHED)
    {
        *response = QString("Control link not established");
        return false;
    }
    tcpSocket->flush();
    tcpSocket->write(dataToSend);
    tcpSocket->waitForBytesWritten();
    if(tcpSocket->waitForReadyRead(timeout) != true)
    {
        *response = QString("Unable to read data");
        return false;
    }
    receivedData = tcpSocket->readAll();
    QString responseAsString(receivedData);
    /* Check did we receive "\r\n" */
    if(!responseAsString.contains("\r\n"))
    {
        *response = QString("End of command not detected");
        return false;
    }
    /* Split response to identify OK*/
    QStringList responseParts = responseAsString.split(" ");
    if(responseParts[0] != "OK")
    {
        *response = QString("ERROR");
        return false;
    }
    for(int i = 1; i < responseParts.size(); i++)
    {
        *response += responseParts[i];
        if((i+1) !=responseParts.size())
        {
            *response += " ";
        }
    }
    /*take substring until*/
    *response = (*response).split("\r\n")[0];
    return true;
}
bool   ControlLink::setSocketKeepAlive()
{
    char enableKeepAlive = 1;
    qintptr sd = tcpSocket->socketDescriptor();
    int response;

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
