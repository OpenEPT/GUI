#ifndef SERIALLINK_H
#define SERIALLINK_H

#include <QObject>
#include <QThread>
#include <QSerialPort>
#include <QByteArray>
#include <QString>
#include <QMutex>
#include <QWaitCondition>

/**
 * @brief Worker that owns the QSerialPort and lives in a dedicated thread.
 *        All reception (readyRead parsing) happens here, off the UI thread.
 */
class SerialWorker : public QObject
{
    Q_OBJECT
public:
    explicit SerialWorker(QObject* parent = nullptr);
    ~SerialWorker();

public slots:
    bool openPort(const QString& name, int baud);
    void closePort();
    void writeData(const QByteArray& data);

signals:
    void asyncMessage(QString line);            /*!< Unsolicited notification (voltage, charging mode, ...) */
    void responseText(QString payload, bool ok);/*!< STATUS/ERROR response line */
    void responseBinary(QByteArray data);        /*!< 0xA5 0xA5 'R' binary response frame payload */
    void disconnected();

private slots:
    void onReadyRead();
    void onErrorOccurred(QSerialPort::SerialPortError error);

private:
    QSerialPort* m_port;
    QByteArray   m_rxBuffer;
};

/**
 * @brief Serial transport for the Charger device. Sending is a classic
 *        (blocking) wrapper that posts to the worker thread and waits for the
 *        response; asynchronous reception runs entirely in the worker thread
 *        and reaches the UI only as queued signals, so the UI thread is never
 *        loaded by the incoming sample stream.
 */
class SerialLink : public QObject
{
    Q_OBJECT
public:
    explicit SerialLink(QObject* parent = nullptr);
    ~SerialLink();

    bool open(const QString& portName, qint32 baud = 115200);
    void close();
    bool isOpen() const { return m_open; }

    /* Text command -> text (STATUS/ERROR) response. */
    bool executeCommand(const QString& command, QString* response, int timeout);
    /* Binary board-data transfer (0xA5 framing). */
    bool transferBdWrite(const QByteArray& data, int timeout);
    bool transferBdRead(QByteArray* out, int timeout);

signals:
    void sigAsyncMessage(QString line);
    void sigDisconnected();

private slots:
    void onWorkerResponseText(QString payload, bool ok);
    void onWorkerResponseBinary(QByteArray data);
    void onWorkerDisconnected();

private:
    bool waitForResponse(int timeout);   /* called with m_mutex held */

    QThread        m_thread;
    SerialWorker*  m_worker;
    bool           m_open;

    QMutex         m_mutex;
    QWaitCondition m_cond;
    bool           m_responseReady;
    bool           m_responseOk;
    bool           m_responseIsBinary;
    QString        m_responsePayload;
    QByteArray     m_responseBinary;
};

#endif // SERIALLINK_H
