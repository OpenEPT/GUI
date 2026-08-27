#include <QPushButton>
#include <QAction>
#include <QMenu>
#include "openept.h"
#include "Windows/Device/devicewnd.h"
#include "ui_openept.h"
#include "Links/controllink.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>
#include <QDockWidget>


#define BUTTON_WIDTH (25)


OpenEPT::OpenEPT(QString aWorkspacePath, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::OpenEPT)
{
    ui->setupUi(this);

    QWidget *oldCentralWidget = takeCentralWidget();

    if(oldCentralWidget != nullptr)
    {
        oldCentralWidget->deleteLater();
    }

    setDockOptions(QMainWindow::AllowTabbedDocks |
                   QMainWindow::AllowNestedDocks |
                   QMainWindow::AnimatedDocks);

    dataAnalyzerWnd = new DataAnalyzer(nullptr,aWorkspacePath);

    addDeviceWnd = new AddDeviceWnd(this);
    addDeviceWnd->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    addDeviceWnd->setWindowModality(Qt::WindowModal);
    connect(addDeviceWnd, SIGNAL(sigAddDevice(QString,QString)), this, SLOT(onAddDeviceWndAddDevice(QString,QString)), Qt::QueuedConnection);


    connectedDevicesMenu = new QMenu("Connected devices");
    connectedDevicesMenu->setStyleSheet("background-color: rgb(186, 59, 10);");
    ui->menuDevices->addMenu(connectedDevicesMenu);


    connect(ui->actionAddSingleDevice, &QAction::triggered, this,  &OpenEPT::onActionAddSingleDeviceTriggered);
    connect(ui->actionDataAnalyzer, &QAction::triggered, this,  &OpenEPT::onActionOpenAndProcessData);
    connect(ui->actionApplicationSettings, &QAction::triggered, this,  &OpenEPT::onActionAppSettings);

    workspacePath = aWorkspacePath;

    applicationConfigDirPath = createApplicationConfigDir();
    applicationConfigFilePath =
        applicationConfigDirPath + QDir::separator() + "application_config.json";

    m_AppParam = new ApplicationParameters();

    initializeApplicationParameters();

    appConfWnd = new ApplicationConfWnd(0, m_AppParam);

    connect(appConfWnd,
            &ApplicationConfWnd::sigApplicationConfigSet,
            this,
            &OpenEPT::onAppConfigUpdated);

    connectedDeviceNumber = 0;

 }

OpenEPT::~OpenEPT()
{
    onDeviceContainerAllDeviceWndClosed();
    delete ui;
}

void OpenEPT::onActionAddSingleDeviceTriggered()
{
    addDeviceWnd->show();
}

void OpenEPT::onAddDeviceWndAddDevice(QString aIpAddress, QString aPort)
{
    if(addNewDevice(aIpAddress, aPort))
    {
        msgBox.setText("Device sucessfully added");
        msgBox.exec();
    }
    else
    {
        msgBox.setText("Unable to add device");
        msgBox.exec();
    }
}


bool OpenEPT::addNewDevice(QString aIpAddress, QString aPort)
{
    QString deviceName;

    /* Create control link and try to access device*/
    ControlLink* tmpControlLink = new ControlLink();

    /* Try to establish connection with device*/
    if(tmpControlLink->establishLink(aIpAddress, aPort) != CONTROL_LINK_STATUS_ESTABLISHED)
    {
        delete tmpControlLink;
        return false;
    }

    if(!tmpControlLink->getDeviceName(&deviceName))
    {
        delete tmpControlLink;
        return false;
    }


    /* Create device */
    Device  *tmpDevice = new Device(0, m_AppParam, connectedDeviceNumber++);
    tmpDevice->setName(deviceName);
    tmpDevice->controlLinkAssign(tmpControlLink);

    /* Create corresponding device window*/
    DeviceWnd *tmpdeviceWnd = new DeviceWnd(0);
    tmpdeviceWnd->setWindowTitle(deviceName);
    tmpdeviceWnd->setDeviceNetworkState(DEVICE_STATE_CONNECTED);
    tmpdeviceWnd->setDeviceInterfaceSelectionState(DEVICE_INTERFACE_SELECTION_STATE_UNDEFINED);
    tmpdeviceWnd->setParameters(tmpDevice->parameters());
    //tmpdeviceWnd->setWorkingSpaceDir(workspacePath);


    // Add the child window to the MDI area
    QDockWidget *dock = new QDockWidget(deviceName, this);

    /* Create device container */
    DeviceContainer *tmpDeviceContainer = new DeviceContainer(NULL,tmpdeviceWnd,tmpDevice, m_AppParam, dock);

    connect(tmpDeviceContainer, SIGNAL(sigDeviceClosed(DeviceContainer*)), this, SLOT(onDeviceContainerDeviceWndClosed(DeviceContainer*)));

    /* Add device to menu bar */
    QAction* tmpDeviceAction = new QAction(deviceName);
    connectedDevicesMenu->addAction(tmpDeviceAction);

    dock->setObjectName("DeviceDock_" + QString::number(connectedDeviceNumber));

    dock->setWidget(tmpdeviceWnd);

    dock->setAllowedAreas(Qt::AllDockWidgetAreas);
    dock->setFeatures(QDockWidget::DockWidgetMovable |
                      QDockWidget::DockWidgetFloatable |
                      QDockWidget::DockWidgetClosable);

    addDockWidget(Qt::TopDockWidgetArea, dock);

    QList<QDockWidget*> docks = findChildren<QDockWidget*>();

    for(QDockWidget *existingDock : docks)
    {
        if(existingDock != dock &&
           existingDock->objectName().startsWith("DeviceDock_"))
        {
            tabifyDockWidget(existingDock, dock);
            break;
        }
    }

    dock->show();
    dock->raise();

    deviceList.append(tmpDeviceContainer);

    setTheme();

    return true;
}

