#ifndef EPLINK_H
#define EPLINK_H

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include <QThread>

class EPLink : public QObject
{
    Q_OBJECT
public:
    explicit EPLink(QObject *parent = nullptr);

    void                    startServer();
    quint16                 getPort();

signals:
    void                    sigNewClientConnected(QString ip);
    void                    sigNewEPNameReceived(unsigned int ebpid, QString name);

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

#endif // EPLINK_H
