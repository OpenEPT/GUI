#include "log.h"
#include <QTime>

Log::Log(QObject *parent)
    : QObject{parent}
{

}

void Log::assignLogWidget(QPlainTextEdit *aWidget)
{
    plainTextWidget = aWidget;
    plainTextWidget->setReadOnly(true);
}

void Log::printLogMessage(QString message, log_message_type_t type, log_message_device_type_t deviceType)
{
    QString tmpMessage;
    QString tmpMessageDeviceType;
    switch(deviceType)
    {
    case LOG_MESSAGE_DEVICE_TYPE_APP:
        tmpMessageDeviceType += "[Application] ";
        break;
    case LOG_MESSAGE_DEVICE_TYPE_DEVICE:
        tmpMessageDeviceType += "[ACDevice]    ";
        break;
    case LOG_MESSAGE_DEVICE_TYPE_CONSOLE:
        tmpMessageDeviceType += "[Console]     ";
        break;
    }
    plainTextWidget->moveCursor(QTextCursor::End);
    switch(type)
    {
    case LOG_MESSAGE_TYPE_INFO:
        tmpMessage = "<p style=\"color:black;\">";
        tmpMessage += tmpMessageDeviceType;
        tmpMessage += "[" + QTime::currentTime().toString() + "]:";
        tmpMessage += message;
        tmpMessage += "</p>";
        plainTextWidget->appendHtml(tmpMessage);
        break;
    case LOG_MESSAGE_TYPE_WARNING:
        tmpMessage = "<p style=\"color:blue;\">";
        tmpMessage += tmpMessageDeviceType;
        tmpMessage += "[" + QTime::currentTime().toString() + "]:";
        tmpMessage += "[Warning]";
        tmpMessage += message;
        tmpMessage += "</p>";
        plainTextWidget->appendHtml(tmpMessage);
        break;
    case LOG_MESSAGE_TYPE_ERROR:
        tmpMessage = "<p style=\"color:red;\">";
        tmpMessage += tmpMessageDeviceType;
        tmpMessage += "[" + QTime::currentTime().toString() + "]:";
        tmpMessage += "[Error]";
        tmpMessage += message;
        tmpMessage += "</p>";
        plainTextWidget->appendHtml(tmpMessage);
        break;
    }
    plainTextWidget->moveCursor(QTextCursor::End);
}
