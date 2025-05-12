#include "energycontrolwnd.h"
#include "ui_energycontrolwnd.h"

#include <QTabBar>
#include <QPushButton>
#include <QMdiSubWindow>
#include <QMdiArea>

EnergyControlWnd::EnergyControlWnd(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EnergyControlWnd)
{
    staticLoadCurrent = 100;
    staticChargingCurrent = 100;
    loadStartStopStatus = false;
    chargingStartStopStatus = false;
    chdischStartStopStatus = false;
    chdischCurrentStepIndex = 0;
    mode = ModeUknown;
    chdischCycleSteps[0].state = ChDschUnknown;
    chdischCycleSteps[1].state = ChDschUnknown;
    chdischCycleSteps[2].state = ChDschUnknown;
    chdischCycleSteps[3].state = ChDschUnknown;

    uVoltage = false;
    oVoltage = false;
    oCurrent = false;

    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Tab widget
    tabWidget = new QTabWidget(this);
    tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Stretch tab bar to fill width
    QTabBar *tabBar = tabWidget->tabBar();
    tabBar->setExpanding(true);
    tabBar->setUsesScrollButtons(false);

    // -------- Load Tab --------
    loadTab = new QWidget(this);
    QVBoxLayout *loadLayout = new QVBoxLayout(loadTab);


    // Top layout (configurable entries and controls)
    QVBoxLayout *loadTopLayout = new QVBoxLayout();
    // Section label with horizontal lines
    loadTopLayout->addLayout(createSectionHeader("Parameters"));
    QStringList loadTypes = {"Static", "Dynamic"};
    loadTopLayout->addLayout(createDropBoxEntry("Mode", loadTypes, loadComboBoxEdits));
    /*If static is selected*/

    connect(loadComboBoxEdits["Mode"], &QComboBox::currentTextChanged,this, &EnergyControlWnd::onLoadModeChanged);

    // --- Static mode widget ---
    loadCurrentWidget = new QWidget(this);
    QVBoxLayout *staticLayout = new QVBoxLayout(loadCurrentWidget);
    staticLayout->setContentsMargins(0, 0, 0, 0);
    staticLayout->addLayout(createEntryRow("Current", "mA", loadEntryEdits));
    loadEntryEdits["Current"]->setText(QString::number(staticLoadCurrent));
    staticLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    loadTopLayout->addWidget(loadCurrentWidget);

    dynamicWidget = new QWidget(this); // <-- add this to the header
    QVBoxLayout *dynamicLayout = new QVBoxLayout(dynamicWidget);
    dynamicLayout->setContentsMargins(0, 0, 0, 0);
    dynamicLayout->addLayout(createQTextEditRow("Current Profile", loadTextEdits));
    dynamicLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    dynamicWidget->setVisible(false); // ← hide the *container*, not the textedit
    loadTopLayout->addWidget(dynamicWidget);

    /*IThis should always be visible*/
    loadTopLayout->addLayout(createButtonRow("Set", "Set", loadButtons));
    loadTopLayout->addLayout(createButtonRow("StartStop", "Start", loadButtons));



    connect(loadButtons["Set"], &QPushButton::clicked, this, &EnergyControlWnd::onLoadSet);
    connect(loadButtons["StartStop"], &QPushButton::clicked, this, &EnergyControlWnd::onLoadStartStop);

    dischargeTimer = new QTimer(this);
    dischargeTimer->setInterval(1000);
    connect(dischargeTimer, &QTimer::timeout, this, &EnergyControlWnd::onDischargeTimerTimeout);

    // Bottom layout (status information)
    QVBoxLayout *loadStatusLayout = new QVBoxLayout();
    loadStatusLayout->addLayout(createSectionHeader("Status"));

    // Create a horizontal layout inside the status section
    QHBoxLayout *statusRowLayout = new QHBoxLayout();

    // ----- Left side (start/stop times) -----
    QVBoxLayout *leftTimesLayout = new QVBoxLayout();
    leftTimesLayout->addLayout(createTimeEntryRow("Discharge Start Time", loadTimeEdits, false, true));
    leftTimesLayout->addLayout(createTimeEntryRow("Discharge Stop Time", loadTimeEdits, false, true));

    // Add left times layout to horizontal row
    statusRowLayout->addLayout(leftTimesLayout);

    // ----- Right side (duration) -----
    QVBoxLayout *rightDurationLayout = new QVBoxLayout();
    rightDurationLayout->addLayout(createTimeEntryRow("Discharge Duration", loadTimeEdits, false, true));

    // Add right layout to horizontal row
    statusRowLayout->addLayout(rightDurationLayout);

    // Add the assembled row into the main load status layout
    loadStatusLayout->addLayout(statusRowLayout);

    // Assemble both into the main load layout
    loadLayout->addLayout(loadTopLayout);
    loadLayout->addLayout(loadStatusLayout);

    loadTab->setLayout(loadLayout);
    tabWidget->addTab(loadTab, "Load");

    // -------- Charger Tab --------
    chargerTab = new QWidget(this);
    QVBoxLayout *chargerLayout = new QVBoxLayout(chargerTab);

    // Top layout (configurable entries and controls)
    QVBoxLayout *chargerTopLayout = new QVBoxLayout();
    chargerTopLayout->addLayout(createSectionHeader("Parameters"));
    chargerTopLayout->addLayout(createEntryRow("Current", "mA", chargerEntryEdits));
    chargerTopLayout->addLayout(createEntryRow("Term Voltage", "V", chargerEntryEdits));
    chargerTopLayout->addLayout(createEntryRow("Term Current", "%", chargerEntryEdits));
    chargerTopLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    chargerTopLayout->addLayout(createButtonRow("Set", "Set", chargerButtons));
    chargerTopLayout->addLayout(createButtonRow("StartStop", "Start", chargerButtons));

    chargeTimer = new QTimer(this);
    chargeTimer->setInterval(1000);
    connect(chargeTimer, &QTimer::timeout, this, &EnergyControlWnd::onChargingTimerTimeout);

    connect(chargerEntryEdits["Current"], &QLineEdit::textChanged, this, &EnergyControlWnd::onChargerEntryChanged);
    connect(chargerEntryEdits["Term Voltage"], &QLineEdit::textChanged, this, &EnergyControlWnd::onChargerEntryChanged);
    connect(chargerEntryEdits["Term Current"], &QLineEdit::textChanged, this, &EnergyControlWnd::onChargerEntryChanged);

    // Initialize all entries as "not changed"
    chargerEntryChanged["Current"] = false;
    chargerEntryChanged["Term Voltage"] = false;
    chargerEntryChanged["Term Current"] = false;

    connect(chargerButtons["Set"], &QPushButton::clicked, this, &EnergyControlWnd::onChargingConfigSet);
    connect(chargerButtons["StartStop"], &QPushButton::clicked, this, &EnergyControlWnd::onChargingStartStop);

    // Bottom layout (status information)
    QVBoxLayout *chargerStatusLayout = new QVBoxLayout();
    chargerStatusLayout->addLayout(createSectionHeader("Status"));

    // Create a horizontal layout inside the charger status section
    QHBoxLayout *chargerStatusRowLayout = new QHBoxLayout();

    // ----- Left side (start/stop times) -----
    QVBoxLayout *chargerLeftTimesLayout = new QVBoxLayout();
    chargerLeftTimesLayout->addLayout(createTimeEntryRow("Charging Start Time", chargerTimeEdits, false, true));
    chargerLeftTimesLayout->addLayout(createTimeEntryRow("Charging Stop Time", chargerTimeEdits, false, true));

    // Add left times layout to horizontal row
    chargerStatusRowLayout->addLayout(chargerLeftTimesLayout);

    // ----- Right side (duration) -----
    QVBoxLayout *chargerRightDurationLayout = new QVBoxLayout();
    chargerRightDurationLayout->addLayout(createTimeEntryRow("Charging Duration", chargerTimeEdits, false, true));

    // Add right layout to horizontal row
    chargerStatusRowLayout->addLayout(chargerRightDurationLayout);

    // Add the assembled row into the main charger status layout
    chargerStatusLayout->addLayout(chargerStatusRowLayout);

    // Assemble both into the main charger layout
    chargerLayout->addLayout(chargerTopLayout);
    chargerLayout->addLayout(chargerStatusLayout);

    chargerTab->setLayout(chargerLayout);
    tabWidget->addTab(chargerTab, "Charger");

    // -------- Charge/Discharge Tab --------
    chdischTab = new QWidget(this);
    QVBoxLayout *chdischLayout = new QVBoxLayout(chdischTab);


    // === Top layout (parameters + radio group) ===
    QVBoxLayout *chdischTopLayout = new QVBoxLayout();

    // --- Horizontal layout: left (entries) + right (radio group) ---
    QHBoxLayout *paramAndRadioRowLayout = new QHBoxLayout();

    // --- Left side: parameter entries ---
    QVBoxLayout *paramLeftLayout = new QVBoxLayout();
    paramLeftLayout->addLayout(createSectionHeader("Parameters"));
    paramLeftLayout->addLayout(createEntryRow("Discharge Current", "mA", chdischEntryEdits));
    paramLeftLayout->addLayout(createTimeEntryRow("Discharge Relax Time", chdischTimeEdits));
    paramLeftLayout->addLayout(createEntryRow("Charge Current", "mA", chdischEntryEdits));
    paramLeftLayout->addLayout(createTimeEntryRow("Charge Relax Time", chdischTimeEdits));
    paramLeftLayout->addLayout(createEntryRow("Cycles", "", chdischEntryEdits));

    // --- Right side: radio buttons ("Start with") ---
    QVBoxLayout *paramRightLayout = new QVBoxLayout();
    paramRightLayout->addLayout(createSectionHeader("Energy Control"));
    QStringList radioNames = {"Discharge", "Charge"};
    paramRightLayout->addLayout(createRadioButtonRow("Start with", 2, radioNames, chdischDirectionGroups));
    paramRightLayout->addLayout(createCheckButtonRow("Write To File", chdischCheckBox));
    paramRightLayout->addStretch();

    connect(chdischDirectionGroups["Start with"], QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
            this, &EnergyControlWnd::onChdischDirectionChanged);
    connect(chdischCheckBox["Write To File"], &QCheckBox::toggled,
            this, &EnergyControlWnd::onChDschWriteToFileToogled);

    // --- Combine left + right ---
    paramAndRadioRowLayout->addLayout(paramLeftLayout);
    paramAndRadioRowLayout->addLayout(paramRightLayout);
    paramAndRadioRowLayout->addStretch();

    // --- Add combined row to top layout ---
    chdischTopLayout->addLayout(paramAndRadioRowLayout);

    // --- Add buttons below ---
    chdischTopLayout->addLayout(createButtonRow("StartStop", "Start", chdischButtons));


    connect(chdischButtons["Set"], &QPushButton::clicked, this, &EnergyControlWnd::onChDschConfigSet);
    connect(chdischButtons["StartStop"], &QPushButton::clicked, this, &EnergyControlWnd::onChDschStartStop);


    // Bottom layout (status)
    QVBoxLayout *chdischStatusLayout = new QVBoxLayout();
    chdischStatusLayout->addLayout(createSectionHeader("Status"));

    // Create a horizontal layout inside the status section
    QHBoxLayout *chdischStatusRowLayout = new QHBoxLayout();

    // ----- Left side (start/stop times) -----
    QVBoxLayout *chdischLeftTimesLayout = new QVBoxLayout();
    chdischLeftTimesLayout->addLayout(createTimeEntryRow("Charge Start Time", chdischTimeEdits, false, true));
    chdischLeftTimesLayout->addLayout(createTimeEntryRow("Charge Stop Time", chdischTimeEdits, false, true));
    chdischLeftTimesLayout->addLayout(createTimeEntryRow("Discharge Start Time", chdischTimeEdits, false, true));
    chdischLeftTimesLayout->addLayout(createTimeEntryRow("Discharge Stop Time", chdischTimeEdits, false, true));

    // ----- Right side (duration and relax timeouts) -----
    QVBoxLayout *chdischRightDurationLayout = new QVBoxLayout();
    chdischRightDurationLayout->addLayout(createTimeEntryRow("Charge Duration", chdischTimeEdits, false, true));
    chdischRightDurationLayout->addLayout(createTimeEntryRow("Charge Relax Timeout", chdischTimeEdits, false, true));
    chdischRightDurationLayout->addLayout(createTimeEntryRow("Discharge Duration", chdischTimeEdits, false, true));
    chdischRightDurationLayout->addLayout(createTimeEntryRow("Discharge Relax Timeout", chdischTimeEdits, false, true));
    chdischRightDurationLayout->addStretch(); // Keep everything aligned to top

    // Assemble horizontal layout
    chdischStatusRowLayout->addLayout(chdischLeftTimesLayout);
    chdischStatusRowLayout->addLayout(chdischRightDurationLayout);

    // Add to main Charge/Discharge status layout
    chdischStatusLayout->addLayout(chdischStatusRowLayout);
    chdischStatusLayout->addLayout(createCycleDirectionStatusRow());
    QStringList checkBoxesName = {"1", "2", "3", "4"};
    chdischStatusLayout->addLayout(createCheckBoxesRow(4, checkBoxesName, chdischWriteToFileCheckBox));


    QHBoxLayout *nextButtonLayout = new QHBoxLayout(chdischTab);
    nextButtonLayout->addLayout(createButtonRow("NextStep", "Next", chdischButtons));
    chdischStatusLayout->addLayout(nextButtonLayout);

    chdischButtons["NextStep"]->setEnabled(false);
    connect(chdischButtons["NextStep"], &QPushButton::clicked, this, &EnergyControlWnd::onChDschNextStep);

    // Assemble both into main layout
    chdischLayout->addLayout(chdischTopLayout);
    chdischLayout->addLayout(chdischStatusLayout);

    chdischChargeTimer = new QTimer(this);
    chdischChargeTimer->setInterval(1000);
    connect(chdischChargeTimer, &QTimer::timeout, this, &EnergyControlWnd::onChDschChargingTimerTimeout);
    chdischDischargeTimer = new QTimer(this);
    chdischDischargeTimer->setInterval(1000);
    connect(chdischDischargeTimer, &QTimer::timeout, this, &EnergyControlWnd::onChDschDischargeTimerTimeout);


    chdischTab->setLayout(chdischLayout);
    tabWidget->addTab(chdischTab, "Charge/Discharge");

    // Add tab widget to layout
    mainLayout->addWidget(tabWidget);

    // ----- Status Section Layout -----
    QVBoxLayout *statusSectionLayout = new QVBoxLayout();

    // ===== 1. Protections Section =====
    statusSectionLayout->addLayout(createSectionHeader("Protections"));
    QHBoxLayout *protectionRowLayout = new QHBoxLayout();
    protectionRowLayout->addLayout(createIndicationStatusRow("UnderVoltage", "Under Voltage", statusIndicators));
    protectionRowLayout->addLayout(createIndicationStatusRow("OverVoltage", "Over Voltage", statusIndicators));
    protectionRowLayout->addLayout(createIndicationStatusRow("OverCurrent", "Over Current", statusIndicators));
    statusSectionLayout->addLayout(protectionRowLayout);

    // Reset Protections Button
    statusSectionLayout->addLayout(createButtonRow("ResetProtection", "Reset", protectionButtons));
    connect(protectionButtons["ResetProtection"], &QPushButton::clicked, this, &EnergyControlWnd::onResetProtection);

    // ===== 2. Controls Section =====
    statusSectionLayout->addLayout(createSectionHeader("Controls"));
    QHBoxLayout *controlRowLayout = new QHBoxLayout();
    controlRowLayout->addLayout(createControlRow("LoadDisable", "Load", statusControlButtons));
    controlRowLayout->addLayout(createControlRow("PPathDisable", "PPath", statusControlButtons));
    controlRowLayout->addLayout(createControlRow("BatteryDisable", "Battery", statusControlButtons));
    statusSectionLayout->addLayout(controlRowLayout);

    connect(statusControlButtons["LoadDisable"], &QPushButton::clicked, this, &EnergyControlWnd::onLoadStatusChanged);
    connect(statusControlButtons["PPathDisable"], &QPushButton::clicked, this, &EnergyControlWnd::onPPathStatusChanged);
    connect(statusControlButtons["BatteryDisable"], &QPushButton::clicked, this, &EnergyControlWnd::onBatteryStatusChanged);

    // ===== 3. Charging Info Section (split into two horizontal panels) =====
    QHBoxLayout *chargingInfoLayout = new QHBoxLayout();

    // --- Left: Configuration Status ---
    QVBoxLayout *configStatusLayout = new QVBoxLayout();
    configStatusLayout->addLayout(createSectionHeader("Configuration Status"));
    configurationStatus = new QLabel("Configuration: Configured", this);
    configurationStatus->setMinimumHeight(ENTRY_ROW_HEIGHT);
    configStatusLayout->addWidget(configurationStatus);
    configStatusLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    chargingInfoLayout->addLayout(configStatusLayout);

    // --- Right: Charging Status ---
    QVBoxLayout *chargingStatusLayout = new QVBoxLayout();
    chargingStatusLayout->addLayout(createSectionHeader("Charging Status"));
    chargingStatus = new QLabel("Charging state: Idle", this);
    chargingStatus->setMinimumHeight(ENTRY_ROW_HEIGHT);
    chargingStatusLayout->addWidget(chargingStatus);
    chargingStatusLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    chargingInfoLayout->addLayout(chargingStatusLayout);

    // Add to status section
    statusSectionLayout->addLayout(chargingInfoLayout);

    // ===== Set Initial States =====
    setConfigurationState(Configured);
    setChargingState(Unknown);

    // ----- Add status section to main layout -----
    mainLayout->addLayout(statusSectionLayout);

    this->setFixedSize(this->sizeHint());
}

