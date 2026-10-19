#include "seriallink.h"
#include <QDeadlineTimer>

/* ======================= SerialWorker (worker thread) ======================= */

SerialWorker::SerialWorker(QObject* parent) :
    QObject(parent),
    m_port(nullptr)
{
}

SerialWorker::~SerialWorker()
{
    if(m_port != nullptr && m_port->isOpen())
        m_port->close();
}

bool SerialWorker::openPort(const QString& name, int baud)
{
    /* Created here so the QSerialPort has affinity to the worker thread. */
    if(m_port == nullptr)
    {
        m_port = new QSerialPort(this);
        connect(m_port, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
        connect(m_port, SIGNAL(errorOccurred(QSerialPort::SerialPortError)),
                this, SLOT(onErrorOccurred(QSerialPort::SerialPortError)));
    }

    if(m_port->isOpen()) m_port->close();
    m_rxBuffer.clear();

    m_port->setPortName(name);
    m_port->setBaudRate(baud);
    m_port->setDataBits(QSerialPort::Data8);
    m_port->setParity(QSerialPort::NoParity);
    m_port->setStopBits(QSerialPort::OneStop);
    m_port->setFlowControl(QSerialPort::NoFlowControl);

    if(!m_port->open(QIODevice::ReadWrite))
        return false;

    m_port->setDataTerminalReady(true);   /* enable firmware TX path */
    m_port->clear();
    return true;
}

void SerialWorker::closePort()
{
    if(m_port != nullptr && m_port->isOpen())
        m_port->close();
}

void SerialWorker::writeData(const QByteArray& data)
{
    if(m_port != nullptr && m_port->isOpen())
    {
        m_port->write(data);
        m_port->flush();
    }
}

void SerialWorker::onReadyRead()
{
    if(m_port == nullptr) return;
    m_rxBuffer += m_port->readAll();

    for(;;)
    {
        /* Binary board-data response frame: 0xA5 0xA5 'R' <sizeLo> <sizeHi> <data> */
        if(m_rxBuffer.size() >= 3 &&
           (quint8)m_rxBuffer.at(0) == 0xA5 &&
           (quint8)m_rxBuffer.at(1) == 0xA5 &&
           m_rxBuffer.at(2) == 'R')
        {
            if(m_rxBuffer.size() < 5) return;
            quint16 size = (quint8)m_rxBuffer.at(3) | ((quint16)(quint8)m_rxBuffer.at(4) << 8);
            if(m_rxBuffer.size() < 5 + (int)size) return;

            QByteArray data = m_rxBuffer.mid(5, size);
            m_rxBuffer.remove(0, 5 + size);
            emit responseBinary(data);
            continue;
        }

        int nlIndex = m_rxBuffer.indexOf('\n');
        if(nlIndex < 0) return;

        QByteArray lineBytes = m_rxBuffer.left(nlIndex);
        m_rxBuffer.remove(0, nlIndex + 1);

        QString line = QString::fromUtf8(lineBytes).trimmed();
        if(line.isEmpty()) continue;

        if(line.startsWith("STATUS"))      emit responseText(line.mid(6).trimmed(), true);
        else if(line.startsWith("ERROR"))  emit responseText("ERROR", false);
        else                               emit asyncMessage(line);
    }
}

void SerialWorker::onErrorOccurred(QSerialPort::SerialPortError error)
{
    if(error == QSerialPort::ResourceError ||
       error == QSerialPort::PermissionError ||
       error == QSerialPort::DeviceNotFoundError)
    {
        if(m_port != nullptr && m_port->isOpen()) m_port->close();
        emit disconnected();
    }
}

/* ============================ SerialLink (UI thread) ======================== */

SerialLink::SerialLink(QObject* parent) :
    QObject(parent),
    m_worker(new SerialWorker),
    m_open(false),
    m_responseReady(false),
    m_responseOk(false),
    m_responseIsBinary(false)
{
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);

    /* Responses are delivered via a DirectConnection so they run in the worker
     * thread and can wake a caller that is blocked in executeCommand(). */
    connect(m_worker, &SerialWorker::responseText,   this, &SerialLink::onWorkerResponseText,   Qt::DirectConnection);
    connect(m_worker, &SerialWorker::responseBinary, this, &SerialLink::onWorkerResponseBinary, Qt::DirectConnection);

    /* Async notifications are relayed to the UI as queued signals. */
    connect(m_worker, &SerialWorker::asyncMessage, this, &SerialLink::sigAsyncMessage);
    connect(m_worker, &SerialWorker::disconnected, this, &SerialLink::onWorkerDisconnected);

    m_thread.start();
}

