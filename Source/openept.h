#ifndef OPENEPT_H
#define OPENEPT_H

#include <QMainWindow>
#include <QList>
#include <QMessageBox>
#include "Windows/AddDevice/adddevicewnd.h"
#include "Windows/DataAnalyzer/dataanalyzer.h"
#include "devicecontainer.h"
#include "Windows/Device/energycontrolwnd.h"
#include "Windows/ApplicationConf/applicationconfwnd.h"
#include "Processing/Parameters/applicationparameters.h"

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
    void onDeviceContainerDeviceWndClosed(DeviceContainer* container);
    void onDeviceContainerAllDeviceWndClosed();
    void onActionOpenAndProcessData();
    void onActionAppSettings();

    void onAppConfigUpdated(QMap<QString, QString> changedFields);


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

    /**/
    ApplicationParameters*     m_AppParam;

    ApplicationConfWnd*        appConfWnd;

    /**/
    QString applicationConfigDirPath;
    QString applicationConfigFilePath;

    QString                     createApplicationConfigDir();
    bool                        loadApplicationConfigJson(QJsonObject *jsonObject);
    bool                        saveApplicationConfigJson(const QJsonObject &jsonObject);
    void                        initializeApplicationParameters();

    unsigned int                connectedDeviceNumber;

};
#endif // OPENEPT_H
