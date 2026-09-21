#ifndef LOG_H
#define LOG_H

#include <QObject>
#include <QPlainTextEdit>
#include <QVector>

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

typedef enum
{
    LOG_MESSAGE_CATEGORY_GENERAL = 0,
    LOG_MESSAGE_CATEGORY_DUT_INFO,
    LOG_MESSAGE_CATEGORY_ENERGY_POINT
}log_message_category_t;

typedef enum
{
    LOG_FILTER_ALL = 0,
    LOG_FILTER_DUT_INFO,
    LOG_FILTER_DUT_INFO_AND_ENERGY_POINTS,
    LOG_FILTER_ENERGY_POINTS
}log_filter_t;

typedef struct
{
    QString                 html;
    log_message_category_t  category;
}log_entry_t;

#define LOG_MAX_ENTRIES     5000

class Log : public QObject
{
    Q_OBJECT
public:
    explicit    Log(QObject *parent = nullptr);
    void        assignLogWidget(QPlainTextEdit* aWidget);
    void        printLogMessage(QString message, log_message_type_t type, log_message_device_type_t deviceType = LOG_MESSAGE_DEVICE_TYPE_APP, log_message_category_t category = LOG_MESSAGE_CATEGORY_GENERAL);
    static bool categoryVisible(log_message_category_t category, log_filter_t filter);

public slots:
    void        setFilter(int filter);
    void        clear();

private:
    void        appendEntry(log_entry_t entry);

    QPlainTextEdit*         plainTextWidget;
    QVector<log_entry_t>    entries;
    log_filter_t            filter;
};

#endif // LOG_H
