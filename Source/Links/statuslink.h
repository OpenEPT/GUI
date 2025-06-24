#ifndef STATUSLINK_H
#define STATUSLINK_H

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include <QThread>

#define STATUS_LINK_SERVER_PORT         7000
#define STATUS_LINK_BUFFER_SIZE         1500

class StatusLink : public QObject
{
    Q_OBJECT
public:
    explicit StatusLink(QObject *parent = nullptr);

    void                    startServer();
    void                    setPort(quint16 aPortNo);
    quint16                 getPort();

signals:
    void                    sigNewClientConnected(QString ip);
    void                    sigNewStatusMessageReceived(QString clientIp, QString message);

public slots:
    void                    onServerStarted();
    void                    onNewConnectionAdded();
    void                    onReadPendingData();


private:
    QTcpServer              *tcpServer;
    QList<QTcpSocket*>      clientList;
    QThread                 *tcpServerThread;
    quint16                 portNo;
};

#endif // STATUSLINK_H