Mode EnergyControlWnd::getMode()
{
    return mode;
}



QHBoxLayout* EnergyControlWnd::createEntryRow(const QString& entryName,
                                              const QString& unit,
                                              QMap<QString, QLineEdit*>& entryMap)
{
    QHBoxLayout *rowLayout = new QHBoxLayout();

    QLabel *label = new QLabel(entryName, this);
    label->setObjectName(entryName + "Label");
    label->setFixedSize(ENTRY_LABEL_WIDTH, ENTRY_ROW_HEIGHT);

    QLineEdit *valueEdit = new QLineEdit(this);
    valueEdit->setObjectName(entryName + "Value");
    valueEdit->setFixedSize(ENTRY_EDIT_WIDTH, ENTRY_ROW_HEIGHT);

    QString unitText = unit.isEmpty() ? "" : "[" + unit + "]";
    QLabel *unitLabel = new QLabel(unitText, this);
    unitLabel->setObjectName(entryName + "Unit");
    unitLabel->setFixedSize(ENTRY_UNIT_WIDTH, ENTRY_ROW_HEIGHT);

    // Register in provided map
    entryMap.insert(entryName, valueEdit);

    rowLayout->addWidget(label);
    rowLayout->addWidget(valueEdit);
    rowLayout->addWidget(unitLabel);
    rowLayout->addStretch();

    return rowLayout;
}
QHBoxLayout* EnergyControlWnd::createCheckButtonRow(const QString& entryName,
                                                    QMap<QString, QCheckBox*>& checkBoxMap)
{
    QHBoxLayout *rowLayout = new QHBoxLayout();

    QLabel *label = new QLabel(entryName, this);
    label->setObjectName(entryName + "Label");
    label->setFixedSize(ENTRY_LABEL_WIDTH, ENTRY_ROW_HEIGHT);

    QCheckBox *checkBox = new QCheckBox(this);
    checkBox->setObjectName(entryName + "CheckBox");
    checkBox->setFixedSize(ENTRY_EDIT_WIDTH, ENTRY_ROW_HEIGHT); // Same width as QLineEdit for layout consistency

    // Register in provided map
    checkBoxMap.insert(entryName, checkBox);

    rowLayout->addWidget(label);
    rowLayout->addWidget(checkBox);
    rowLayout->addStretch();

    return rowLayout;
}

