#ifndef CONTROLLINK_H
#define CONTROLLINK_H

#include <QObject>
#include <QTcpSocket>
#include <QString>
#include <QTimer>

#define CONTROL_LINK_COMMAND_TIMEOUT    1000
#define CONTROL_LINK_RECONNECT_PERIOD   1000

typedef enum{
    CONTROL_LINK_STATUS_ESTABLISHED,
    CONTROL_LINK_STATUS_DISABLED,
    CONTROL_LINK_STATUS_RECONNECTING
}control_link_status_t;

class ControlLink : public QObject
{
    Q_OBJECT
public:
    explicit ControlLink(QObject *parent = nullptr);
    ~ControlLink();
    control_link_status_t   establishLink(QString aIpAddress, QString aPortNumber);
    bool                    getDeviceName(QString* deviceName);
    bool                    executeCommand(QString command, QString* response, int timeout);

signals:
    void                    sigDisconnected();
    void                    sigConnected();

public slots:
    void                    onDisconnected();
    void                    onReconnected();
    void                    reconnect();

private:

    QTcpSocket              *tcpSocket;
    QString                 ipAddress;
    quint16                 portNumber;
    control_link_status_t   linkStatus;
    QTimer                  *reconnectTimer;

    bool                    setSocketKeepAlive();

};

#endif // CONTROLLINK_H
