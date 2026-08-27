#include "configurationwnd.h"
#include "ui_configurationwnd.h"
#include "Processing/Parameters/deviceparamdefs.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QTabBar>
#include <QScrollArea>
#include <QFrame>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QSizePolicy>
#include <QSignalBlocker>
#include <QFile>
#include <QFileDialog>
#include <QComboBox>

#define CONFIG_LABEL_WIDTH     150
#define CONFIG_FIELD_WIDTH     170
#define CONFIG_UNIT_WIDTH      40
#define CONFIG_ROW_HEIGHT      28
#define CONFIG_BUTTON_WIDTH    32
#define CONFIG_BUTTON_HEIGHT   32

ConfigurationWnd::ConfigurationWnd(QWidget *parent,
                                   ParameterStore *params) :
    QWidget(parent),
    ui(new Ui::ConfigurationWnd),
    m_params(params),
    tabWidget(nullptr),
    configurationStatusLabel(nullptr),
    changedFieldsLabel(nullptr),
    setConfigButton(nullptr),
    acquireConfigButton(nullptr)
{
    ui->setupUi(this);

    setMinimumSize(900, 700);
    resize(900, 700);

    rebuildUi();

    if(m_params != nullptr)
    {
        connect(m_params,
                &ParameterStore::paramChanged,
                this,
                &ConfigurationWnd::onParamChanged);
    }
}

ConfigurationWnd::~ConfigurationWnd()
{
    delete ui;
}

void ConfigurationWnd::setParameters(ParameterStore *params)
{
    if(m_params != nullptr)
    {
        disconnect(m_params,
                   &ParameterStore::paramChanged,
                   this,
                   &ConfigurationWnd::onParamChanged);
    }

    m_params = params;

    if(m_params != nullptr)
    {
        connect(m_params,
                &ParameterStore::paramChanged,
                this,
                &ConfigurationWnd::onParamChanged);
    }

    rebuildUi();
}

void ConfigurationWnd::setParamValue(const QString &key, const QString &value)
{
    setFieldValue(key, value, true);
}

void ConfigurationWnd::setFieldEditable(const QString &key, bool editable)
{
    if(fields.contains(key) == false)
    {
        return;
    }

    setFieldWidgetEditable(fields[key], editable);
}

void ConfigurationWnd::setConfigurationAcquiredStatus(bool status)
{
    if(configurationStatusLabel == nullptr)
    {
        return;
    }

    configurationStatusLabel->setText(status ?
                                      "Device status: Configuration acquired successfully" :
                                      "Device status: Configuration acquire failed");

    refreshStatusBar();
}

void ConfigurationWnd::setConfigurationAppliedStatus(bool status)
{
    if(configurationStatusLabel == nullptr)
    {
        return;
    }

    if(status == true)
    {
        configurationStatusLabel->setText("Device status: Configuration applied successfully");

        for(auto it = fields.begin(); it != fields.end(); ++it)
        {
            appliedValues[it.key()] = getFieldValue(it.value());
        }
    }
    else
    {
        configurationStatusLabel->setText("Device status: Configuration apply failed");
    }

    refreshStatusBar();
}

void ConfigurationWnd::setBDContent(const QString &content)
{
    if(bdContentTextEdit == nullptr)
        return;

    QByteArray data = QByteArray::fromHex(content.toUtf8());
    m_currentBDData = data;
    QByteArray prev = m_prevBDData;

    QString formatted;
    int bytesPerLine = 16;
    int totalBytes = data.size();

    for(int i = 0; i < totalBytes; i++)
    {
        if(i % bytesPerLine == 0)
        {
            formatted += QString("%1: ")
                         .arg(i, 8, 16, QChar('0'))
                         .toUpper();
        }

        uint8_t byte = (uint8_t)data[i];

        QString hex = QString("%1").arg(byte, 2, 16, QChar('0')).toUpper();

        bool changed = (i < prev.size()) && (byte != (uint8_t)prev[i]);

        if(changed)
            formatted += "<span style=\"color:red;font-weight:bold;\">" + hex + "</span> ";
        else
            formatted += hex + " ";

        /* ASCII */
        if((i % bytesPerLine) == bytesPerLine - 1 || i == totalBytes - 1)
        {
            int lineStart = i - (i % bytesPerLine);
            int lineEnd = i;

            QString ascii = " |";

            for(int j = lineStart; j <= lineEnd; j++)
            {
                uint8_t c = (uint8_t)data[j];

                if(c >= 32 && c <= 126)
                    ascii += QChar(c);
                else
                    ascii += '.';
            }

            ascii += "|";

            formatted += ascii;
            formatted += "<br>";
        }
    }

    bdContentTextEdit->setHtml("<pre style='font-family:Courier New;'>" + formatted + "</pre>");

    m_prevBDData = data;
}