QHBoxLayout* EnergyControlWnd::createCheckBoxesRow(int count, const QStringList& checkBoxNames, QMap<QString, QCheckBox*>& checkBoxMap)
{
    QHBoxLayout *layout = new QHBoxLayout();
    layout->setSpacing(0);  // Use manual spacers

    // Total max width
    const int totalTargetWidth = 2 * (ENTRY_LABEL_WIDTH + ENTRY_EDIT_WIDTH + ENTRY_UNIT_WIDTH);
    const int numSpacers = count - 1;
    const int checkboxWidth = totalTargetWidth / (count + numSpacers);
    const int spacerWidth = checkboxWidth;

    for (int i = 0; i < count; ++i) {
        QString key = checkBoxNames[i];

        // Create container widget to center checkbox inside fixed width
        QWidget *container = new QWidget(this);
        container->setFixedWidth(checkboxWidth);
        container->setFixedHeight(ENTRY_ROW_HEIGHT);

        QHBoxLayout *containerLayout = new QHBoxLayout(container);
        containerLayout->setContentsMargins(0, 0, 0, 0);
        containerLayout->setAlignment(Qt::AlignCenter);

        QCheckBox *checkBox = new QCheckBox(container);
        checkBox->setObjectName(key + "CheckBox");
        checkBox->setFixedSize(20, 20);  // fixed checkbox size
        checkBox->setVisible(false);
        checkBox->setEnabled(false);
        checkBox->setCheckState(Qt::Checked);
        containerLayout->addWidget(checkBox);

        checkBoxMap.insert(key, checkBox);
        layout->addWidget(container);

        if (i < count - 1) {
            QSpacerItem *spacer = new QSpacerItem(spacerWidth, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
            layout->addSpacerItem(spacer);
        }
    }

    layout->addStretch();
    return layout;
}

QHBoxLayout* EnergyControlWnd::createTimeEntryRow(const QString &entryName,
                                                  QMap<QString, QTimeEdit*> &entryMap,
                                                  bool enabled,
                                                  bool readOnly)
{
    QHBoxLayout *rowLayout = new QHBoxLayout();

    QLabel *label = new QLabel(entryName, this);
    label->setObjectName(entryName + "Label");
    label->setFixedSize(ENTRY_LABEL_WIDTH, ENTRY_ROW_HEIGHT);

    QTimeEdit *timeEdit = new QTimeEdit(this);
    timeEdit->setObjectName(entryName + "Time");
    timeEdit->setFixedSize(ENTRY_EDIT_WIDTH, ENTRY_ROW_HEIGHT);
    timeEdit->setDisplayFormat("hh:mm:ss");
    timeEdit->setEnabled(enabled);  // Set enabled/disabled state
    if(readOnly)
    {
        timeEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
        timeEdit->setStyleSheet("background-color: transparent; border: none;");
    }
    timeEdit->setReadOnly(readOnly);

    // Store in the map
    entryMap.insert(entryName, timeEdit);

    rowLayout->addWidget(label);
    rowLayout->addWidget(timeEdit);
    rowLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

    return rowLayout;
}
QHBoxLayout* EnergyControlWnd::createButtonRow(const QString& entryName, const QString& displayName, QMap<QString, QPushButton*>& buttonMap)
{
    QHBoxLayout *layout = new QHBoxLayout();

    QPushButton *button = new QPushButton(displayName, this);
    button->setObjectName(entryName + "Button");
    button->setFixedSize(BUTTON_WIDTH, BUTTON_HEIGHT);

    buttonMap.insert(entryName, button);

    layout->addStretch();
    layout->addWidget(button);

    return layout;
}
QHBoxLayout* EnergyControlWnd::createSectionHeader(const QString &title)
{
    QHBoxLayout *layout = new QHBoxLayout();

    QFrame *leftLine = new QFrame(this);
    leftLine->setFrameShape(QFrame::HLine);
    leftLine->setFrameShadow(QFrame::Sunken);
    leftLine->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    QLabel *label = new QLabel(title, this);
    label->setObjectName(title + "HeaderLabel");
    label->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum); // 👈 Makes it minimal
    label->setStyleSheet("font-weight: bold; padding: 0 4px;");

    QFrame *rightLine = new QFrame(this);
    rightLine->setFrameShape(QFrame::HLine);
    rightLine->setFrameShadow(QFrame::Sunken);
    rightLine->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    layout->addWidget(leftLine);
    layout->addWidget(label);
    layout->addWidget(rightLine);

    return layout;
}
QHBoxLayout* EnergyControlWnd::createIndicationStatusRow(const QString &entryName,
                                                         const QString &statusText,
                                                         QMap<QString, QLabel*> &statusMap)
{
    QHBoxLayout *layout = new QHBoxLayout();

    const int circleSize = 32; // choose your desired circle diameter

    QLabel *indicatorLabel = new QLabel(this);
    indicatorLabel->setObjectName(entryName + "Indicator");
    indicatorLabel->setFixedSize(circleSize, circleSize);

    // Set to circular shape: radius = size / 2
    indicatorLabel->setStyleSheet(
        "background-color: rgba(128, 128, 128, 180);"
        "border-radius: 16px;"  // half of circleSize
        "border: 1px solid black;" // optional for visibility
    );

    // Align the circle vertically center
    layout->setAlignment(indicatorLabel, Qt::AlignVCenter);

    QLabel *statusLabel = new QLabel(statusText, this);
    statusLabel->setObjectName(entryName + "StatusText");
    statusLabel->setFixedSize(ENTRY_EDIT_WIDTH + ENTRY_UNIT_WIDTH, ENTRY_ROW_HEIGHT);

    layout->addWidget(indicatorLabel);
    layout->addWidget(statusLabel);
    layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

    statusMap.insert(entryName, indicatorLabel);

    return layout;
}
QHBoxLayout* EnergyControlWnd::createDropBoxEntry(const QString& entryName,
                                                  const QStringList& values,
                                                  QMap<QString, QComboBox*>& comboMap)
{
    QHBoxLayout *rowLayout = new QHBoxLayout();

    QLabel *label = new QLabel(entryName, this);
    label->setObjectName(entryName + "Label");
    label->setFixedSize(ENTRY_LABEL_WIDTH, ENTRY_ROW_HEIGHT);

    QComboBox *comboBox = new QComboBox(this);
    comboBox->setObjectName(entryName + "ComboBox");
    comboBox->addItems(values);
    comboBox->setFixedSize(ENTRY_EDIT_WIDTH + ENTRY_UNIT_WIDTH, ENTRY_ROW_HEIGHT);  // fills both edit+unit space

    comboMap.insert(entryName, comboBox);

    rowLayout->addWidget(label);
    rowLayout->addWidget(comboBox);
    rowLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

    return rowLayout;
}

QVBoxLayout *EnergyControlWnd::createQTextEditRow(const QString &entryName, QMap<QString, QTextEdit *> &entryMap)
{
    QVBoxLayout *rowLayout = new QVBoxLayout();

    QLabel *label = new QLabel(entryName, this);
    label->setObjectName(entryName + "Label");
    label->setFixedSize(ENTRY_LABEL_WIDTH + ENTRY_EDIT_WIDTH + ENTRY_UNIT_WIDTH, ENTRY_ROW_HEIGHT); // span full width

    QTextEdit *textEdit = new QTextEdit(this);
    textEdit->setObjectName(entryName + "TextEdit");
    textEdit->setFixedHeight(80);  // Adjust as needed
    entryMap.insert(entryName, textEdit);

    rowLayout->addWidget(label);
    rowLayout->addWidget(textEdit);

    return rowLayout;
}

