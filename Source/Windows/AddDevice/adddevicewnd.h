#ifndef ADDDEVICEWND_H
#define ADDDEVICEWND_H

#include <QWidget>

namespace Ui {
class AddDeviceWnd;
}

class AddDeviceWnd : public QWidget
{
    Q_OBJECT

public:
    explicit AddDeviceWnd(QWidget *parent = nullptr);
    ~AddDeviceWnd();

signals:
    void    sigAddDevice(QString aIpAddress, QString aPort);

public slots:
    void    onClosePusbPressed();
    void    onAddDevicePusbPressed();

private:
    Ui::AddDeviceWnd *ui;
};

#endif // ADDDEVICEWND_H