void ConfigurationWnd::rebuildUi()
{
    clearUiState();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    tabWidget = new QTabWidget(this);
    tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QTabBar *tabBar = tabWidget->tabBar();
    tabBar->setExpanding(true);
    tabBar->setUsesScrollButtons(false);

    if(m_params != nullptr)
    {
        QList<Params::GroupMeta> groups = m_params->getAllGroupMeta();

        for(const Params::GroupMeta &groupMeta : groups)
        {
            QWidget *tab = createTab(groupMeta.id);

            if(tab != nullptr)
            {
                tabWidget->addTab(tab, groupMeta.name);
            }
        }
    }

    mainLayout->addWidget(tabWidget);
    mainLayout->addLayout(createButtonsRow());
    mainLayout->addLayout(createStatusBarLayout());

    setLayout(mainLayout);

    refreshStatusBar();
}

void ConfigurationWnd::clearUiState()
{
    fields.clear();
    appliedValues.clear();
    displayNames.clear();

    tabWidget = nullptr;
    configurationStatusLabel = nullptr;
    changedFieldsLabel = nullptr;
    setConfigButton = nullptr;
    acquireConfigButton = nullptr;

    storeConfigButton = nullptr;
    resetDeviceButton = nullptr;

    bdContentTextEdit = nullptr;
    bdGetButton = nullptr;
    bdUpdateButton = nullptr;
    bdFormatButton = nullptr;
    bdExportButton = nullptr;
    bdProgressBar = nullptr;
    bdProgressLabel = nullptr;

    chargerBDContentTextEdit = nullptr;
    chargerBDGetButton = nullptr;
    chargerBDUpdateButton = nullptr;
    chargerBDFormatButton = nullptr;
    chargerBDExportButton = nullptr;
    chargerBDProgressBar = nullptr;
    chargerBDProgressLabel = nullptr;

    QLayout *oldLayout = layout();

    if(oldLayout != nullptr)
    {
        QLayoutItem *item = nullptr;

        while((item = oldLayout->takeAt(0)) != nullptr)
        {
            if(item->widget() != nullptr)
            {
                delete item->widget();
            }

            if(item->layout() != nullptr)
            {
                delete item->layout();
            }

            delete item;
        }

        delete oldLayout;
    }
}

QWidget *ConfigurationWnd::createTab(Params::GroupId group)
{
    QWidget *tab = new QWidget(this);
    QVBoxLayout *tabMainLayout = new QVBoxLayout(tab);

    QWidget *scrollContent = new QWidget(this);
    scrollContent->setLayout(createGroupLayout(group));

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidget(scrollContent);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    tabMainLayout->addWidget(scrollArea);

    return tab;
}

QVBoxLayout *ConfigurationWnd::createGroupLayout(Params::GroupId group)
{
    QVBoxLayout *groupLayout = new QVBoxLayout();

    if(m_params == nullptr)
    {
        groupLayout->addStretch();
        return groupLayout;
    }

    QList<Params::SubGroupMeta> subGroups = m_params->getAllSubGroupMeta();

    for(const Params::SubGroupMeta &subGroupMeta : subGroups)
    {
        QList<Params::Param> params =
            m_params->getParamsBySubGroup(group, subGroupMeta.id);

        bool hasVisibleParam = false;

        for(const Params::Param &param : params)
        {
            if(param.meta.visible == true)
            {
                hasVisibleParam = true;
                break;
            }
        }

        bool isDeviceBD =
            (group == static_cast<Params::GroupId>(DeviceParamDefs::DeviceConfig) &&
             subGroupMeta.id == static_cast<Params::SubGroupId>(DeviceParamDefs::FileStorage));

        bool isChargerBD =
            (group == static_cast<Params::GroupId>(DeviceParamDefs::ChargerConfig) &&
             subGroupMeta.id == static_cast<Params::SubGroupId>(DeviceParamDefs::ChargerBD));

        if(hasVisibleParam == false && isDeviceBD == false && isChargerBD == false)
        {
            continue;
        }

        QGroupBox *box = createSubGroupBox(group, subGroupMeta.id);

        if(box != nullptr)
        {
            groupLayout->addWidget(box);
        }
    }

    groupLayout->addStretch();

    return groupLayout;
}