QHBoxLayout *EnergyControlWnd::createControlRow(const QString &entryName, const QString &labelPrefix, QMap<QString, QPushButton *> &buttonMap)
{
    QHBoxLayout *rowLayout = new QHBoxLayout();

    QPushButton *button = new QPushButton(this);
    button->setObjectName(entryName + "Button");
    button->setCheckable(true);
    button->setFixedSize(INDICATOR_WIDTH, ENTRY_ROW_HEIGHT);
    buttonMap.insert(entryName, button);

    QLabel *stateLabel = new QLabel(labelPrefix + " Enabled", this);
    stateLabel->setObjectName(entryName + "StateLabel");
    stateLabel->setFixedSize(ENTRY_EDIT_WIDTH + ENTRY_UNIT_WIDTH, ENTRY_ROW_HEIGHT);
    statusControlLabels.insert(entryName, stateLabel);

    // Prevent button from toggling itself on click
//    connect(button, &QPushButton::pressed, this, [=]() {
//        // Immediately revert its internal toggle — do not visually change state
//        button->setDown(false);
//    });

    rowLayout->addWidget(button);
    rowLayout->addWidget(stateLabel);
    rowLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

    return rowLayout;
}

QVBoxLayout* EnergyControlWnd::createRadioButtonRow(const QString &groupName,
                                                    int count,
                                                    const QStringList &names,
                                                    QMap<QString, QButtonGroup*> &radioGroups)
{
    const int MIN_ROW_WIDTH = ENTRY_LABEL_WIDTH + ENTRY_EDIT_WIDTH + ENTRY_UNIT_WIDTH;

    QVBoxLayout *layout = new QVBoxLayout();

    if (!groupName.isEmpty()) {
        QLabel *label = new QLabel(groupName, this);
        label->setMinimumWidth(MIN_ROW_WIDTH);
        label->setFixedHeight(ENTRY_ROW_HEIGHT);
        layout->addWidget(label);
    }

    QButtonGroup *buttonGroup = new QButtonGroup(this);
    buttonGroup->setExclusive(true);

    for (int i = 0; i < count && i < names.size(); ++i) {
        QRadioButton *radio = new QRadioButton(names[i], this);
        radio->setMinimumWidth(MIN_ROW_WIDTH);
        radio->setFixedHeight(ENTRY_ROW_HEIGHT);
        layout->addWidget(radio);
        buttonGroup->addButton(radio);
    }

    radioGroups[groupName] = buttonGroup;
    return layout;
}

QHBoxLayout* EnergyControlWnd::createCycleDirectionStatusRow()
{
    QHBoxLayout *layout = new QHBoxLayout();
    layout->setSpacing(0);

    // Calculate target width
    const int totalTargetWidth = 2 * (ENTRY_LABEL_WIDTH + ENTRY_EDIT_WIDTH + ENTRY_UNIT_WIDTH);
    const int totalElements = 4 + 3; // 4 labels + 3 arrows
    const int elementMinWidth = totalTargetWidth / totalElements;

    for (int i = 1; i <= 4; ++i) {
        QString labelKey = QString("Step %1").arg(i);
        QLabel *label = new QLabel("?", this);
        label->setFixedHeight(ENTRY_ROW_HEIGHT);
        label->setMinimumWidth(elementMinWidth);
        label->setAlignment(Qt::AlignCenter);
        label->setFrameShape(QFrame::StyledPanel);
        chdischCycleDirectionLabels[labelKey] = label;
        layout->addWidget(label);

        if (i != 4) {
            QLabel *arrow = new QLabel("→", this);
            arrow->setFixedHeight(ENTRY_ROW_HEIGHT);
            arrow->setMinimumWidth(elementMinWidth);
            arrow->setAlignment(Qt::AlignCenter);
            layout->addWidget(arrow);
        }
    }

    layout->addStretch(); // Optional: push content to left
    return layout;
}


void EnergyControlWnd::setTabControlsEnabled(QWidget *tab, bool enabled)
{
    // Recursively find all child widgets
    const QList<QWidget*> children = tab->findChildren<QWidget*>();
    for (QWidget *child : children) {
        child->setEnabled(enabled);
    }
}

void EnergyControlWnd::setTabColor(QTabWidget *tabWidget, QWidget *tab, bool enabled)
{
    int tabIndex = tabWidget->indexOf(tab);
    if (tabIndex == -1)
        return; // Tab not found, do nothing

    QTabBar *tabBar = tabWidget->tabBar();

    if (enabled) {
        tabBar->setTabTextColor(tabIndex, QColor(Qt::blue)); // Set to blue or any color
    } else {
        tabBar->setTabTextColor(tabIndex, tabWidget->palette().color(QPalette::WindowText)); // Reset to default
    }
}

void EnergyControlWnd::chDischStartDischargeRelaxTimeout()
{
    // Read initial time from the "Discharge Relax Time" entry
    if (!chdischTimeEdits.contains("Discharge Relax Time") || !chdischTimeEdits.contains("Discharge Relax Timeout"))
        return;

    // Create timer if not created
    if (!chdischDischargeRelaxTimer) {
        chdischDischargeRelaxTimer = new QTimer(this);
        chdischDischargeRelaxTimer->setInterval(1000); // 1 second
        connect(chdischDischargeRelaxTimer, &QTimer::timeout, this, &EnergyControlWnd::onChDschDischargeRelaxTick);
    }

    chdischDischargeRelaxTimer->start();
}

void EnergyControlWnd::chDischStartChargeRelaxTimeout()
{
    // Read initial time from the "Charge Relax Time" entry
    if (!chdischTimeEdits.contains("Charge Relax Time") || !chdischTimeEdits.contains("Charge Relax Timeout"))
        return;

    // Create timer if not created
    if (!chdischChargeRelaxTimer) {
        chdischChargeRelaxTimer = new QTimer(this);
        chdischChargeRelaxTimer->setInterval(1000); // 1 second
        connect(chdischChargeRelaxTimer, &QTimer::timeout, this, &EnergyControlWnd::onChDschChargeRelaxTick);
    }

    chdischChargeRelaxTimer->start();
}

void EnergyControlWnd::chdischResetCycles()
{
    chdischCurrentStepIndex = 0;
    chdischCycleSteps[0].status= StepUnknown;
    chdischCycleSteps[1].status= StepUnknown;
    chdischCycleSteps[2].status= StepUnknown;
    chdischCycleSteps[3].status= StepUnknown;
    chdischCycleStartStep = chdischCycleSteps[0];
}

void EnergyControlWnd::assignCycleStatusRowDirection(const ChDschCycleStep steps[4])
{
    for (int i = 1; i <= 4; ++i) {
        QString labelKey = QString("Step %1").arg(i);
        if (!chdischCycleDirectionLabels.contains(labelKey))
            continue;

        QLabel *label = chdischCycleDirectionLabels[labelKey];
        ChDschState state = steps[i - 1].state;

        QString text;
        switch (state) {
            case ChDschDischarge:       text = "Discharge"; break;
            case ChDschCharge:          text = "Charge"; break;
            case ChDschDischargeRelax:  text = "DRelax"; break;
            case ChDschChargeRelax:     text = "CRelax"; break;
            default:                    text = "?"; break;
        }

        label->setText(text);

        QPalette palette = label->palette();
        palette.setColor(QPalette::WindowText, Qt::black);
        palette.setColor(QPalette::Window, QColor(192, 192, 192)); // light gray
        label->setAutoFillBackground(true);
        label->setPalette(palette);
    }
}

void EnergyControlWnd::chdischCycleStepDone(ChDschState step)
{
    int stepIndex = chdischStateToIndex(step);
    if (stepIndex == -1)
        return;

    QString labelKey = QString("Step %1").arg(stepIndex);
    if (!chdischCycleDirectionLabels.contains(labelKey))
        return;

    QLabel *label = chdischCycleDirectionLabels[labelKey];

    QPalette palette = label->palette();
    palette.setColor(QPalette::Window, QColor(0, 160, 0));  // Green
    palette.setColor(QPalette::WindowText, Qt::white);      // White text
    label->setAutoFillBackground(true);
    label->setPalette(palette);
}

int EnergyControlWnd::chdischStateToIndex(ChDschState state)
{
    for (int i = 0; i < 4; ++i) {
        if (chdischCycleSteps[i].state == state)
            return i + 1; // "Step 1" to "Step 4"
    }
    return -1;
}

void EnergyControlWnd::chdischCycleStepOngoing(ChDschState step)
{
    int stepIndex = chdischStateToIndex(step);
    if (stepIndex == -1)
        return;

    QString labelKey = QString("Step %1").arg(stepIndex);
    if (!chdischCycleDirectionLabels.contains(labelKey))
        return;

    QLabel *label = chdischCycleDirectionLabels[labelKey];

    QPalette palette = label->palette();
    palette.setColor(QPalette::Window, QColor(255, 215, 0));  // Yellow background (Gold)
    palette.setColor(QPalette::WindowText, Qt::black);        // Black text
    label->setAutoFillBackground(true);
    label->setPalette(palette);
}