void OpenEPT::setTheme()
{
    //setStyleSheet("color:white;background-color:#404241;border-color:#404241");
}

void OpenEPT::onDeviceContainerDeviceWndClosed(DeviceContainer *container)
{
    Device* device = container->getDevice();

    QString name;
    device->getName(&name);

    QList<QAction*> actionList = connectedDevicesMenu->actions();

    for(int i = 0; i < actionList.size(); i++)
    {
        if(actionList[i]->text() == name)
        {
            connectedDevicesMenu->removeAction(actionList[i]);
            deviceList.removeAt(i);
            break;
        }
    }


    /* ===== CLOSE UI ===== */
//    if(container->getDeviceWnd())
//    {
//        container->getDeviceWnd()->close();
//    }

    /* ===== DELETE CONTAINER (cascade) ===== */
    delete container;
}

void OpenEPT::onDeviceContainerAllDeviceWndClosed()
{
    DeviceContainer* tmpDeviceContainer;
    QList<QAction*> actionList = connectedDevicesMenu->actions();
    for(int i = 0; i < actionList.size(); i++)
    {
        connectedDevicesMenu->removeAction(actionList[i]);
        tmpDeviceContainer = deviceList.at(i);
        deviceList.removeAt(i);
        delete tmpDeviceContainer;
    }
}

void OpenEPT::onActionOpenAndProcessData()
{
    dataAnalyzerWnd->show();
}

void OpenEPT::onActionAppSettings()
{
    appConfWnd->show();
    appConfWnd->raise();
    appConfWnd->activateWindow();
}

void OpenEPT::onAppConfigUpdated(QMap<QString, QString> changedFields)
{
    bool updateOk = true;

    for(auto it = changedFields.constBegin(); it != changedFields.constEnd(); ++it)
    {
        if(m_AppParam->setParamValue(it.key(), it.value()) == false)
        {
            updateOk = false;
        }
    }

    const bool saved = updateOk && saveApplicationConfigJson(m_AppParam->toJson());

    if(appConfWnd != nullptr)
    {
        appConfWnd->setConfigurationAppliedStatus(saved);
    }
}
QString OpenEPT::createApplicationConfigDir()
{
    QString configDirPath =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    if(configDirPath.isEmpty())
    {
        configDirPath =
            QDir::homePath() + QDir::separator() + ".config" + QDir::separator() + "OpenEPT";
    }

    QDir configDir(configDirPath);

    if(configDir.exists() == false)
    {
        configDir.mkpath(".");
    }

    return configDirPath;
}

bool OpenEPT::loadApplicationConfigJson(QJsonObject *jsonObject)
{
    if(jsonObject == nullptr)
    {
        return false;
    }

    QFile file(applicationConfigFilePath);

    if(file.exists() == false)
    {
        return false;
    }

    if(file.open(QIODevice::ReadOnly) == false)
    {
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData, &parseError);

    if(parseError.error != QJsonParseError::NoError)
    {
        return false;
    }

    if(jsonDocument.isObject() == false)
    {
        return false;
    }

    *jsonObject = jsonDocument.object();

    return true;
}

bool OpenEPT::saveApplicationConfigJson(const QJsonObject &jsonObject)
{
    QDir configDir(applicationConfigDirPath);

    if(configDir.exists() == false)
    {
        if(configDir.mkpath(".") == false)
        {
            return false;
        }
    }

    QFile file(applicationConfigFilePath);

    if(file.open(QIODevice::WriteOnly | QIODevice::Truncate) == false)
    {
        return false;
    }

    QJsonDocument jsonDocument(jsonObject);

    file.write(jsonDocument.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

void OpenEPT::initializeApplicationParameters()
{
    applicationConfigDirPath = createApplicationConfigDir();

    applicationConfigFilePath =
        applicationConfigDirPath +
        QDir::separator() +
        "application_config.json";

    QJsonObject configJson;
    bool configValid = loadApplicationConfigJson(&configJson);

    if(configValid == true)
    {
        if(m_AppParam->fromJson(configJson) == false)
        {
            configValid = false;
        }
    }

    /*
     * workspacePath is selected before the main application window is created.
     * Therefore, it always overrides the value stored in application_config.json.
     */
    m_AppParam->setParamValue("workspacePath", workspacePath);

    if(configValid == false)
    {
        saveApplicationConfigJson(m_AppParam->toJson());
        return;
    }

    /*
     * Save again because workspacePath may have been changed by the startup
     * workspace selection dialog.
     */
    saveApplicationConfigJson(m_AppParam->toJson());
}