QGroupBox *ConfigurationWnd::createSubGroupBox(Params::GroupId group, Params::SubGroupId subGroup)
{
    if(m_params == nullptr)
    {
        return nullptr;
    }

    QList<Params::Param> params = m_params->getParamsBySubGroup(group, subGroup);

    QList<Params::Param> visibleParams;

    bool isDeviceBD = (group == static_cast<Params::GroupId>(DeviceParamDefs::DeviceConfig) &&
                       subGroup == static_cast<Params::SubGroupId>(DeviceParamDefs::FileStorage));

    bool isChargerBD = (group == static_cast<Params::GroupId>(DeviceParamDefs::ChargerConfig) &&
                        subGroup == static_cast<Params::SubGroupId>(DeviceParamDefs::ChargerBD));

    for(const Params::Param &param : params)
    {
        if(param.meta.visible == true)
        {
            visibleParams.append(param);
        }
    }

    if(visibleParams.isEmpty() == true && isDeviceBD == false && isChargerBD == false)
    {
        return nullptr;
    }

    Params::SubGroupMeta subGroupMeta = m_params->getSubGroupMeta(subGroup);

    QGroupBox *groupBox = new QGroupBox(subGroupMeta.name, this);
    groupBox->setToolTip(subGroupMeta.description);

    if(isDeviceBD == true)
    {
        QVBoxLayout *vLayout = new QVBoxLayout();

        if(visibleParams.isEmpty() == false)
        {
            vLayout->addLayout(createParamsGrid(visibleParams));
        }

        vLayout->addWidget(createBDMemoryWidget());

        groupBox->setLayout(vLayout);
    }
    else if(isChargerBD == true)
    {
        QVBoxLayout *vLayout = new QVBoxLayout();

        if(visibleParams.isEmpty() == false)
        {
            vLayout->addLayout(createParamsGrid(visibleParams));
        }

        vLayout->addWidget(createChargerBDMemoryWidget());

        groupBox->setLayout(vLayout);
    }
    else
    {
        QGridLayout *grid = createParamsGrid(visibleParams);
        groupBox->setLayout(grid);
    }

    return groupBox;
}

QGridLayout *ConfigurationWnd::createParamsGrid(const QList<Params::Param> &params)
{
    QGridLayout *gridLayout = new QGridLayout();

    for(int i = 0; i < params.size(); i++)
    {
        QWidget *paramWidget = createParamWidget(params[i]);

        int row = i / 3;
        int column = i % 3;

        gridLayout->addWidget(paramWidget, row, column);
    }

    gridLayout->setColumnStretch(0, 1);
    gridLayout->setColumnStretch(1, 1);
    gridLayout->setColumnStretch(2, 1);

    return gridLayout;
}

QWidget *ConfigurationWnd::createParamWidget(const Params::Param &param)
{
    QWidget *controlWidget = new QWidget(this);
    QVBoxLayout *controlLayout = new QVBoxLayout(controlWidget);
    controlLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *nameLabel = new QLabel(param.meta.displayName, this);
    nameLabel->setMinimumHeight(CONFIG_ROW_HEIGHT);
    nameLabel->setToolTip(param.meta.description);

    QHBoxLayout *fieldLayout = new QHBoxLayout();

    QWidget *field = nullptr;

    if(param.meta.editor == Params::Editor::ComboBox)
    {
        QComboBox *comboBox = new QComboBox(this);

        comboBox->setFixedWidth(CONFIG_FIELD_WIDTH);
        comboBox->setMinimumHeight(CONFIG_ROW_HEIGHT);
        comboBox->setToolTip(param.meta.description);
        comboBox->addItems(param.meta.allowedValues);

        field = comboBox;
    }
    else
    {
        QLineEdit *lineEdit = new QLineEdit(this);

        lineEdit->setFixedWidth(CONFIG_FIELD_WIDTH);
        lineEdit->setMinimumHeight(CONFIG_ROW_HEIGHT);
        lineEdit->setToolTip(param.meta.description);

        field = lineEdit;
    }

    QLabel *unitLabel = new QLabel(param.meta.unit, this);
    unitLabel->setFixedWidth(CONFIG_UNIT_WIDTH);
    unitLabel->setMinimumHeight(CONFIG_ROW_HEIGHT);

    fieldLayout->addWidget(field);
    fieldLayout->addWidget(unitLabel);
    fieldLayout->addStretch();

    controlLayout->addWidget(nameLabel);
    controlLayout->addLayout(fieldLayout);

    registerField(param, field);

    return controlWidget;
}
QString ConfigurationWnd::getFieldValue(QWidget *field) const
{
    if(QLineEdit *lineEdit = qobject_cast<QLineEdit*>(field))
    {
        return lineEdit->text();
    }

    if(QComboBox *comboBox = qobject_cast<QComboBox*>(field))
    {
        return comboBox->currentText();
    }

    return QString();
}
void ConfigurationWnd::setFieldWidgetValue(QWidget *field, const QString &value)
{
    if(QLineEdit *lineEdit = qobject_cast<QLineEdit*>(field))
    {
        lineEdit->setText(value);
        return;
    }

    if(QComboBox *comboBox = qobject_cast<QComboBox*>(field))
    {
        int index = comboBox->findText(value);

        if(index >= 0)
        {
            comboBox->setCurrentIndex(index);
        }

        return;
    }
}
void ConfigurationWnd::setFieldWidgetEditable(QWidget *field, bool editable)
{
    if(QLineEdit *lineEdit = qobject_cast<QLineEdit*>(field))
    {
        lineEdit->setReadOnly(!editable);
        return;
    }

    if(QComboBox *comboBox = qobject_cast<QComboBox*>(field))
    {
        comboBox->setEnabled(editable);
        return;
    }
}
QHBoxLayout *ConfigurationWnd::createButtonsRow()
{
    QHBoxLayout *buttonsLayout = new QHBoxLayout();

    configurationStatusLabel = new QLabel("Device status: Configuration not acquired", this);
    configurationStatusLabel->setMinimumHeight(CONFIG_ROW_HEIGHT);

    setConfigButton = new QPushButton("Set", this);
    setConfigButton->setFixedSize(80, CONFIG_BUTTON_HEIGHT);

    storeConfigButton = new QPushButton("Store", this);
    storeConfigButton->setFixedSize(80, CONFIG_BUTTON_HEIGHT);

    acquireConfigButton = new QPushButton("Get", this);
    acquireConfigButton->setFixedSize(80, CONFIG_BUTTON_HEIGHT);

    resetDeviceButton = new QPushButton("Reset", this);
    resetDeviceButton->setFixedSize(80, CONFIG_BUTTON_HEIGHT);

    buttonsLayout->addWidget(configurationStatusLabel);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(setConfigButton);
    buttonsLayout->addWidget(storeConfigButton);
    buttonsLayout->addWidget(acquireConfigButton);
    buttonsLayout->addWidget(resetDeviceButton);

    connect(setConfigButton,
            &QPushButton::clicked,
            this,
            &ConfigurationWnd::onSetConfigClicked);

    connect(storeConfigButton,
            &QPushButton::clicked,
            this,
            &ConfigurationWnd::onStoreConfigClicked);

    connect(acquireConfigButton,
            &QPushButton::clicked,
            this,
            &ConfigurationWnd::onAcquireConfigClicked);

    connect(resetDeviceButton,
            &QPushButton::clicked,
            this,
            &ConfigurationWnd::onResetDevice);

    return buttonsLayout;
}