void EnergyControlWnd::chdischUpdateCycleStatusSteps(const ChDschCycleStep steps[])
{
    for (int i = 0; i < 4; ++i) {
        chdischCycleSteps[i] = steps[i];

        switch (steps[i].status) {
            case StepDone:
                chdischCycleStepDone(steps[i].state);
                break;
            case StepOngoing:
                chdischCycleStepOngoing(steps[i].state);
                break;
            case StepUnknown:
            default:
                // reset to default gray background and black text
                int stepIndex = chdischStateToIndex(steps[i].state);
                if (stepIndex == -1) continue;

                {
                    QString labelKey = QString("Step %1").arg(stepIndex);
                    if (!chdischCycleDirectionLabels.contains(labelKey)) continue;

                    QLabel *label = chdischCycleDirectionLabels[labelKey];
                    QPalette palette = label->palette();
                    palette.setColor(QPalette::Window, QColor(192, 192, 192));  // Gray
                    palette.setColor(QPalette::WindowText, Qt::black);         // Black
                    label->setAutoFillBackground(true);
                    label->setPalette(palette);
                }
                break;
        }
    }
}

bool EnergyControlWnd::chdischDoneCurrentStep()
{
    QTime now = QTime::currentTime();
    if(chdischCycleSteps[chdischCurrentStepIndex].state == ChDschCharge)
    {
        chdischChargeTimer->stop();
        chdischTimeEdits["Charge Stop Time"]->setEnabled(true);
        chdischTimeEdits["Charge Stop Time"]->setTime(now);
        emit sigChargingCurrentStatusChanged(false);
    }
    if(chdischCycleSteps[chdischCurrentStepIndex].state == ChDschDischarge)
    {
        chdischDischargeTimer->stop();
        chdischTimeEdits["Discharge Stop Time"]->setEnabled(true);
        chdischTimeEdits["Discharge Stop Time"]->setTime(now);
        emit sigLoadCurrentStatusChanged(false);
    }
    if(chdischCycleSteps[chdischCurrentStepIndex].state == ChDschChargeRelax)
    {
        chdischChargeRelaxTimer->stop();
    }
    if(chdischCycleSteps[chdischCurrentStepIndex].state == ChDschDischargeRelax)
    {
        chdischDischargeRelaxTimer->stop();
    }
    chdischCycleSteps[chdischCurrentStepIndex].status = StepDone;
    chdischUpdateCycleStatusSteps(chdischCycleSteps);
    return true;
}

bool EnergyControlWnd::chdischMoveToNextStep()
{
    QTime now = QTime::currentTime();
    if(chdischCycleSteps[chdischCurrentStepIndex].status != StepDone) return false;
    chdischCurrentStepIndex += 1;
    if(chdischCurrentStepIndex == 4)
    {
        chdischCycleCompleted();
        return true;
    }
    else
    {
        if(chdischCycleSteps[chdischCurrentStepIndex].state == ChDschCharge)
        {
            chdischTimeEdits["Charge Start Time"]->setEnabled(true);
            chdischTimeEdits["Charge Start Time"]->setTime(now);
            chdischTimeEdits["Charge Duration"]->setEnabled(true);
            chdischTimeEdits["Charge Duration"]->setTime(QTime(0,0,0));
            if(!ppathStatus)
            {
                onPPathStatusChanged();
            }
            if(loadStatus)
            {
                onLoadStatusChanged();
            }
            if((uVoltage || oVoltage || oCurrent))
            {
                onResetProtection();
            }
            chdischChargeTimer->start();
            emit sigChargingCurrentStatusChanged(true);
        }
        if(chdischCycleSteps[chdischCurrentStepIndex].state == ChDschDischarge)
        {
            chdischTimeEdits["Discharge Start Time"]->setEnabled(true);
            chdischTimeEdits["Discharge Start Time"]->setTime(now);
            chdischTimeEdits["Discharge Duration"]->setEnabled(true);
            chdischTimeEdits["Discharge Duration"]->setTime(QTime(0,0,0));
            chdischDischargeTimer->start();
            if(!loadStatus)
            {
                onLoadStatusChanged();
            }
            if(!ppathStatus)
            {
                onPPathStatusChanged();
            }
            if((uVoltage || oVoltage || oCurrent))
            {
                onResetProtection();
            }
            emit sigLoadCurrentStatusChanged(true);
        }
        if(chdischCycleSteps[chdischCurrentStepIndex].state == ChDschChargeRelax)
        {
            if(loadStatus)
            {
                onLoadStatusChanged();
            }
            if(ppathStatus)
            {
                onPPathStatusChanged();
            }
            chdischTimeEdits["Charge Relax Timeout"]->setEnabled(true);
            chDischStartChargeRelaxTimeout();
        }
        if(chdischCycleSteps[chdischCurrentStepIndex].state == ChDschDischargeRelax)
        {
            if(loadStatus)
            {
                onLoadStatusChanged();
            }
            if(ppathStatus)
            {
                onPPathStatusChanged();
            }
            chdischTimeEdits["Discharge Relax Timeout"]->setEnabled(true);
            chDischStartDischargeRelaxTimeout();
        }
    }
    chdischCycleSteps[chdischCurrentStepIndex].status = StepOngoing;
    chdischUpdateCycleStatusSteps(chdischCycleSteps);
    return true;
}

void EnergyControlWnd::chdischCycleCompleted()
{
    chdischStartStatusSet(false, ChDschUnknown);
    if(chdischCheckBox["Write To File"]->isChecked())
    {
        chdischCheckBox["Write To File"]->setChecked(false);
    }

}
void EnergyControlWnd::underVoltageStatusSet(bool enabled)
{
    QString color = enabled ? "rgba(255, 0, 0, 255)" : "rgba(128, 128, 128, 180)";
    statusIndicators["UnderVoltage"]->setStyleSheet(
        QString("background-color: %1; border-radius: 4px;").arg(color));
    uVoltage = enabled;
    switch(mode){
    case ModeLoad:
        if((uVoltage && loadStartStopStatus))
        {
            QMessageBox *msgBox = new QMessageBox(this);
            msgBox->setIcon(QMessageBox::Warning);
            msgBox->setWindowTitle("Warning");
            emit sigLoadCurrentStatusChanged(false);
            msgBox->setText("Load disabled due to under voltage protection");
            msgBox->setStandardButtons(QMessageBox::Ok);
            msgBox->show();
        }
        break;
    break;
    case ModeChDisch:
        if(chdischStartStopStatus && uVoltage)
        {
            chdischDoneCurrentStep();
            chdischMoveToNextStep();
        }
        break;
    case ModeCharge:
        if((uVoltage && chargingStartStopStatus))
        {
            QMessageBox *msgBox = new QMessageBox(this);
            msgBox->setIcon(QMessageBox::Warning);
            msgBox->setWindowTitle("Warning");
                emit sigChargingCurrentStatusChanged(false);
                msgBox->setText("Charging disabled due to under voltage protection");
            msgBox->setStandardButtons(QMessageBox::Ok);
            msgBox->show();
        }
        break;
    case ModeUknown:
    default:
        //QMessageBox::warning(this, "Warning", "Working mode not supported");
        break;
    }

}

void EnergyControlWnd::overVoltageStatusSet(bool enabled)
{
    QString color = enabled ? "rgba(255, 0, 0, 255)" : "rgba(128, 128, 128, 180)";
    statusIndicators["OverVoltage"]->setStyleSheet(
        QString("background-color: %1; border-radius: 4px;").arg(color));
    oVoltage = enabled;
    if((oVoltage && loadStartStopStatus) || (oVoltage && chargingStartStopStatus))
    {
        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setIcon(QMessageBox::Warning);
        msgBox->setWindowTitle("Warning");
        if(loadStartStopStatus)
        {
            emit sigLoadCurrentStatusChanged(false);
            msgBox->setText("Load disabled due to over voltage protection");
        }
        if(chargingStartStopStatus)
        {
            emit sigChargingCurrentStatusChanged(false);
            msgBox->setText("Charging disabled due to over voltage protection");
        }
        msgBox->setStandardButtons(QMessageBox::Ok);
        msgBox->show();
    }
}

void EnergyControlWnd::overCurrentStatusSet(bool enabled)
{
    QString color = enabled ? "rgba(255, 0, 0, 255)" : "rgba(128, 128, 128, 180)";
    statusIndicators["OverCurrent"]->setStyleSheet(
        QString("background-color: %1; border-radius: 4px;").arg(color));

    oCurrent = enabled;
    if((oCurrent && loadStartStopStatus) || (oCurrent && chargingStartStopStatus))
    {
        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setIcon(QMessageBox::Warning);
        msgBox->setWindowTitle("Warning");
        if(loadStartStopStatus)
        {
            emit sigLoadCurrentStatusChanged(false);
            msgBox->setText("Load disabled due to over current protection");
        }
        if(chargingStartStopStatus)
        {
            emit sigChargingCurrentStatusChanged(false);
            msgBox->setText("Charging disabled due to over current protection");
        }
        msgBox->setStandardButtons(QMessageBox::Ok);
        msgBox->show();
    }
}
void EnergyControlWnd::loadStatusSet(bool enabled)
{
    loadStatus = enabled;
    statusControlButtons["LoadDisable"]->setChecked(enabled);
    statusControlLabels["LoadDisable"]->setText("Load " + QString(enabled ? "Enabled" : "Disabled"));
    if(!loadStatus && loadStartStopStatus)
    {
        emit sigLoadCurrentStatusChanged(false);
    }

    // Red background when disabled
    QString style = enabled ? "" : "background-color: blue;";
    statusControlButtons["LoadDisable"]->setStyleSheet(style);
}

