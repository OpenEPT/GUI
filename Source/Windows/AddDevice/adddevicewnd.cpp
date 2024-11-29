#include "adddevicewnd.h"
#include "ui_adddevicewnd.h"
#include "ui_consolewnd.h"

AddDeviceWnd::AddDeviceWnd(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AddDeviceWnd)
{
    ui->setupUi(this);
    layout()->setSizeConstraint(QLayout::SetFixedSize);

    connect(ui->closePusb, SIGNAL(pressed()), this, SLOT(onClosePusbPressed()));
    connect(ui->addDevicePusb, SIGNAL(pressed()), this, SLOT(onAddDevicePusbPressed()));

    QFont appFont = this->font();  // Get the default font
    appFont.setPointSize(10);    // Set font size to 14 (or any desired size)
    this->setFont(appFont);
}

AddDeviceWnd::~AddDeviceWnd()
{
    delete ui;
}

void AddDeviceWnd::onClosePusbPressed()
{
    close();
}

void AddDeviceWnd::onAddDevicePusbPressed()
{
    emit sigAddDevice(ui->ipAddressLine->text(), ui->portNumberLine->text());
    close();
}
