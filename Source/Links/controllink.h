#ifndef CONTROLLINK_H
#define CONTROLLINK_H

#include <QObject>
#include <QTcpSocket>
#include <QString>
#include <QTimer>

#define CONTROL_LINK_COMMAND_TIMEOUT    1000
#define CONTROL_LINK_RECONNECT_PERIOD   1000
#define CONTROL_LINK_SERIAL_BAUDRATE    115200

class SerialLink;

/* Transport used by the control link */
typedef enum{
    CONTROL_LINK_TRANSPORT_TCP,     /*!< EPP devices over TCP/IP (binary 0xA5 framing) */
    CONTROL_LINK_TRANSPORT_SERIAL   /*!< Charger device over USB CDC serial (via SerialLink) */
}control_link_transport_t;

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
    control_link_status_t   establishSerialLink(QString aPortName, qint32 aBaudRate = CONTROL_LINK_SERIAL_BAUDRATE);
    bool                    getDeviceName(QString* deviceName);

    /* Binary board-data (EEPROM) transfer over serial (0xA5 0xA5 frame). */
    bool                    transferBdWrite(const QByteArray& data, int timeout);
    bool                    transferBdRead(QByteArray* out, int timeout);
    bool                    executeCommand(QString command, QString* response, int timeout);
    bool                    executeCommand(QByteArray request, QString* response, int timeout);

    QString                 getDeviceIP_Addr();
    quint16                 getDeviceIP_Port();

signals:
    void                    sigDisconnected();
    void                    sigConnected();
    void                    sigAsyncMessage(QString message);   /*!< Unsolicited notification (serial) */

public slots:
    void                    onDisconnected();
    void                    onReconnected();
    void                    reconnect();
    void                    onSerialDisconnected();

private:

    QTcpSocket              *tcpSocket;
    SerialLink              *serialLink;        /*!< Threaded serial transport (charger) */
    control_link_transport_t transport;
    QString                 ipAddress;
    QString                 serialPortName;
    quint16                 portNumber;
    control_link_status_t   linkStatus;
    QTimer                  *reconnectTimer;

    bool                    setSocketKeepAlive();
    bool                    prvReadResponse(QString* response, int timeout);

};

#endif // CONTROLLINK_H
