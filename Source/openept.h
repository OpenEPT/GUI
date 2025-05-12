#ifndef OPENEPT_H
#define OPENEPT_H

#include <QMainWindow>
#include <QList>
#include <QMessageBox>
#include "Windows/AddDevice/adddevicewnd.h"
#include "Windows/DataAnalyzer/dataanalyzer.h"
#include "devicecontainer.h"
#include "Windows/Device/energycontrolwnd.h"

QT_BEGIN_NAMESPACE
namespace Ui { class OpenEPT; }
QT_END_NAMESPACE

class OpenEPT : public QMainWindow
{
    Q_OBJECT

public:
    OpenEPT(QString aWorkspacePath, QWidget *parent = nullptr);
    ~OpenEPT();


private slots:
    void onActionAddSingleDeviceTriggered();
    void onAddDeviceWndAddDevice(QString aIpAddress, QString aPort);
    void onDeviceContainerDeviceWndClosed(Device* aDevice);
    void onDeviceContainerAllDeviceWndClosed();
    void onActionOpenAndProcessData();


private:
    Ui::OpenEPT                 *ui;
    /**/
    AddDeviceWnd                *addDeviceWnd;
    /**/
    QList<DeviceContainer*>     deviceList;
    /**/
    QMenu                       *connectedDevicesMenu;

    /**/
    DataAnalyzer                *dataAnalyzerWnd;

    /* */
    QMessageBox                 msgBox;

    /**/
    bool                        addNewDevice(QString aIpAddress, QString aPort);

    /**/
    void                        setTheme();

    /**/
    QString                     workspacePath;

    /**/
//    EnergyControlWnd            ecw;

};
#endif // OPENEPT_H