QVBoxLayout *ConfigurationWnd::createStatusBarLayout()
{
    QVBoxLayout *statusLayout = new QVBoxLayout();

    changedFieldsLabel = new QLabel("Changed: None", this);
    changedFieldsLabel->setMinimumHeight(CONFIG_ROW_HEIGHT);

    statusLayout->addWidget(changedFieldsLabel);

    return statusLayout;
}

void ConfigurationWnd::registerField(const Params::Param &param, QWidget *field)
{
    const QString key = param.meta.key;

    fields[key] = field;
    displayNames[key] = param.meta.displayName;

    if(param.initialized)
    {
        setFieldWidgetValue(field, param.value.toString());
        appliedValues[key] = param.value.toString();
        field->setStyleSheet("");
        setFieldWidgetEditable(field, m_params->isEditable(key));
    }
    else
    {
        appliedValues[key] = QString();
        setFieldWidgetEditable(field, false);

        if(QLineEdit *lineEdit = qobject_cast<QLineEdit*>(field))
        {
            lineEdit->setText("");
            lineEdit->setStyleSheet("QLineEdit { background-color: #e0e0e0; color: #707070; }");
            lineEdit->setPlaceholderText("Not acquired");
        }
        else if(QComboBox *comboBox = qobject_cast<QComboBox*>(field))
        {
            comboBox->setCurrentIndex(-1);
        }
    }

    if(QLineEdit *lineEdit = qobject_cast<QLineEdit*>(field))
    {
        connect(lineEdit, &QLineEdit::textChanged, this, &ConfigurationWnd::onFieldChanged);
    }
    else if(QComboBox *comboBox = qobject_cast<QComboBox*>(field))
    {
        connect(comboBox, &QComboBox::currentTextChanged, this, &ConfigurationWnd::onFieldChanged);
    }
}

void ConfigurationWnd::setFieldValue(const QString &key, const QString &value, bool markAsApplied)
{
    if(fields.contains(key) == false)
        return;

    QWidget *field = fields[key];

    QSignalBlocker blocker(field);

    setFieldWidgetValue(field, value);
    field->setStyleSheet("");

    if(m_params->getParam(key).meta.access != Params::Access::ReadWrite)
    {
        setFieldWidgetEditable(field, false);

        if(QLineEdit *lineEdit = qobject_cast<QLineEdit*>(field))
        {
            lineEdit->setStyleSheet("QLineEdit { background-color: #e0e0e0; color: #707070; }");
        }
    }
    else
    {
        setFieldWidgetEditable(field, true);
    }

    if(markAsApplied == true)
        appliedValues[key] = value;

    refreshStatusBar();
}

QMap<QString, QString> ConfigurationWnd::getChangedFields() const
{
    QMap<QString, QString> changedFields;

    for(auto it = fields.constBegin(); it != fields.constEnd(); ++it)
    {
        const QString key = it.key();
        const QString currentValue = getFieldValue(it.value());

        if(m_params != nullptr)
        {
            if(m_params->isEditable(key) == false)
                continue;

            if(m_params->isInitialized(key) == false && currentValue.isEmpty())
                continue;
        }

        const QString appliedValue = appliedValues.value(key);

        if(currentValue != appliedValue)
        {
            changedFields[key] = currentValue;
        }
    }

    return changedFields;
}

