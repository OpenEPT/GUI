#ifndef LOG_H
#define LOG_H

#include <QObject>
#include <QPlainTextEdit>

typedef enum
{
    LOG_MESSAGE_TYPE_INFO,
    LOG_MESSAGE_TYPE_WARNING,
    LOG_MESSAGE_TYPE_ERROR
}log_message_type_t;

typedef enum
{
    LOG_MESSAGE_DEVICE_TYPE_APP,
    LOG_MESSAGE_DEVICE_TYPE_CONSOLE,
    LOG_MESSAGE_DEVICE_TYPE_DEVICE
}log_message_device_type_t;

class Log : public QObject
{
    Q_OBJECT
public:
    explicit    Log(QObject *parent = nullptr);
    void        assignLogWidget(QPlainTextEdit* aWidget);
    void        printLogMessage(QString message, log_message_type_t type, log_message_device_type_t deviceType = LOG_MESSAGE_DEVICE_TYPE_APP);

signals:

private:
    QPlainTextEdit* plainTextWidget;

};

#endif // LOG_H
