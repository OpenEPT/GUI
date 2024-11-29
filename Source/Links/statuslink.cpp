#include "statuslink.h"

StatusLink::StatusLink(QObject *parent)
    : QObject{parent}
{
    tcpServerThread = new QThread(this);
    tcpServerThread->setObjectName("Status link server");
    connect(tcpServerThread, SIGNAL(started()),this,SLOT(onServerStarted()));
}

void StatusLink::startServer()
{
    tcpServerThread->start();
}

void StatusLink::onServerStarted()
{
    tcpServer   = new QTcpServer();

    if(!tcpServer->listen(QHostAddress::Any, STATUS_LINK_SERVER_PORT))
    {
        qDebug()<<"Status Link Server Bind failed";
    }
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(onNewConnectionAdded()));

}

void StatusLink::onNewConnectionAdded()
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

void StatusLink::onReadPendingData()
{
    char message[STATUS_LINK_BUFFER_SIZE];
    memset(message, 0, STATUS_LINK_BUFFER_SIZE);
    QString clientIp;
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    clientIp = QHostAddress(clientSocket->peerAddress().toIPv4Address()).toString();
    while(clientSocket->read(message, STATUS_LINK_BUFFER_SIZE) != 0)
    {
        emit sigNewStatusMessageReceived(clientIp, QString(message));
    }
}