void EnergyControlWnd::pPathStatusSet(bool enabled)
{
    ppathStatus = enabled;
    statusControlButtons["PPathDisable"]->setChecked(enabled);
    statusControlLabels["PPathDisable"]->setText("PPath " + QString(enabled ? "Enabled" : "Disabled"));
    if(!ppathStatus && loadStartStopStatus)
    {
        emit sigLoadCurrentStatusChanged(false);
    }

    QString style = enabled ? "" : "background-color: blue;";
    statusControlButtons["PPathDisable"]->setStyleSheet(style);
}

void EnergyControlWnd::batteryStatusSet(bool enabled)
{
    battStatus = enabled;
    statusControlButtons["BatteryDisable"]->setChecked(enabled);
    statusControlLabels["BatteryDisable"]->setText("Battery " + QString(enabled ? "Enabled" : "Disabled"));
    if(!battStatus && loadStartStopStatus)
    {
        emit sigLoadCurrentStatusChanged(false);
    }

    QString style = enabled ? "" : "background-color: blue;";
    statusControlButtons["BatteryDisable"]->setStyleSheet(style);
}

// Load tab
void EnergyControlWnd::loadCurrentSet(int current)
{
    loadEntryEdits["Current"]->setText(QString::number(current));
    chdischEntryEdits["Discharge Current"]->setText(QString::number(current));
}

bool EnergyControlWnd::loadCurrentStatusSet(bool status)
{
    QTime now = QTime::currentTime();
    switch(mode){
    case ModeLoad:
        if((uVoltage || oVoltage || oCurrent) && !loadStartStopStatus)
        {
            QMessageBox::warning(this, "Warning", "Unable to enable load. Please check protections!");
            return false;
        }
        loadStartStopStatus = status;
        loadButtons["Set"]->setEnabled(!loadStartStopStatus);
        if(loadStartStopStatus)
        {
            loadButtons["StartStop"]->setText("Stop");
            loadTimeEdits["Discharge Start Time"]->setEnabled(true);
            loadTimeEdits["Discharge Start Time"]->setTime(now);
            loadTimeEdits["Discharge Stop Time"]->setTime(QTime(0,0,0));
            loadTimeEdits["Discharge Stop Time"]->setEnabled(false);
            loadEntryEdits["Current"]->setEnabled(false);
            loadComboBoxEdits["Mode"]->setEnabled(false);

            statusControlButtons["LoadDisable"]->setEnabled(false);
            statusControlButtons["PPathDisable"]->setEnabled(false);
            statusControlButtons["BatteryDisable"]->setEnabled(false);

            loadTimeEdits["Discharge Duration"]->setEnabled(true);
            loadTimeEdits["Discharge Duration"]->setTime(QTime(0,0,0));
            dischargeTimer->start();

            setTabColor(tabWidget, loadTab, true);
            setTabControlsEnabled(chargerTab, false);
            setTabControlsEnabled(chdischTab, false);
        }
        else
        {
            loadButtons["StartStop"]->setText("Start");
            loadTimeEdits["Discharge Start Time"]->setEnabled(true);
            loadTimeEdits["Discharge Stop Time"]->setEnabled(true);
            loadTimeEdits["Discharge Stop Time"]->setTime(now);
            loadEntryEdits["Current"]->setEnabled(true);
            loadComboBoxEdits["Mode"]->setEnabled(true);

            statusControlButtons["LoadDisable"]->setEnabled(true);
            statusControlButtons["PPathDisable"]->setEnabled(true);
            statusControlButtons["BatteryDisable"]->setEnabled(true);


            setTabColor(tabWidget, loadTab, false);
            setTabControlsEnabled(chargerTab, true);
            setTabControlsEnabled(chdischTab, true);

            dischargeTimer->stop();
        }
        return true;
    break;
    case ModeChDisch:
        //chdischStartStatusSet(true, ChDschDischarge);
        return true;
    break;
    case ModeUknown:
    case ModeCharge:
    default:
        QMessageBox::warning(this, "Warning", "Working mode not supported");
        return false;
    }

}

// Charger tab
void EnergyControlWnd::chargerCurrentSet(int current)
{
    chargerEntryEdits["Current"]->blockSignals(true);
    chargerEntryEdits["Current"]->setText(QString::number(current));
    chargerEntryEdits["Current"]->blockSignals(false);
}

void EnergyControlWnd::chargerTermVoltageSet(float voltage)
{
    chargerEntryEdits["Term Voltage"]->blockSignals(true);
    chargerEntryEdits["Term Voltage"]->setText(QString::number(voltage, 'f', 2));
    chargerEntryEdits["Term Voltage"]->blockSignals(false);
}

void EnergyControlWnd::chargerTermCurrentSet(int current)
{
    chargerEntryEdits["Term Current"]->blockSignals(true);
    chargerEntryEdits["Term Current"]->setText(QString::number(current));
    chargerEntryEdits["Term Current"]->blockSignals(false);
}

bool EnergyControlWnd::chargerCurrentStatusSet(bool status)
{
    QTime now = QTime::currentTime();
    switch(mode){
    case ModeCharge:
        if((uVoltage || oVoltage || oCurrent) && !chargingStartStopStatus)
        {
            QMessageBox::warning(this, "Warning", "Unable to enable charging. Please check protections!");
            return false;
        }
        chargingStartStopStatus = status;
        chargerButtons["Set"]->setEnabled(!chargingStartStopStatus);
        if(chargingStartStopStatus)
        {
            chargerButtons["StartStop"]->setText("Stop");
            chargerTimeEdits["Charging Start Time"]->setEnabled(true);
            chargerTimeEdits["Charging Start Time"]->setTime(now);
            chargerTimeEdits["Charging Stop Time"]->setTime(QTime(0,0,0));
            chargerTimeEdits["Charging Stop Time"]->setEnabled(false);
            chargerEntryEdits["Current"]->setEnabled(false);
            chargerEntryEdits["Term Voltage"]->setEnabled(false);
            chargerEntryEdits["Term Current"]->setEnabled(false);

            statusControlButtons["LoadDisable"]->setEnabled(false);
            statusControlButtons["PPathDisable"]->setEnabled(false);
            statusControlButtons["BatteryDisable"]->setEnabled(false);

            chargerTimeEdits["Charging Duration"]->setEnabled(true);
            chargerTimeEdits["Charging Duration"]->setTime(QTime(0,0,0));
            chargeTimer->start();

            setTabControlsEnabled(loadTab, false);
            setTabControlsEnabled(chdischTab, false);
        }
        else
        {
            chargerButtons["StartStop"]->setText("Start");
            chargerTimeEdits["Charging Start Time"]->setEnabled(true);
            chargerTimeEdits["Charging Stop Time"]->setEnabled(true);
            chargerTimeEdits["Charging Stop Time"]->setTime(now);
            chargerEntryEdits["Current"]->setEnabled(true);
            chargerEntryEdits["Term Voltage"]->setEnabled(true);
            chargerEntryEdits["Term Current"]->setEnabled(true);

            statusControlButtons["LoadDisable"]->setEnabled(true);
            statusControlButtons["PPathDisable"]->setEnabled(true);
            statusControlButtons["BatteryDisable"]->setEnabled(true);

            chargeTimer->stop();

            setTabControlsEnabled(loadTab, true);
            setTabControlsEnabled(chdischTab, true);
        }
        return true;
    break;
    case ModeChDisch:
        //chdischStartStatusSet(true, ChDschCharge);
        return true;
    break;
    case ModeUknown:
    case ModeLoad:
    default:
        QMessageBox::warning(this, "Warning", "Working mode not supported");
        return false;
    }

}

void EnergyControlWnd::chargingDone()
{
    QMessageBox *msgBox = new QMessageBox(this);
    switch(mode){
    case ModeCharge:
        msgBox->setIcon(QMessageBox::Warning);
        msgBox->setWindowTitle("Warning");
        if(chargingStartStopStatus)
        {
            emit sigChargingCurrentStatusChanged(false);
            msgBox->setText("Charging completed");
        }
        msgBox->setStandardButtons(QMessageBox::Ok);
        msgBox->show();
        chargerCurrentStatusSet(false);
        break;
    break;
    case ModeChDisch:
        if(chdischStartStopStatus)
        {
            chdischDoneCurrentStep();
            chdischMoveToNextStep();
        }
        break;
    break;
    case ModeUknown:
    case ModeLoad:
    default:
        QMessageBox::warning(this, "Warning", "Working mode not supported");
        break;
    }

}

// Charge/Discharge tab
void EnergyControlWnd::chdischDischargeCurrentSet(int current)
{
    chdischEntryEdits["Discharge Current"]->setText(QString::number(current));
}

