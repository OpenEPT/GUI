#include "adddevicewnd.h"
#include "ui_adddevicewnd.h"
#include "ui_consolewnd.h"

#include <QSerialPortInfo>

/* USB identifiers advertised by the OpenEPT Charger firmware (usbd_desc.c) */
#define ADD_DEVICE_CHARGER_USB_VID  0x0483
#define ADD_DEVICE_CHARGER_USB_PID  0x5740

AddDeviceWnd::AddDeviceWnd(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AddDeviceWnd)
{
    ui->setupUi(this);
    layout()->setSizeConstraint(QLayout::SetFixedSize);

    ui->deviceTypeCombo->addItem("EPP",     (int)ADD_DEVICE_TYPE_EPP);
    ui->deviceTypeCombo->addItem("Charger", (int)ADD_DEVICE_TYPE_CHARGER);

    connect(ui->closePusb, SIGNAL(pressed()), this, SLOT(onClosePusbPressed()));
    connect(ui->addDevicePusb, SIGNAL(pressed()), this, SLOT(onAddDevicePusbPressed()));
    connect(ui->deviceTypeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onDeviceTypeChanged(int)));
    connect(ui->refreshPortsPusb, SIGNAL(pressed()), this, SLOT(onRefreshPortsPressed()));

    QFont appFont = this->font();  // Get the default font
    appFont.setPointSize(10);    // Set font size to 14 (or any desired size)
    this->setFont(appFont);

    refreshSerialPorts();
    applyDeviceTypeVisibility();
}

AddDeviceWnd::~AddDeviceWnd()
{
    delete ui;
}

void AddDeviceWnd::refreshSerialPorts()
{
    ui->serialPortCombo->clear();

    int chargerIndex = -1;
    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for(const QSerialPortInfo& info : ports)
    {
        QString label = info.systemLocation();
        if(!info.description().isEmpty())
            label += " (" + info.description() + ")";

        /* Store the port name used to open the port as the item data. */
        ui->serialPortCombo->addItem(label, info.portName());

        bool isCharger = false;
        if(info.hasVendorIdentifier() && info.hasProductIdentifier())
        {
            if(info.vendorIdentifier()  == ADD_DEVICE_CHARGER_USB_VID &&
               info.productIdentifier() == ADD_DEVICE_CHARGER_USB_PID)
                isCharger = true;
        }
        if(info.description().contains("OpenEPT", Qt::CaseInsensitive) ||
           info.manufacturer().contains("OpenEPT", Qt::CaseInsensitive))
            isCharger = true;

        if(isCharger && chargerIndex == -1)
            chargerIndex = ui->serialPortCombo->count() - 1;
    }

    /* Pre-select a detected OpenEPT Charger, if any. */
    if(chargerIndex != -1)
        ui->serialPortCombo->setCurrentIndex(chargerIndex);
}

void AddDeviceWnd::applyDeviceTypeVisibility()
{
    int type = ui->deviceTypeCombo->currentData().toInt();
    bool isCharger = (type == ADD_DEVICE_TYPE_CHARGER);

    /* EPP: IP + port.  Charger: serial port. */
    ui->ipAddressLabe->setVisible(!isCharger);
    ui->ipAddressLine->setVisible(!isCharger);
    ui->portNumberLabe->setVisible(!isCharger);
    ui->portNumberLine->setVisible(!isCharger);

    ui->serialPortLabe->setVisible(isCharger);
    ui->serialPortCombo->setVisible(isCharger);
    ui->refreshPortsPusb->setVisible(isCharger);

    if(isCharger)
        refreshSerialPorts();
}

void AddDeviceWnd::onDeviceTypeChanged(int index)
{
    Q_UNUSED(index);
    applyDeviceTypeVisibility();
}

void AddDeviceWnd::onRefreshPortsPressed()
{
    refreshSerialPorts();
}

void AddDeviceWnd::onClosePusbPressed()
{
    close();
}

void AddDeviceWnd::onAddDevicePusbPressed()
{
    int type = ui->deviceTypeCombo->currentData().toInt();

    if(type == ADD_DEVICE_TYPE_CHARGER)
    {
        /* For a charger the serial port name travels in the aIpAddress field. */
        QString portName = ui->serialPortCombo->currentData().toString();
        emit sigAddDevice(portName, "", ADD_DEVICE_TYPE_CHARGER);
    }
    else
    {
        emit sigAddDevice(ui->ipAddressLine->text(), ui->portNumberLine->text(), ADD_DEVICE_TYPE_EPP);
    }
    close();
}