SerialLink::~SerialLink()
{
    close();
    m_thread.quit();
    m_thread.wait(1000);
}

bool SerialLink::open(const QString& portName, qint32 baud)
{
    bool ok = false;
    QMetaObject::invokeMethod(m_worker, "openPort", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, ok),
                              Q_ARG(QString, portName), Q_ARG(int, (int)baud));
    m_open = ok;
    return ok;
}

void SerialLink::close()
{
    if(m_worker != nullptr)
        QMetaObject::invokeMethod(m_worker, "closePort", Qt::BlockingQueuedConnection);
    m_open = false;
}

void SerialLink::onWorkerResponseText(QString payload, bool ok)
{
    QMutexLocker locker(&m_mutex);
    m_responsePayload  = payload;
    m_responseOk       = ok;
    m_responseIsBinary = false;
    m_responseReady    = true;
    m_cond.wakeAll();
}

void SerialLink::onWorkerResponseBinary(QByteArray data)
{
    QMutexLocker locker(&m_mutex);
    m_responseBinary   = data;
    m_responseOk       = true;
    m_responseIsBinary = true;
    m_responseReady    = true;
    m_cond.wakeAll();
}

void SerialLink::onWorkerDisconnected()
{
    m_open = false;
    emit sigDisconnected();
}

bool SerialLink::waitForResponse(int timeout)
{
    QDeadlineTimer deadline(timeout);
    while(!m_responseReady)
    {
        qint64 remaining = deadline.remainingTime();
        if(remaining <= 0) return false;
        m_cond.wait(&m_mutex, (unsigned long)remaining);
    }
    return true;
}

bool SerialLink::executeCommand(const QString& command, QString* response, int timeout)
{
    if(!m_open) { if(response) *response = "Serial link not open"; return false; }

    QByteArray data = command.toUtf8();
    data.append('\r');

    QMutexLocker locker(&m_mutex);
    m_responseReady = false;
    QMetaObject::invokeMethod(m_worker, "writeData", Qt::QueuedConnection, Q_ARG(QByteArray, data));

    if(!waitForResponse(timeout)) { if(response) *response = "Unable to read data"; return false; }
    if(m_responseIsBinary)        { if(response) *response = "unexpected binary";   return false; }

    if(response) *response = m_responsePayload;
    return m_responseOk;
}

bool SerialLink::transferBdWrite(const QByteArray& data, int timeout)
{
    if(!m_open) return false;

    quint16 size = (quint16)data.size();
    QByteArray frame;
    frame.append((char)0xA5);
    frame.append((char)0xA5);
    frame.append('W');
    frame.append((char)(size & 0xFF));
    frame.append((char)((size >> 8) & 0xFF));
    frame.append(data);

    QMutexLocker locker(&m_mutex);
    m_responseReady = false;
    QMetaObject::invokeMethod(m_worker, "writeData", Qt::QueuedConnection, Q_ARG(QByteArray, frame));

    if(!waitForResponse(timeout)) return false;
    if(m_responseIsBinary)        return false;
    return (m_responsePayload == "OK");
}

bool SerialLink::transferBdRead(QByteArray* out, int timeout)
{
    if(out == nullptr || !m_open) return false;

    QByteArray cmd("charger bd read");
    cmd.append('\r');

    QMutexLocker locker(&m_mutex);
    m_responseReady = false;
    QMetaObject::invokeMethod(m_worker, "writeData", Qt::QueuedConnection, Q_ARG(QByteArray, cmd));

    if(!waitForResponse(timeout)) return false;
    if(!m_responseIsBinary)       return false;

    *out = m_responseBinary;
    return true;
}