void EnergyControlWnd::chdischChargeCurrentSet(int current)
{
    chdischEntryEdits["Charge Current"]->setText(QString::number(current));
}
void EnergyControlWnd::chdischDischargeRelaxTimeSet(const QTime &time)
{
    chdischTimeEdits["Discharge Relax Time"]->setTime(time);
}
void EnergyControlWnd::chdischChargeRelaxTimeSet(const QTime &time)
{
    chdischTimeEdits["Charge Relax Time"]->setTime(time);
}

bool EnergyControlWnd::chdischStartStatusSet(bool status, ChDschState state)
{
    if((!((state == ChDschCharge) || (state == ChDschDischarge))) && status)
    {
        QMessageBox::warning(this, "Warning", "Unable to set requested starting mode");
        return false;
    }
    if((chdischCycleStartStep.state != state) && (status))
    {
        QMessageBox::warning(this, "Warning", "Obtained state diferent from starting state");
        return false;
    }
    if((uVoltage || oVoltage || oCurrent) && !chdischStartStopStatus)
    {
        QMessageBox::warning(this, "Warning", "Unable to enable Chr/Dsch process. Please check protections!");
        return false;
    }
    if((chdischCurrentStepIndex != 0 && status == true))
    {
        chdischResetCycles();
    }
    if(!((chdischCycleStartStep.state == ChDschCharge) || (chdischCycleStartStep.state == ChDschDischarge)))
    {
        QMessageBox::warning(this, "Warning", "Select Cycle Direction!");
        return false;
    }
    QTime now = QTime::currentTime();
    chdischStartStopStatus = status;
    if(chdischStartStopStatus)
    {
        chdischButtons["StartStop"]->setText("Stop");
        if(chdischCycleStartStep.state == ChDschCharge)
        {
            chdischTimeEdits["Charge Start Time"]->setEnabled(true);
            chdischTimeEdits["Charge Start Time"]->setTime(now);
            chdischTimeEdits["Discharge Start Time"]->setEnabled(false);
            chdischTimeEdits["Discharge Start Time"]->setTime(QTime(0,0,0));
            chdischTimeEdits["Charge Duration"]->setEnabled(true);
            chdischTimeEdits["Charge Duration"]->setTime(QTime(0,0,0));
            chdischTimeEdits["Discharge Duration"]->setEnabled(false);
            chdischTimeEdits["Discharge Duration"]->setTime(QTime(0,0,0));
            if(loadStatus)
            {
                onLoadStatusChanged();
            }
            if(!ppathStatus)
            {
                onPPathStatusChanged();
            }
            emit sigChargingCurrentStatusChanged(true);
            chdischChargeTimer->start();
        }
        if(chdischCycleStartStep.state == ChDschDischarge)
        {
            chdischTimeEdits["Discharge Start Time"]->setEnabled(true);
            chdischTimeEdits["Discharge Start Time"]->setTime(now);
            chdischTimeEdits["Charge Start Time"]->setEnabled(false);
            chdischTimeEdits["Charge Start Time"]->setTime(QTime(0,0,0));
            chdischTimeEdits["Discharge Duration"]->setEnabled(true);
            chdischTimeEdits["Discharge Duration"]->setTime(QTime(0,0,0));
            chdischTimeEdits["Charge Duration"]->setEnabled(false);
            chdischTimeEdits["Charge Duration"]->setTime(QTime(0,0,0));
            if(!ppathStatus)
            {
                onPPathStatusChanged();
            }
            if(!loadStatus)
            {
                onLoadStatusChanged();
                //This is required because of hardware glitch
                onResetProtection();
            }

            emit sigLoadCurrentStatusChanged(true);
            chdischDischargeTimer->start();
        }
        chdischTimeEdits["Charge Stop Time"]->setTime(QTime(0,0,0));
        chdischTimeEdits["Charge Stop Time"]->setEnabled(false);
        chdischTimeEdits["Discharge Stop Time"]->setTime(QTime(0,0,0));
        chdischTimeEdits["Discharge Stop Time"]->setEnabled(false);

        chdischTimeEdits["Discharge Relax Time"]->setEnabled(false);
        chdischTimeEdits["Charge Relax Time"]->setEnabled(false);
        chdischEntryEdits["Discharge Current"]->setEnabled(false);
        chdischEntryEdits["Charge Current"]->setEnabled(false);
        chdischEntryEdits["Cycles"]->setEnabled(false);

        statusControlButtons["LoadDisable"]->setEnabled(false);
        statusControlButtons["PPathDisable"]->setEnabled(false);
        statusControlButtons["BatteryDisable"]->setEnabled(false);

        chdischDischargeRelaxRemaining = chdischTimeEdits["Discharge Relax Time"]->time();

        chdischTimeEdits["Discharge Relax Timeout"]->setTime(chdischDischargeRelaxRemaining);
        chdischTimeEdits["Discharge Relax Timeout"]->setEnabled(false);

        chdischChargeRelaxRemaining = chdischTimeEdits["Charge Relax Time"]->time();

        chdischTimeEdits["Charge Relax Timeout"]->setTime(chdischChargeRelaxRemaining);
        chdischTimeEdits["Charge Relax Timeout"]->setEnabled(false);

        chdischButtons["NextStep"]->setEnabled(true);
        setTabControlsEnabled(loadTab, false);
        setTabControlsEnabled(chargerTab, false);

        chdischCycleSteps[chdischCurrentStepIndex].status = StepOngoing;
        chdischUpdateCycleStatusSteps(chdischCycleSteps);
    }
    else
    {
        chdischButtons["StartStop"]->setText("Start");
        if(chdischCycleSteps[chdischCurrentStepIndex].state == ChDschCharge)
        {
            chdischChargeTimer->stop();
            chdischTimeEdits["Charge Stop Time"]->setEnabled(true);
            chdischTimeEdits["Charge Stop Time"]->setTime(now);
            emit sigChargingCurrentStatusChanged(false);
        }
        if(chdischCycleSteps[chdischCurrentStepIndex].state == ChDschDischarge)
        {
            chdischDischargeTimer->stop();
            chdischTimeEdits["Discharge Stop Time"]->setEnabled(true);
            chdischTimeEdits["Discharge Stop Time"]->setTime(now);
            emit sigLoadCurrentStatusChanged(false);
        }
        if(chdischCycleStartStep.state == ChDschChargeRelax)
        {
            chdischChargeRelaxTimer->stop();
        }
        if(chdischCycleStartStep.state == ChDschDischargeRelax)
        {
            chdischDischargeRelaxTimer->stop();
        }
        chdischTimeEdits["Discharge Relax Time"]->setEnabled(true);
        chdischTimeEdits["Charge Relax Time"]->setEnabled(true);
        chdischEntryEdits["Discharge Current"]->setEnabled(true);
        chdischEntryEdits["Charge Current"]->setEnabled(true);
        chdischEntryEdits["Cycles"]->setEnabled(true);

        statusControlButtons["LoadDisable"]->setEnabled(true);
        statusControlButtons["PPathDisable"]->setEnabled(true);
        statusControlButtons["BatteryDisable"]->setEnabled(true);

        chdischButtons["NextStep"]->setEnabled(false);

        setTabControlsEnabled(loadTab, true);
        setTabControlsEnabled(chargerTab, true);
        if(chdischCurrentStepIndex < 4)
        {
            chdischCycleSteps[chdischCurrentStepIndex].status = StepDone;
            chdischUpdateCycleStatusSteps(chdischCycleSteps);
        }

    }
    return true;
}

void EnergyControlWnd::chdischDischargeRelaxDone()
{
    chdischDoneCurrentStep();
    chdischMoveToNextStep();
}

void EnergyControlWnd::chdischChargeRelaxDone()
{
    chdischDoneCurrentStep();
    chdischMoveToNextStep();
}

void EnergyControlWnd::setChargingState(ChargingState state)
{
    QString text, tooltip, color;

    switch (state) {
        case Discharge:
            text = "Charging state: Discharge";
            tooltip = "Battery discharging process is ongoing";
            color = "#D8BFD8"; // light purple
            break;
        case Charge:
            text = "Charging state: Charge";
            tooltip = "Battery is charging";
            color = "#90EE90"; // light green
            break;
        case Relax:
            text = "Charging state: Relax";
            tooltip = "Battery relaxing";
            color = "#FFFFE0"; // light yellow
            break;
        case Unknown:
        default:
            text = "Charging state: Unknown";
            tooltip = "Start the measurement";
            color = "#FFB6C1"; // light red (pinkish)
            break;
    }

    chargingStatus->setText(text);
    chargingStatus->setToolTip(tooltip);
    chargingStatus->setStyleSheet(QString("background-color: %1; padding: 2px;").arg(color));
}
void EnergyControlWnd::setConfigurationState(ConfigurationState state)
{
    QString text, tooltip, color;

    switch (state) {
        case Ongoing:
            text = "Configuration: Ongoing";
            tooltip = "Configuration is sent to device but not confirmed";
            color = "#FFFFE0"; // light yellow
            break;
        case Configured:
        default:
            text = "Configuration: Configured";
            tooltip = "Configuration is sent to device";
            color = "#90EE90"; // light green
            break;
    }

    configurationStatus->setText(text);
    configurationStatus->setToolTip(tooltip);
    configurationStatus->setStyleSheet(QString("background-color: %1; padding: 2px;").arg(color));
}
EnergyControlWnd::~EnergyControlWnd()
{
    delete ui;
}