QMap<QString, QString> ConfigurationWnd::getChangedFields(Params::GroupId group) const
{
    QMap<QString, QString> changedFields;

    if(m_params == nullptr)
    {
        return changedFields;
    }

    QMap<QString, QString> allChangedFields = getChangedFields();

    for(auto it = allChangedFields.constBegin(); it != allChangedFields.constEnd(); ++it)
    {
        if(m_params->hasParam(it.key()) == false)
        {
            continue;
        }

        Params::ParamMeta meta = m_params->getParamMeta(it.key());

        if(meta.group == group)
        {
            changedFields[it.key()] = it.value();
        }
    }

    return changedFields;
}

QStringList ConfigurationWnd::getChangedFieldDisplayNames() const
{
    QStringList changedNames;

    QMap<QString, QString> changedFields = getChangedFields();

    for(auto it = changedFields.constBegin(); it != changedFields.constEnd(); ++it)
    {
        changedNames.append(displayNames.value(it.key(), it.key()));
    }

    return changedNames;
}

void ConfigurationWnd::refreshStatusBar()
{
    if(changedFieldsLabel == nullptr)
    {
        return;
    }

    QStringList changedNames = getChangedFieldDisplayNames();

    if(changedNames.isEmpty() == true)
    {
        changedFieldsLabel->setText("Changed: None");
    }
    else
    {
        changedFieldsLabel->setText("Changed: " + changedNames.join(", "));

        if(configurationStatusLabel != nullptr)
        {
            configurationStatusLabel->setText("Device status: Configuration changed, not applied");
        }
    }
}

bool ConfigurationWnd::isDeviceParam(const QString &key) const
{
    if(m_params == nullptr || m_params->hasParam(key) == false)
    {
        return false;
    }

    return m_params->getParamMeta(key).group ==
           static_cast<Params::GroupId>(DeviceParamDefs::Group::DeviceConfig);
}

bool ConfigurationWnd::isApplicationParam(const QString &key) const
{
    if(m_params == nullptr || m_params->hasParam(key) == false)
    {
        return false;
    }

    return m_params->getParamMeta(key).group ==
            static_cast<Params::GroupId>(DeviceParamDefs::Group::ApplicationConfig);
}
void ConfigurationWnd::onBDGetClicked()
{
    emit sigBDContentGetRequest();
}

void ConfigurationWnd::onBDUpdateClicked()
{
    QString filePath = QFileDialog::getOpenFileName(
                this,
                "Select file to upload",
                "",
                "Binary (*.bin);;Text (*.txt *.hex);;All Files (*)");

    if(filePath.isEmpty())
        return;

    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return;

    QByteArray data;

    /* ================= BIN FILE ================= */
    if(filePath.endsWith(".bin"))
    {
        data = file.readAll();
    }
    else
    {
        /* ================= TEXT / HEX FILE ================= */
        QString text = file.readAll();

        QString clean = text;
        clean.remove(' ');
        clean.remove('\n');
        clean.remove('\r');

        /* ako je HEX */
        QRegularExpression hexRegex("^[0-9A-Fa-f]+$");

        if(hexRegex.match(clean).hasMatch())
        {
            data = QByteArray::fromHex(clean.toUtf8());
        }
        else
        {
            /* fallback: ASCII tekst */
            data = text.toUtf8();
        }
    }

    file.close();

    if(data.isEmpty())
        return;

    emit sigBDContentSetRequest(data);
}

void ConfigurationWnd::onBDFormatClicked()
{
    emit sigBDFormatRequest();
}

void ConfigurationWnd::onBDExportClicked()
{
    QString filePath = QFileDialog::getSaveFileName(
                this,
                "Save memory content",
                "",
                "Binary (*.bin);;All Files (*)");

    if(filePath.isEmpty())
        return;

    if(m_currentBDData.isEmpty())
    {
        qDebug() << "No data available!";
        return;
    }

    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly))
        return;

    file.write(m_currentBDData);
    file.close();

    qDebug() << "Saved bytes:" << m_currentBDData.size();
}

void ConfigurationWnd::onChargerBDGetClicked()
{
    emit sigChargerBDContentGetRequest();
}

void ConfigurationWnd::onChargerBDUpdateClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Select file to upload", "", "Binary (*.bin);;Text (*.txt *.hex);;All Files (*)");

    if(filePath.isEmpty())
        return;

    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return;

    QByteArray data;

    if(filePath.endsWith(".bin"))
    {
        data = file.readAll();
    }
    else
    {
        QString text = file.readAll();

        QString clean = text;
        clean.remove(' ');
        clean.remove('\n');
        clean.remove('\r');

        QRegularExpression hexRegex("^[0-9A-Fa-f]+$");

        if(hexRegex.match(clean).hasMatch())
        {
            data = QByteArray::fromHex(clean.toUtf8());
        }
        else
        {
            data = text.toUtf8();
        }
    }

    file.close();

    if(data.isEmpty())
        return;

    emit sigChargerBDContentSetRequest(data);
}

void ConfigurationWnd::onChargerBDFormatClicked()
{
    emit sigChargerBDFormatRequest();
}

