#ifndef EDLINK_H
#define EDLINK_H

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include <QThread>

class EDLink : public QObject
{
    Q_OBJECT
public:
    explicit EDLink(QObject *parent = nullptr);

    void                    startServer();
    quint16                 getPort();

signals:
    void                    sigNewClientConnected(QString ip);
    void                    sigNewEPNameReceived(unsigned int PacketID, unsigned int SampleID, QString name);

public slots:
    void                    onServerStarted();
    void                    onNewConnectionAdded();
    void                    onReadPendingData();


private:
    QTcpServer              *tcpServer;
    QList<QTcpSocket*>      clientList;
    QThread                 *tcpServerThread;
    quint16                 port;

};

#endif // EDLINK_H