void EnergyControlWnd::onLoadModeChanged(const QString &mode)
{
    bool isStatic = (mode == "Static");

    if (loadCurrentWidget)
        loadCurrentWidget->setVisible(isStatic);

    if (dynamicWidget)
        dynamicWidget->setVisible(!isStatic);
}

void EnergyControlWnd::onLoadStatusChanged()
{
    statusControlButtons["LoadDisable"]->setChecked(!statusControlButtons["LoadDisable"]->isChecked());

    emit sigLoadStatusChanged(!loadStatus);
}

void EnergyControlWnd::onPPathStatusChanged()
{
    statusControlButtons["PPathDisable"]->setChecked(!statusControlButtons["PPathDisable"]->isChecked());

    emit sigPPathStatusChanged(!ppathStatus);
}

void EnergyControlWnd::onBatteryStatusChanged()
{
    statusControlButtons["BatteryDisable"]->setChecked(!statusControlButtons["BatteryDisable"]->isChecked());

    emit sigBatteryStatusChanged(!battStatus);
}

void EnergyControlWnd::onLoadSet()
{
    staticLoadCurrent = loadEntryEdits["Current"]->text().toInt();
    chdischDischargeCurrentSet(staticLoadCurrent);
    emit sigLoadCurrentChanged(staticLoadCurrent);

}

void EnergyControlWnd::onLoadStartStop()
{
    if((uVoltage || oVoltage || oCurrent))
    {
        QMessageBox::warning(this, "Warning", "Unable to enable load. Please check protections!");
        return;
    }
    if(!loadStatus || !ppathStatus || !battStatus)
    {
        QMessageBox::warning(this, "Warning", "Load, PPath or Batt are disabled!");
        return;
    }
    mode = ModeLoad;
    emit sigLoadCurrentStatusChanged(!loadStartStopStatus);
}

void EnergyControlWnd::onDischargeTimerTimeout()
{
    QTime startTime = loadTimeEdits["Discharge Start Time"]->time();
    QTime now = QTime::currentTime();

    int secondsElapsed = startTime.secsTo(now);
    if (secondsElapsed < 0)
        secondsElapsed = 0;

    QTime duration(0, 0, 0);
    duration = duration.addSecs(secondsElapsed);

    loadTimeEdits["Discharge Duration"]->setTime(duration);
}

void EnergyControlWnd::onChargerEntryChanged(const QString &)
{
    QLineEdit* senderEdit = qobject_cast<QLineEdit*>(sender());
    if (!senderEdit)
        return;

    for (const auto& key : chargerEntryEdits.keys()) {
        if (chargerEntryEdits[key] == senderEdit) {
            // Mark as changed
            chargerEntryChanged[key] = true;
            // Set text color to red
            QPalette palette = senderEdit->palette();
            palette.setColor(QPalette::Text, Qt::red);
            senderEdit->setPalette(palette);
            break;
        }
    }
}

void EnergyControlWnd::onChargingConfigSet()
{
    for (const auto& key : chargerEntryEdits.keys()) {
        if (chargerEntryChanged[key]) {
            QString text = chargerEntryEdits[key]->text();

            if (key == "Current") {
                bool ok = false;
                staticChargingCurrent = text.toInt(&ok);
                if (ok) {
                    chdischChargeCurrentSet(staticChargingCurrent);
                    emit sigChargingCurrentChanged(staticChargingCurrent);
                }
            }
            else if (key == "Term Voltage") {
                bool ok = false;
                staticChargingTermVoltage = text.toFloat(&ok);
                if (ok) {
                    emit sigChargingTermVoltageChanged(staticChargingTermVoltage);
                }
            }
            else if (key == "Term Current") {
                bool ok = false;
                staticChargingTermCurrent = text.toInt(&ok);
                if (ok) {
                    emit sigChargingTermCurrentChanged(staticChargingTermCurrent);
                }
            }
            // After sending the signal, reset color and flag
            QPalette palette = chargerEntryEdits[key]->palette();
            palette.setColor(QPalette::Text, Qt::black);
            chargerEntryEdits[key]->setPalette(palette);

            chargerEntryChanged[key] = false;
        }
    }
}

void EnergyControlWnd::onChargingStartStop()
{
    if((uVoltage || oVoltage || oCurrent))
    {
        QMessageBox::warning(this, "Warning", "Unable to enable load. Please check protections!");
        return;
    }
    if(loadStatus)
    {
        QMessageBox::warning(this, "Warning", "Disable load");
        return;
    }
    mode = ModeCharge;
    emit sigChargingCurrentStatusChanged(!chargingStartStopStatus);
}

void EnergyControlWnd::onChDschDischargeRelaxTick()
{
    if (chdischDischargeRelaxRemaining == QTime(0, 0, 0)) {
        chdischDischargeRelaxTimer->stop();
        chdischDischargeRelaxDone();
        return;
    }
    chdischDischargeRelaxRemaining = chdischDischargeRelaxRemaining.addSecs(-1);
    chdischTimeEdits["Discharge Relax Timeout"]->setTime(chdischDischargeRelaxRemaining);
}

void EnergyControlWnd::onChDschChargeRelaxTick()
{
    if (chdischChargeRelaxRemaining == QTime(0, 0, 0)) {
        chdischChargeRelaxTimer->stop();
        chdischChargeRelaxDone();
        return;
    }

    chdischChargeRelaxRemaining = chdischChargeRelaxRemaining.addSecs(-1);
    chdischTimeEdits["Charge Relax Timeout"]->setTime(chdischChargeRelaxRemaining);
}

void EnergyControlWnd::onChDschChargingTimerTimeout()
{
    QTime startTime = chdischTimeEdits["Charge Start Time"]->time();
    QTime now = QTime::currentTime();

    int secondsElapsed = startTime.secsTo(now);
    if (secondsElapsed < 0)
        secondsElapsed = 0;

    QTime duration(0, 0, 0);
    duration = duration.addSecs(secondsElapsed);

    chdischTimeEdits["Charge Duration"]->setTime(duration);
}

void EnergyControlWnd::onChDschDischargeTimerTimeout()
{
    QTime startTime = chdischTimeEdits["Discharge Start Time"]->time();
    QTime now = QTime::currentTime();

    int secondsElapsed = startTime.secsTo(now);
    if (secondsElapsed < 0)
        secondsElapsed = 0;

    QTime duration(0, 0, 0);
    duration = duration.addSecs(secondsElapsed);

    chdischTimeEdits["Discharge Duration"]->setTime(duration);
}

void EnergyControlWnd::onChdischDirectionChanged()
{
    QButtonGroup *group = chdischDirectionGroups.value("Start with", nullptr);
    if (!group)
        return;

    QAbstractButton *selected = group->checkedButton();
    if (!selected)
        return;

    QString selectedText = selected->text();
    if(selectedText == "Charge")
    {
        chdischCycleSteps[0].state = ChDschCharge;
        chdischCycleSteps[0].status= StepUnknown;
        chdischCycleSteps[1].state = ChDschChargeRelax;
        chdischCycleSteps[1].status= StepUnknown;
        chdischCycleSteps[2].state = ChDschDischarge;
        chdischCycleSteps[2].status= StepUnknown;
        chdischCycleSteps[3].state = ChDschDischargeRelax;
        chdischCycleSteps[3].status= StepUnknown;
    }
    if(selectedText == "Discharge")
    {
        chdischCycleSteps[0].state = ChDschDischarge;
        chdischCycleSteps[0].status= StepUnknown;
        chdischCycleSteps[1].state = ChDschDischargeRelax;
        chdischCycleSteps[1].status= StepUnknown;
        chdischCycleSteps[2].state = ChDschCharge;
        chdischCycleSteps[2].status= StepUnknown;
        chdischCycleSteps[3].state = ChDschChargeRelax;
        chdischCycleSteps[3].status= StepUnknown;
    }
    assignCycleStatusRowDirection(chdischCycleSteps);
    chdischCycleStartStep = chdischCycleSteps[0];
}

void EnergyControlWnd::onChDschConfigSet()
{

}

void EnergyControlWnd::onChDschNextStep()
{
    chdischDoneCurrentStep();
    chdischMoveToNextStep();
}

void EnergyControlWnd::onChDschStartStop()
{
    mode = ModeChDisch;
    chdischStartStatusSet(!chdischStartStopStatus, chdischCycleStartStep.state);
}

void EnergyControlWnd::onChDschWriteToFileToogled()
{
    bool checked = chdischCheckBox["Write To File"]->isChecked();
    for (QCheckBox *checkBox : chdischWriteToFileCheckBox) {
        if (checkBox)
        {
            checkBox->setVisible(checked);
            checkBox->setEnabled(checked);
        }
    }
    sigChDschWriteToFileToogled(checked);
}

void EnergyControlWnd::onChargingTimerTimeout()
{
    QTime startTime = chargerTimeEdits["Charging Start Time"]->time();
    QTime now = QTime::currentTime();

    int secondsElapsed = startTime.secsTo(now);
    if (secondsElapsed < 0)
        secondsElapsed = 0;

    QTime duration(0, 0, 0);
    duration = duration.addSecs(secondsElapsed);

    chargerTimeEdits["Charging Duration"]->setTime(duration);
}

void EnergyControlWnd::onResetProtection()
{
    emit sigResetProtection();
}
