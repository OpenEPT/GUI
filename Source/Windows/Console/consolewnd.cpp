#include <QFileDialog>
#include <QTextStream>
#include <QRegularExpression>
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
    connect(ui->loadScriptPusb,  SIGNAL(clicked(bool)), this, SLOT(onProcessScriptPusb()));
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
        "device adc samplesno set",

        "device dac enable set",
        "device dac enable get",
        "device dac value set",
        "device dac value get",

        "device load enable",
        "device load disable",
        "device load get",

        "device bat enable",
        "device bat disable",
        "device bat get",

        "device ppath enable",
        "device ppath disable",
        "device ppath get",

        "device uvoltage state get",
        "device ovoltage state get",
        "device ocurrent state get",

        "device uvoltage value get",
        "device uvoltage value set",
        "device ovoltage value get",
        "device ovoltage value set",
        "device ocurrent value get",
        "device ocurrent value set",

        "device latch trigger",

        "device rgb setcolor",

        "device wave chunk add",
        "device wave counter set",
        "device wave state set",
        "device wave clear",

        "charger charging enable",
        "charger charging disable",
        "charger charging get",

        "charger charging current set",
        "charger charging current get",

        "charger charging termcurrent set",
        "charger charging termcurrent get",

        "charger charging termvoltage set",
        "charger charging termvoltage get",

        "charger reg read"
    };

    completer = new QCompleter(commandList);
    ui->controlSendLine->setPlaceholderText("Ented command");

    ui->controlSendLine->setCompleter(completer);

    isBatchSending = false;

}
ConsoleWnd::~ConsoleWnd()
{
    delete ui;
}

void ConsoleWnd::sendCommandBatch(const QString &batch)
{
    if (isBatchSending) return;  // prevent overlapping calls

    pendingCommands.clear();

    // Split by newline, preserve semicolons, and trim whitespace
    QStringList lines = batch.split(QRegExp("[\r\n]+"), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        QString cmd = line.trimmed();
        if (!cmd.isEmpty()) {
            pendingCommands.append(cmd);
        }
    }

    if (pendingCommands.isEmpty())
        return;

    isBatchSending = true;
    sendNextBatchCommand();
}

void ConsoleWnd::sendNextBatchCommand()
{
    if (pendingCommands.isEmpty()) {
        isBatchSending = false;
        return;
    }

    QString nextCommand = pendingCommands.takeFirst();
    if (!nextCommand.isEmpty()) {
        logUtil.printLogMessage(" Command: " + nextCommand, LOG_MESSAGE_TYPE_INFO, LOG_MESSAGE_DEVICE_TYPE_CONSOLE);
        emit sigControlMsgSend(nextCommand);
    } else {
        sendNextBatchCommand();  // Skip empty
    }
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
    // Process next batch command if current succeeded
    if (isBatchSending)
    {
        if (exeStatus)
             sendNextBatchCommand();  // Skip empty
        else
            isBatchSending = false; // stop on error
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

void ConsoleWnd::onProcessScriptPusb()
{
    // Open file dialog to select a text file
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Open Script File"),
        "",
        tr("Text Files (*.txt);;All Files (*)"));

    if (fileName.isEmpty())
        return;  // User canceled

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        logUtil.printLogMessage(" Unable to open file", LOG_MESSAGE_TYPE_ERROR, LOG_MESSAGE_DEVICE_TYPE_CONSOLE);
        return;
    }

    QTextStream in(&file);
    QString scriptContent = in.readAll();
    file.close();
    // Send commands batch
    sendCommandBatch(scriptContent);
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