void ConfigurationWnd::onChargerBDExportClicked()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Save charger memory content", "", "Binary (*.bin);;All Files (*)");

    if(filePath.isEmpty())
        return;

    if(m_currentChargerBDData.isEmpty())
    {
        qDebug() << "No charger data available!";
        return;
    }

    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly))
        return;

    file.write(m_currentChargerBDData);
    file.close();

    qDebug() << "Saved charger bytes:" << m_currentChargerBDData.size();
}

void ConfigurationWnd::setBDProgress(int percent, const QString &text)
{
    if(bdProgressBar == nullptr || bdProgressLabel == nullptr)
        return;

    bdProgressBar->setVisible(true);
    bdProgressLabel->setVisible(true);

    bdProgressBar->setValue(percent);
    bdProgressBar->update();
    bdProgressLabel->setText(text);
}

void ConfigurationWnd::resetBDProgress()
{
    if(bdProgressBar == nullptr || bdProgressLabel == nullptr)
        return;

    bdProgressBar->setValue(0);
    bdProgressBar->setVisible(false);
    bdProgressLabel->setVisible(false);
}

void ConfigurationWnd::setChargerBDContent(const QString &content)
{
    if(chargerBDContentTextEdit == nullptr)
        return;

    QByteArray data = QByteArray::fromHex(content.toUtf8());
    m_currentChargerBDData = data;
    QByteArray prev = m_prevChargerBDData;

    QString formatted;
    int bytesPerLine = 16;
    int totalBytes = data.size();

    for(int i = 0; i < totalBytes; i++)
    {
        if(i % bytesPerLine == 0)
        {
            formatted += QString("%1: ").arg(i, 8, 16, QChar('0')).toUpper();
        }

        uint8_t byte = (uint8_t)data[i];

        QString hex = QString("%1").arg(byte, 2, 16, QChar('0')).toUpper();

        bool changed = (i < prev.size()) && (byte != (uint8_t)prev[i]);

        if(changed)
            formatted += "<span style=\"color:red;font-weight:bold;\">" + hex + "</span> ";
        else
            formatted += hex + " ";

        if((i % bytesPerLine) == bytesPerLine - 1 || i == totalBytes - 1)
        {
            int lineStart = i - (i % bytesPerLine);
            int lineEnd = i;

            QString ascii = " |";

            for(int j = lineStart; j <= lineEnd; j++)
            {
                uint8_t c = (uint8_t)data[j];

                if(c >= 32 && c <= 126)
                    ascii += QChar(c);
                else
                    ascii += '.';
            }

            ascii += "|";

            formatted += ascii;
            formatted += "<br>";
        }
    }

    chargerBDContentTextEdit->setHtml("<pre style='font-family:Courier New;'>" + formatted + "</pre>");

    m_prevChargerBDData = data;
}

void ConfigurationWnd::setChargerBDProgress(int percent, const QString &text)
{
    if(chargerBDProgressBar == nullptr || chargerBDProgressLabel == nullptr)
        return;

    chargerBDProgressBar->setVisible(true);
    chargerBDProgressLabel->setVisible(true);

    chargerBDProgressBar->setValue(percent);
    chargerBDProgressBar->update();
    chargerBDProgressLabel->setText(text);
}

void ConfigurationWnd::resetChargerBDProgress()
{
    if(chargerBDProgressBar == nullptr || chargerBDProgressLabel == nullptr)
        return;

    chargerBDProgressBar->setValue(0);
    chargerBDProgressBar->setVisible(false);
    chargerBDProgressLabel->setVisible(false);
}


