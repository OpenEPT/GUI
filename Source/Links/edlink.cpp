#include "edlink.h"


#define EP_LINK_SERVER_PORT         8000
#define EP_LINK_BUFFER_SIZE         1500

EDLink::EDLink(QObject *parent)
    : QObject{parent}
{
    port = EP_LINK_SERVER_PORT;
    tcpServerThread = new QThread(this);
    tcpServerThread->setObjectName("OpenEPT - EP server");
    connect(tcpServerThread, SIGNAL(started()),this,SLOT(onServerStarted()));
}

void EDLink::startServer()
{
    tcpServerThread->start();
}

quint16 EDLink::getPort()
{
    return port;
}

void EDLink::onServerStarted()
{
    tcpServer   = new QTcpServer();

    if(!tcpServer->listen(QHostAddress::Any, port))
    {
        qDebug()<<"EP Link Server Bind failed";
    }
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(onNewConnectionAdded()));

}

void EDLink::onNewConnectionAdded()
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

void EDLink::onReadPendingData()
{
    static QByteArray buffer;  // Buffer to store incomplete messages
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;

    buffer.append(clientSocket->readAll()); // Read all available data into buffer

    while (true) {
        int endIndex = buffer.indexOf("\r\n"); // Find message delimiter
        if (endIndex == -1) {
            // No complete message found, wait for more data
            break;
        }

        // Extract a full message
        QByteArray message = buffer.left(endIndex);
        buffer.remove(0, endIndex + 2); // Remove processed message from buffer

        if (message.size() < 8) {
            qWarning() << "Received incomplete header, discarding";
            continue;
        }

        unsigned int id = *reinterpret_cast<unsigned int*>(message.data());
        unsigned int dmaID = *reinterpret_cast<unsigned int*>(message.data() + 4);
        QString payload = QString::fromUtf8(message.mid(8));

        qDebug() << "ID:" << id << "DMA:" << dmaID << "Message:" << payload;
        emit sigNewEPNameReceived(id, dmaID, payload);
    }
}
