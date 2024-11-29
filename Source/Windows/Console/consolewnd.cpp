
#include "consolewnd.h"
#include "ui_consolewnd.h"

ConsoleWnd::ConsoleWnd(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ConsoleWnd)
{
    ui->setupUi(this);

    QFont appFont = this->font();  // Get the default font
    appFont.setPointSize(10);    // Set font size to 14 (or any desired size)
    this->setFont(appFont);

    connect(ui->controlSendPusb, SIGNAL(clicked(bool)), this, SLOT(onSendControlMsgClicked()));
    connect(ui->controlSendLine, SIGNAL(returnPressed()), this, SLOT(onSendControlMsgClicked()));
    logUtil.assignLogWidget(ui->controlSendRecievePlte);
    lastIndex = 0;
    QStringList commandList = {
        "",
        "device hello",
        "device setname",
        "device slink create",
        "device slink send",
        "device eplink create",
        "device stream create",
        "device stream start",
        "device stream stop",
        "device adc chresolution set",
        "device adc chresolution get",
        "device adc chclkdiv set",
        "device adc chclkdiv get",
        "device adc chstime set",
        "device adc chstime get",
        "device adc chavrratio set",
        "device adc chavrratio get",
        "device adc speriod set",
        "device adc speriod get",
        "device adc voffset set",
        "device adc voffset get",
        "device adc coffset set",
        "device adc coffset get",
        "device adc clk get",
        "device adc value get",
        "device dac enable set",
        "device dac value set",
        "device load enable",
        "device load disable",
        "device bat enable",
        "device bat disable",
        "device rgb setcolor"
    };

    completer = new QCompleter(commandList);
    ui->controlSendLine->setPlaceholderText("Ented command");

    ui->controlSendLine->setCompleter(completer);


}

ConsoleWnd::~ConsoleWnd()
{
    delete ui;
}

void ConsoleWnd::printMessage(QString msg, bool exeStatus)
{
    if(exeStatus)
    {
        logUtil.printLogMessage(" Response: " + msg, LOG_MESSAGE_TYPE_INFO, LOG_MESSAGE_DEVICE_TYPE_DEVICE);
    }
    else
    {
        logUtil.printLogMessage(" Response: " + msg, LOG_MESSAGE_TYPE_ERROR, LOG_MESSAGE_DEVICE_TYPE_DEVICE);
    }
}

void ConsoleWnd::onSendControlMsgClicked() {
    QString textToSend = ui->controlSendLine->text();
    logUtil.printLogMessage(" Command: " + textToSend, LOG_MESSAGE_TYPE_INFO, LOG_MESSAGE_DEVICE_TYPE_CONSOLE);
    entries.append(textToSend);
    if(entries.length() != 0)
    {
        lastIndex = entries.length();
    }
    ui->controlSendLine->clear();
    /* emit Signal to deviceWnd -> */
    emit sigControlMsgSend(textToSend);
}

void ConsoleWnd::onOkRecieved() {
    ui->controlSendRecievePlte->appendPlainText("OK");
}
void ConsoleWnd::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Up)
    {
        if(entries.length() != 0){
            lastIndex = lastIndex - 1;
            lastIndex = lastIndex < 0 ?  0: lastIndex;
            ui->controlSendLine->setText(entries.at(lastIndex));
        }

    }
    if(event->key() == Qt::Key_Down)
    {
        if(entries.length() != 0){
            lastIndex = lastIndex + 1;
            lastIndex = lastIndex >= entries.length() ?  entries.length() - 1 : lastIndex;
            ui->controlSendLine->setText(entries.at(lastIndex));
        }
    }
}
