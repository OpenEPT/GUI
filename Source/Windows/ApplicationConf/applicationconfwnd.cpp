#include "applicationconfwnd.h"
#include "ui_applicationconfwnd.h"

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

#define APP_CONFIG_FIELD_WIDTH     170
#define APP_CONFIG_UNIT_WIDTH      40
#define APP_CONFIG_ROW_HEIGHT      28
#define APP_CONFIG_BUTTON_WIDTH    120
#define APP_CONFIG_BUTTON_HEIGHT   32

ApplicationConfWnd::ApplicationConfWnd(QWidget *parent,
                                       ParameterStore *params) :
    QWidget(parent),
    ui(new Ui::ApplicationConfWnd),
    m_params(params),
    tabWidget(nullptr),
    configurationStatusLabel(nullptr),
    changedFieldsLabel(nullptr),
    applyConfigButton(nullptr)
{
    ui->setupUi(this);

    setMinimumSize(800, 600);
    resize(800, 600);

    rebuildUi();

    if(m_params != nullptr)
    {
        connect(m_params,
                &ParameterStore::paramChanged,
                this,
                &ApplicationConfWnd::onParamChanged);
    }

    hide();

}

ApplicationConfWnd::~ApplicationConfWnd()
{
    delete ui;
}

void ApplicationConfWnd::setParameters(ParameterStore *params)
{
    if(m_params != nullptr)
    {
        disconnect(m_params,
                   &ParameterStore::paramChanged,
                   this,
                   &ApplicationConfWnd::onParamChanged);
    }

    m_params = params;

    if(m_params != nullptr)
    {
        connect(m_params,
                &ParameterStore::paramChanged,
                this,
                &ApplicationConfWnd::onParamChanged);
    }

    rebuildUi();
}

void ApplicationConfWnd::setParamValue(const QString &key, const QString &value)
{
    setFieldValue(key, value, true);
}

void ApplicationConfWnd::setFieldEditable(const QString &key, bool editable)
{
    if(fields.contains(key) == false)
    {
        return;
    }

    fields[key]->setReadOnly(!editable);
}

void ApplicationConfWnd::setConfigurationAppliedStatus(bool status)
{
    if(configurationStatusLabel == nullptr)
    {
        return;
    }

    if(status == true)
    {
        configurationStatusLabel->setText("Application status: Configuration applied successfully");

        for(auto it = fields.begin(); it != fields.end(); ++it)
        {
            appliedValues[it.key()] = it.value()->text();
        }
    }
    else
    {
        configurationStatusLabel->setText("Application status: Configuration apply failed");
    }

    refreshStatusBar();
}

void ApplicationConfWnd::rebuildUi()
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

