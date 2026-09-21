#include "log.h"
#include <QTime>

Log::Log(QObject *parent)
    : QObject{parent}
{
    plainTextWidget = NULL;
    filter = LOG_FILTER_ALL;
}

void Log::assignLogWidget(QPlainTextEdit *aWidget)
{
    plainTextWidget = aWidget;
    plainTextWidget->setReadOnly(true);
}

bool Log::categoryVisible(log_message_category_t category, log_filter_t filter)
{
    switch(filter)
    {
    case LOG_FILTER_DUT_INFO:
        return category == LOG_MESSAGE_CATEGORY_DUT_INFO;
    case LOG_FILTER_DUT_INFO_AND_ENERGY_POINTS:
        return category == LOG_MESSAGE_CATEGORY_DUT_INFO || category == LOG_MESSAGE_CATEGORY_ENERGY_POINT;
    case LOG_FILTER_ENERGY_POINTS:
        return category == LOG_MESSAGE_CATEGORY_ENERGY_POINT;
    default:
        return true;
    }
}

void Log::appendEntry(log_entry_t entry)
{
    if(plainTextWidget == NULL) return;
    if(!categoryVisible(entry.category, filter)) return;
    plainTextWidget->moveCursor(QTextCursor::End);
    plainTextWidget->appendHtml(entry.html);
    plainTextWidget->moveCursor(QTextCursor::End);
}

void Log::setFilter(int aFilter)
{
    filter = (log_filter_t)aFilter;
    if(plainTextWidget == NULL) return;
    plainTextWidget->clear();
    for(int i = 0; i < entries.size(); i++)
    {
        appendEntry(entries[i]);
    }
}

void Log::clear()
{
    entries.clear();
    if(plainTextWidget != NULL) plainTextWidget->clear();
}

void Log::printLogMessage(QString message, log_message_type_t type, log_message_device_type_t deviceType, log_message_category_t category)
{
    QString tmpMessage;
    QString tmpMessageDeviceType;
    log_entry_t entry;

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
    switch(category)
    {
    case LOG_MESSAGE_CATEGORY_DUT_INFO:
        tmpMessageDeviceType += "[DUT info]    ";
        break;
    case LOG_MESSAGE_CATEGORY_ENERGY_POINT:
        tmpMessageDeviceType += "[EP]          ";
        break;
    default:
        break;
    }
    switch(type)
    {
    case LOG_MESSAGE_TYPE_INFO:
        tmpMessage = "<p style=\"color:" + QString(category == LOG_MESSAGE_CATEGORY_DUT_INFO ? "#006400" : (category == LOG_MESSAGE_CATEGORY_ENERGY_POINT ? "#8B4513" : "black")) + ";\">";
        tmpMessage += tmpMessageDeviceType;
        tmpMessage += "[" + QTime::currentTime().toString() + "]:";
        tmpMessage += message.toHtmlEscaped();
        tmpMessage += "</p>";
        break;
    case LOG_MESSAGE_TYPE_WARNING:
        tmpMessage = "<p style=\"color:blue;\">";
        tmpMessage += tmpMessageDeviceType;
        tmpMessage += "[" + QTime::currentTime().toString() + "]:";
        tmpMessage += "[Warning]";
        tmpMessage += message.toHtmlEscaped();
        tmpMessage += "</p>";
        break;
    case LOG_MESSAGE_TYPE_ERROR:
        tmpMessage = "<p style=\"color:red;\">";
        tmpMessage += tmpMessageDeviceType;
        tmpMessage += "[" + QTime::currentTime().toString() + "]:";
        tmpMessage += "[Error]";
        tmpMessage += message.toHtmlEscaped();
        tmpMessage += "</p>";
        break;
    }

    entry.html = tmpMessage;
    entry.category = category;
    if(entries.size() >= LOG_MAX_ENTRIES) entries.removeFirst();
    entries.append(entry);
    appendEntry(entry);
}
