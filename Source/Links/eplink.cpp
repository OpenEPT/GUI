#include "eplink.h"


#define EP_LINK_SERVER_PORT         8000
#define EP_LINK_BUFFER_SIZE         1500

EPLink::EPLink(QObject *parent)
    : QObject{parent}
{
    port = EP_LINK_SERVER_PORT;
    tcpServerThread = new QThread(this);
    tcpServerThread->setObjectName("EP server");
    connect(tcpServerThread, SIGNAL(started()),this,SLOT(onServerStarted()));
}

void EPLink::startServer()
{
    tcpServerThread->start();
}

quint16 EPLink::getPort()
{
    return port;
}

void EPLink::onServerStarted()
{
    tcpServer   = new QTcpServer();

    if(!tcpServer->listen(QHostAddress::Any, port))
    {
        qDebug()<<"EP Link Server Bind failed";
    }
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(onNewConnectionAdded()));

}

void EPLink::onNewConnectionAdded()
{
    QTcpSocket* tmpSocket;
    while(tcpServer->hasPendingConnections())
    {
        tmpSocket = tcpServer->nextPendingConnection();
        clientList.append(tmpSocket);
        connect(tmpSocket, SIGNAL(readyRead()), this, SLOT(onReadPendingData()));
        emit sigNewClientConnected(QHostAddress(tmpSocket->peerAddress().toIPv4Address()).toString());
    }
}

void EPLink::onReadPendingData()
{
    char message[EP_LINK_BUFFER_SIZE];
    unsigned int id = 0;
    memset(message, 0, EP_LINK_BUFFER_SIZE);
    QString clientIp;
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    clientIp = QHostAddress(clientSocket->peerAddress().toIPv4Address()).toString();
    while(clientSocket->read(message, EP_LINK_BUFFER_SIZE) != 0)
    {
        id = *((unsigned int*)&message[0]);
        emit sigNewEPNameReceived(id, QString(&message[4]));
    }
}
