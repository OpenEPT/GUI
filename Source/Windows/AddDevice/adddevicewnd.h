#ifndef ADDDEVICEWND_H
#define ADDDEVICEWND_H

#include <QWidget>

namespace Ui {
class AddDeviceWnd;
}

/* Device type selected in the Add Device dialog. For CHARGER the aIpAddress
 * field of sigAddDevice carries the serial port name (e.g. /dev/ttyACM0). */
typedef enum{
    ADD_DEVICE_TYPE_EPP     = 0,
    ADD_DEVICE_TYPE_CHARGER = 1
}add_device_type_t;

class AddDeviceWnd : public QWidget
{
    Q_OBJECT

public:
    explicit AddDeviceWnd(QWidget *parent = nullptr);
    ~AddDeviceWnd();

signals:
    void    sigAddDevice(QString aIpAddress, QString aPort, int aDeviceType);

public slots:
    void    onClosePusbPressed();
    void    onAddDevicePusbPressed();
    void    onDeviceTypeChanged(int index);
    void    onRefreshPortsPressed();

private:
    void    refreshSerialPorts();
    void    applyDeviceTypeVisibility();
    Ui::AddDeviceWnd *ui;
};

#endif // ADDDEVICEWND_H