QWidget* ConfigurationWnd::createBDMemoryWidget()
{
    QWidget *container = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(container);

    /* Header */
    QHBoxLayout *headerLayout = new QHBoxLayout();

    QLabel *title = new QLabel("Memory Content", this);
    QFont font = title->font();
    font.setBold(true);
    title->setFont(font);

    QString btnStyle = R"(
    QPushButton {
        background: transparent;
        border: none;
    }
    QPushButton:hover {
        background-color: rgba(255,255,255,30);
    }
    QPushButton:pressed {
        background-color: rgba(255,255,255,60);
    }
    QToolTip {
        background-color: white;
        color: black;
        border: 1px solid #ccc;
    }
    )";

    bdGetButton = new QPushButton(this);
    bdGetButton->setFixedSize(CONFIG_BUTTON_WIDTH, CONFIG_BUTTON_HEIGHT);
    bdGetButton->setIcon(QIcon(":/images/NewSet/upload.png"));
    bdGetButton->setIconSize(QSize(24, 24));
    bdGetButton->setToolTip("Get memory content");

    bdUpdateButton = new QPushButton(this);
    bdUpdateButton->setFixedSize(CONFIG_BUTTON_WIDTH, CONFIG_BUTTON_HEIGHT);
    bdUpdateButton->setIcon(QIcon(":/images/NewSet/download.png"));
    bdUpdateButton->setIconSize(QSize(24, 24));
    bdUpdateButton->setToolTip("Update memory content");

    bdFormatButton = new QPushButton(this);
    bdFormatButton->setFixedSize(CONFIG_BUTTON_WIDTH, CONFIG_BUTTON_HEIGHT);
    bdFormatButton->setIcon(QIcon(":/images/NewSet/format.png"));
    bdFormatButton->setIconSize(QSize(24, 24));
    bdFormatButton->setToolTip("Format memory");

    bdExportButton = new QPushButton(this);
    bdExportButton->setFixedSize(CONFIG_BUTTON_WIDTH, CONFIG_BUTTON_HEIGHT);
    bdExportButton->setIcon(QIcon(":/images/NewSet/export.png"));
    bdExportButton->setIconSize(QSize(24, 24));
    bdExportButton->setToolTip("Export memory to PC");

    bdGetButton->setStyleSheet(btnStyle);
    bdUpdateButton->setStyleSheet(btnStyle);
    bdFormatButton->setStyleSheet(btnStyle);
    bdExportButton->setStyleSheet(btnStyle);

    headerLayout->addWidget(title);
    headerLayout->addStretch();



    /* ===== Progress Section ===== */



    QHBoxLayout *progressLayout = new QHBoxLayout();


    bdProgressBar = new QProgressBar(this);
    bdProgressBar->setRange(0, 100);
    bdProgressBar->setValue(0);
    bdProgressBar->setTextVisible(true);
    bdProgressBar->setMinimumHeight(18);
    bdProgressBar->setMaximumHeight(18);

    bdProgressLabel = new QLabel("Idle", this);
    bdProgressLabel->setMinimumHeight(CONFIG_ROW_HEIGHT);
    bdProgressLabel->setMaximumHeight(CONFIG_ROW_HEIGHT);

    /* inicijalno sakriveno */
    bdProgressBar->setVisible(true);
    bdProgressLabel->setVisible(true);

    progressLayout->addWidget(bdProgressBar);
    progressLayout->addWidget(bdProgressLabel);

    headerLayout->addLayout(progressLayout);
    headerLayout->addStretch();
    headerLayout->addWidget(bdGetButton);
    headerLayout->addWidget(bdUpdateButton);
    headerLayout->addWidget(bdFormatButton);
    headerLayout->addWidget(bdExportButton);


    layout->addLayout(headerLayout);

    /* ===== Text area ===== */

    /* Text area */
    bdContentTextEdit = new QTextEdit(this);
    bdContentTextEdit->setReadOnly(true);
    bdContentTextEdit->setMinimumHeight(250);

    QFont mono("Courier New");
    bdContentTextEdit->setFont(mono);

    layout->addWidget(bdContentTextEdit);

    /* connections */
    connect(bdGetButton,
            &QPushButton::clicked,
            this,
            &ConfigurationWnd::onBDGetClicked);

    connect(bdUpdateButton,
            &QPushButton::clicked,
            this,
            &ConfigurationWnd::onBDUpdateClicked);

    connect(bdFormatButton,
            &QPushButton::clicked,
            this,
            &ConfigurationWnd::onBDFormatClicked);

    connect(bdExportButton,
            &QPushButton::clicked,
            this,
            &ConfigurationWnd::onBDExportClicked);

    return container;
}