void ApplicationConfWnd::clearUiState()
{
    fields.clear();
    appliedValues.clear();
    displayNames.clear();

    tabWidget = nullptr;
    configurationStatusLabel = nullptr;
    changedFieldsLabel = nullptr;
    applyConfigButton = nullptr;

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

QWidget *ApplicationConfWnd::createTab(Params::GroupId group)
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

QVBoxLayout *ApplicationConfWnd::createGroupLayout(Params::GroupId group)
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

        if(hasVisibleParam == false)
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

QGroupBox *ApplicationConfWnd::createSubGroupBox(Params::GroupId group,
                                                 Params::SubGroupId subGroup)
{
    if(m_params == nullptr)
    {
        return nullptr;
    }

    QList<Params::Param> params = m_params->getParamsBySubGroup(group, subGroup);

    QList<Params::Param> visibleParams;

    for(const Params::Param &param : params)
    {
        if(param.meta.visible == true)
        {
            visibleParams.append(param);
        }
    }

    if(visibleParams.isEmpty() == true)
    {
        return nullptr;
    }

    Params::SubGroupMeta subGroupMeta = m_params->getSubGroupMeta(subGroup);

    QGroupBox *groupBox = new QGroupBox(subGroupMeta.name, this);
    groupBox->setToolTip(subGroupMeta.description);
    groupBox->setLayout(createParamsGrid(visibleParams));

    return groupBox;
}

QGridLayout *ApplicationConfWnd::createParamsGrid(const QList<Params::Param> &params)
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

QWidget *ApplicationConfWnd::createParamWidget(const Params::Param &param)
{
    QWidget *controlWidget = new QWidget(this);
    QVBoxLayout *controlLayout = new QVBoxLayout(controlWidget);
    controlLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *nameLabel = new QLabel(param.meta.displayName, this);
    nameLabel->setMinimumHeight(APP_CONFIG_ROW_HEIGHT);
    nameLabel->setToolTip(param.meta.description);

    QHBoxLayout *fieldLayout = new QHBoxLayout();

    QLineEdit *field = new QLineEdit(this);
    field->setFixedWidth(APP_CONFIG_FIELD_WIDTH);
    field->setMinimumHeight(APP_CONFIG_ROW_HEIGHT);
    field->setToolTip(param.meta.description);

    QLabel *unitLabel = new QLabel(param.meta.unit, this);
    unitLabel->setFixedWidth(APP_CONFIG_UNIT_WIDTH);
    unitLabel->setMinimumHeight(APP_CONFIG_ROW_HEIGHT);

    fieldLayout->addWidget(field);
    fieldLayout->addWidget(unitLabel);
    fieldLayout->addStretch();

    controlLayout->addWidget(nameLabel);
    controlLayout->addLayout(fieldLayout);

    registerField(param, field);

    return controlWidget;
}

QHBoxLayout *ApplicationConfWnd::createButtonsRow()
{
    QHBoxLayout *buttonsLayout = new QHBoxLayout();

    configurationStatusLabel = new QLabel("Application status: Configuration not changed", this);
    configurationStatusLabel->setMinimumHeight(APP_CONFIG_ROW_HEIGHT);

    applyConfigButton = new QPushButton("Apply config", this);
    applyConfigButton->setFixedSize(APP_CONFIG_BUTTON_WIDTH, APP_CONFIG_BUTTON_HEIGHT);

    buttonsLayout->addWidget(configurationStatusLabel);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(applyConfigButton);

    connect(applyConfigButton,
            &QPushButton::clicked,
            this,
            &ApplicationConfWnd::onApplyConfigClicked);

    return buttonsLayout;
}

QVBoxLayout *ApplicationConfWnd::createStatusBarLayout()
{
    QVBoxLayout *statusLayout = new QVBoxLayout();

    changedFieldsLabel = new QLabel("Changed: None", this);
    changedFieldsLabel->setMinimumHeight(APP_CONFIG_ROW_HEIGHT);

    statusLayout->addWidget(changedFieldsLabel);

    return statusLayout;
}

void ApplicationConfWnd::registerField(const Params::Param &param,
                                       QLineEdit *field)
{
    const QString key = param.meta.key;

    fields[key] = field;
    displayNames[key] = param.meta.displayName;

    if(param.initialized == true)
    {
        field->setText(param.value.toString());
        appliedValues[key] = param.value.toString();
        field->setStyleSheet("");
        field->setReadOnly(!m_params->isEditable(key));
    }
    else
    {
        field->setText("");
        field->setStyleSheet("QLineEdit { background-color: #e0e0e0; color: #707070; }");
        field->setPlaceholderText("Not initialized");
        appliedValues[key] = QString();
        field->setReadOnly(true);
    }

    if(param.meta.access != Params::Access::ReadWrite)
    {
        field->setReadOnly(true);
        field->setStyleSheet("QLineEdit { background-color: #e0e0e0; color: #707070; }");
    }

    connect(field,
            &QLineEdit::textChanged,
            this,
            &ApplicationConfWnd::onFieldChanged);
}

void ApplicationConfWnd::setFieldValue(const QString &key,
                                       const QString &value,
                                       bool markAsApplied)
{
    if(fields.contains(key) == false)
    {
        return;
    }

    if(m_params == nullptr || m_params->hasParam(key) == false)
    {
        return;
    }

    QSignalBlocker blocker(fields[key]);

    fields[key]->setText(value);
    fields[key]->setPlaceholderText("");
    fields[key]->setStyleSheet("");

    if(m_params->getParam(key).meta.access != Params::Access::ReadWrite)
    {
        fields[key]->setReadOnly(true);
        fields[key]->setStyleSheet("QLineEdit { background-color: #e0e0e0; color: #707070; }");
    }
    else
    {
        fields[key]->setReadOnly(false);
    }

    if(markAsApplied == true)
    {
        appliedValues[key] = value;
    }

    refreshStatusBar();
}

QMap<QString, QString> ApplicationConfWnd::getChangedFields() const
{
    QMap<QString, QString> changedFields;

    for(auto it = fields.constBegin(); it != fields.constEnd(); ++it)
    {
        const QString key = it.key();

        if(m_params != nullptr)
        {
            if(m_params->isEditable(key) == false)
            {
                continue;
            }

            if(m_params->isInitialized(key) == false && it.value()->text().isEmpty())
            {
                continue;
            }
        }

        const QString currentValue = it.value()->text();
        const QString appliedValue = appliedValues.value(key);

        if(currentValue != appliedValue)
        {
            changedFields[key] = currentValue;
        }
    }

    return changedFields;
}

QStringList ApplicationConfWnd::getChangedFieldDisplayNames() const
{
    QStringList changedNames;

    QMap<QString, QString> changedFields = getChangedFields();

    for(auto it = changedFields.constBegin(); it != changedFields.constEnd(); ++it)
    {
        changedNames.append(displayNames.value(it.key(), it.key()));
    }

    return changedNames;
}

void ApplicationConfWnd::refreshStatusBar()
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
            configurationStatusLabel->setText("Application status: Configuration changed, not applied");
        }
    }
}

void ApplicationConfWnd::onFieldChanged()
{
    refreshStatusBar();
}

void ApplicationConfWnd::onParamChanged(QString key, QString value)
{
    setFieldValue(key, value, true);
}

void ApplicationConfWnd::onApplyConfigClicked()
{
    QMap<QString, QString> changedFields = getChangedFields();

    if(changedFields.isEmpty() == true)
    {
        if(configurationStatusLabel != nullptr)
        {
            configurationStatusLabel->setText("Application status: No configuration changes detected");
        }

        return;
    }

    if(configurationStatusLabel != nullptr)
    {
        configurationStatusLabel->setText("Application status: Configuration apply requested");
    }

    emit sigApplicationConfigSet(changedFields);
    emit sigConfigSet(changedFields);
}
