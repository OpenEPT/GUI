#ifndef CONSOLEWND_H
#define CONSOLEWND_H
#include <QWidget>
#include <QCompleter>
#include "Utility/log.h"

namespace Ui {
class ConsoleWnd;
}

class ConsoleWnd : public QWidget
{
    Q_OBJECT

public:
    explicit ConsoleWnd(QWidget *parent = nullptr);
    ~ConsoleWnd();
    void     printMessage(QString msg, bool exeStatus);


signals:
    void sigControlMsgSend(const QString &text);

private slots:
    void onSendControlMsgClicked();
    void onOkRecieved();
private:
    Ui::ConsoleWnd *ui;
    Log             logUtil;

    QStringList     entries;
    int             lastIndex;

protected:
    void keyPressEvent(QKeyEvent *event);

private:
    QCompleter *completer;


};

#endif // CONSOLEWND_H