QWidget *ConfigurationWnd::createChargerBDMemoryWidget()
{
    QWidget *container = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(container);

    QHBoxLayout *headerLayout = new QHBoxLayout();

    QLabel *title = new QLabel("Memory Content", this);
    QFont font = title->font();
    font.setBold(true);
    title->setFont(font);

    QString btnStyle = R"(
    QPushButton {
        background: transparent;
        border: none;
    }
    QPushButton:hover {
        background-color: rgba(255,255,255,30);
    }
    QPushButton:pressed {
        background-color: rgba(255,255,255,60);
    }
    QToolTip {
        background-color: white;
        color: black;
        border: 1px solid #ccc;
    }
    )";

    chargerBDGetButton = new QPushButton(this);
    chargerBDGetButton->setFixedSize(CONFIG_BUTTON_WIDTH, CONFIG_BUTTON_HEIGHT);
    chargerBDGetButton->setIcon(QIcon(":/images/NewSet/upload.png"));
    chargerBDGetButton->setIconSize(QSize(24, 24));
    chargerBDGetButton->setToolTip("Get charger memory content");

    chargerBDUpdateButton = new QPushButton(this);
    chargerBDUpdateButton->setFixedSize(CONFIG_BUTTON_WIDTH, CONFIG_BUTTON_HEIGHT);
    chargerBDUpdateButton->setIcon(QIcon(":/images/NewSet/download.png"));
    chargerBDUpdateButton->setIconSize(QSize(24, 24));
    chargerBDUpdateButton->setToolTip("Update charger memory content");

    chargerBDFormatButton = new QPushButton(this);
    chargerBDFormatButton->setFixedSize(CONFIG_BUTTON_WIDTH, CONFIG_BUTTON_HEIGHT);
    chargerBDFormatButton->setIcon(QIcon(":/images/NewSet/format.png"));
    chargerBDFormatButton->setIconSize(QSize(24, 24));
    chargerBDFormatButton->setToolTip("Format charger memory");

    chargerBDExportButton = new QPushButton(this);
    chargerBDExportButton->setFixedSize(CONFIG_BUTTON_WIDTH, CONFIG_BUTTON_HEIGHT);
    chargerBDExportButton->setIcon(QIcon(":/images/NewSet/export.png"));
    chargerBDExportButton->setIconSize(QSize(24, 24));
    chargerBDExportButton->setToolTip("Export charger memory to PC");

    chargerBDGetButton->setStyleSheet(btnStyle);
    chargerBDUpdateButton->setStyleSheet(btnStyle);
    chargerBDFormatButton->setStyleSheet(btnStyle);
    chargerBDExportButton->setStyleSheet(btnStyle);

    headerLayout->addWidget(title);
    headerLayout->addStretch();

    QHBoxLayout *progressLayout = new QHBoxLayout();

    chargerBDProgressBar = new QProgressBar(this);
    chargerBDProgressBar->setRange(0, 100);
    chargerBDProgressBar->setValue(0);
    chargerBDProgressBar->setTextVisible(true);
    chargerBDProgressBar->setMinimumHeight(18);
    chargerBDProgressBar->setMaximumHeight(18);

    chargerBDProgressLabel = new QLabel("Idle", this);
    chargerBDProgressLabel->setMinimumHeight(CONFIG_ROW_HEIGHT);
    chargerBDProgressLabel->setMaximumHeight(CONFIG_ROW_HEIGHT);

    chargerBDProgressBar->setVisible(true);
    chargerBDProgressLabel->setVisible(true);

    progressLayout->addWidget(chargerBDProgressBar);
    progressLayout->addWidget(chargerBDProgressLabel);

    headerLayout->addLayout(progressLayout);
    headerLayout->addStretch();
    headerLayout->addWidget(chargerBDGetButton);
    headerLayout->addWidget(chargerBDUpdateButton);
    headerLayout->addWidget(chargerBDFormatButton);
    headerLayout->addWidget(chargerBDExportButton);

    layout->addLayout(headerLayout);

    chargerBDContentTextEdit = new QTextEdit(this);
    chargerBDContentTextEdit->setReadOnly(true);
    chargerBDContentTextEdit->setMinimumHeight(250);

    QFont mono("Courier New");
    chargerBDContentTextEdit->setFont(mono);

    layout->addWidget(chargerBDContentTextEdit);

    connect(chargerBDGetButton, &QPushButton::clicked, this, &ConfigurationWnd::onChargerBDGetClicked);
    connect(chargerBDUpdateButton, &QPushButton::clicked, this, &ConfigurationWnd::onChargerBDUpdateClicked);
    connect(chargerBDFormatButton, &QPushButton::clicked, this, &ConfigurationWnd::onChargerBDFormatClicked);
    connect(chargerBDExportButton, &QPushButton::clicked, this, &ConfigurationWnd::onChargerBDExportClicked);

    return container;
}

void ConfigurationWnd::onFieldChanged()
{
    refreshStatusBar();
}

void ConfigurationWnd::onParamChanged(QString key, QString value)
{
    setFieldValue(key, value, true);
}

void ConfigurationWnd::onSetConfigClicked()
{
    QMap<QString, QString> changedFields = getChangedFields();

    if(changedFields.isEmpty() == true)
    {
        if(configurationStatusLabel != nullptr)
        {
            configurationStatusLabel->setText("Device status: No configuration changes detected");
        }

        return;
    }

    QMap<QString, QString> changedDeviceFields =
        getChangedFields(static_cast<Params::GroupId>(DeviceParamDefs::Group::DeviceConfig));

    QMap<QString, QString> changedApplicationFields =
        getChangedFields(static_cast<Params::GroupId>(DeviceParamDefs::Group::ApplicationConfig));

    QMap<QString, QString> changedChargerFields =
        getChangedFields(static_cast<Params::GroupId>(DeviceParamDefs::Group::ChargerConfig));

    if(configurationStatusLabel != nullptr)
    {
        configurationStatusLabel->setText("Device status: Configuration apply requested");
    }

    if(changedDeviceFields.isEmpty() == false)
    {
        emit sigDeviceConfigSet(changedDeviceFields);
    }

    if(changedApplicationFields.isEmpty() == false)
    {
        emit sigApplicationConfigSet(changedApplicationFields);
    }
    if(changedChargerFields.isEmpty() == false)
    {
        emit sigDeviceConfigSet(changedChargerFields);
    }

    emit sigConfigSet(changedFields);
}

void ConfigurationWnd::onAcquireConfigClicked()
{
    if(configurationStatusLabel != nullptr)
    {
        configurationStatusLabel->setText("Device status: Configuration acquire requested");
    }

    emit sigDeviceConfigAcquireRequest();
}

void ConfigurationWnd::onStoreConfigClicked()
{
    emit sigDeviceConfigStore();
}

void ConfigurationWnd::onResetDevice()
{
    emit sigResetDevice();
}
