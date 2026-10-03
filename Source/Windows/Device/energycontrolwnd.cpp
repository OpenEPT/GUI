#include "energycontrolwnd.h"
#include <limits.h>
#include "ui_energycontrolwnd.h"

#include <QTabBar>
#include <QPushButton>
#include <QMdiSubWindow>
#include <QMdiArea>
#include <QHeaderView>
#include <QAbstractItemView>

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
    QStringList loadTypes = {"Static", "Standard Wave", "Custom Wave", "Bat Param Extraction"};
    QHBoxLayout *loadModeRow = createDropBoxEntry("Mode", loadTypes, loadComboBoxEdits);
    batParamViewButton = new QPushButton("Battery params", this);
    batParamViewButton->setFixedHeight(BUTTON_HEIGHT);
    batParamViewButton->setMinimumWidth(BUTTON_WIDTH);
    batParamViewButton->setToolTip("Open real time battery parameters view.\nAvailable while the wave is running");
    batParamViewButton->setEnabled(false);
    batParamViewButton->setVisible(false);
    loadButtons.insert("BatParamView", batParamViewButton);
    loadModeRow->addWidget(batParamViewButton);
    loadTopLayout->addLayout(loadModeRow);

    connect(batParamViewButton, &QPushButton::clicked, this, &EnergyControlWnd::sigBatParamViewRequested);
    connect(loadComboBoxEdits["Mode"], &QComboBox::currentTextChanged,this, &EnergyControlWnd::onLoadModeChanged);

    // --- Static mode widget ---
    loadCurrentWidget = new QWidget(this);
    QVBoxLayout *staticLayout = new QVBoxLayout(loadCurrentWidget);
    staticLayout->setContentsMargins(0, 0, 0, 0);
    staticLayout->addLayout(createEntryRow("Current", "mA", loadEntryEdits));
    loadEntryEdits["Current"]->setText(QString::number(staticLoadCurrent));
    staticLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    loadTopLayout->addWidget(loadCurrentWidget);

    // --- Standard wave mode widget ---
    standardWaveWidget = new QWidget(this);
    QVBoxLayout *stdWaveLayout = new QVBoxLayout(standardWaveWidget);
    stdWaveLayout->setContentsMargins(0, 0, 0, 0);
    stdWaveLayout->addLayout(createDropBoxEntry("Wave", Waveform::standardTypeNames(), loadComboBoxEdits));
    loadComboBoxEdits["Wave"]->setToolTip("Ramp: 0->A rise\nSawtooth: A->0 fall\nTriangle: rise then fall\nSquare: A / 0\nSine: 0..A");
    stdWaveLayout->addLayout(createEntryRow("Amplitude", "mA", loadEntryEdits));
    stdWaveLayout->addLayout(createEntryRow("Period", "ms", loadEntryEdits));
    stdWaveLayout->addLayout(createEntryRow("Points", "", loadEntryEdits));
    stdWaveLayout->addLayout(createEntryRow("Repetitions", "-1=inf", loadEntryEdits));
    loadEntryEdits["Amplitude"]->setText("1000");
    loadEntryEdits["Period"]->setText("20");
    loadEntryEdits["Points"]->setText("20");
    loadEntryEdits["Repetitions"]->setText("-1");
    stdWaveInfoLabel = new QLabel(this);
    stdWaveInfoLabel->setStyleSheet("color: gray;");
    QHBoxLayout *stdWaveSaveRow = new QHBoxLayout();
    QPushButton *stdSaveWaveButton = createSmallButton("Save wave", stdWaveSaveRow);
    stdSaveWaveButton->setToolTip("Save wave to library for later use");
    loadButtons.insert("SaveStdWave", stdSaveWaveButton);
    stdWaveSaveRow->addWidget(stdWaveInfoLabel, 1);
    stdWaveLayout->addLayout(stdWaveSaveRow);
    connect(stdSaveWaveButton, &QPushButton::clicked, this, &EnergyControlWnd::onLoadWaveSave);
    stdWaveLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    standardWaveWidget->setVisible(false);
    loadTopLayout->addWidget(standardWaveWidget);

    connect(loadComboBoxEdits["Wave"], &QComboBox::currentTextChanged, this, &EnergyControlWnd::onStdWaveParamChanged);
    connect(loadEntryEdits["Amplitude"], &QLineEdit::textChanged, this, &EnergyControlWnd::onStdWaveParamChanged);
    connect(loadEntryEdits["Period"], &QLineEdit::textChanged, this, &EnergyControlWnd::onStdWaveParamChanged);
    connect(loadEntryEdits["Points"], &QLineEdit::textChanged, this, &EnergyControlWnd::onStdWaveParamChanged);
    connect(loadEntryEdits["Repetitions"], &QLineEdit::textChanged, this, &EnergyControlWnd::onStdWaveParamChanged);

    // --- Custom wave mode widget ---
    customWaveTableUpdating = false;
    epEnabled = false;
    customWaveWidget = new QWidget(this);
    QVBoxLayout *customWaveLayout = new QVBoxLayout(customWaveWidget);
    customWaveLayout->setContentsMargins(0, 0, 0, 0);

    // Library selection + name
    QHBoxLayout *libRow = new QHBoxLayout();
    QLabel *libLabel = new QLabel("Library", this);
    libLabel->setFixedSize(ENTRY_LABEL_WIDTH, ENTRY_ROW_HEIGHT);
    customWaveLibraryCombo = new QComboBox(this);
    customWaveLibraryCombo->setFixedHeight(ENTRY_ROW_HEIGHT);
    customWaveLibraryCombo->setMinimumWidth(ENTRY_EDIT_WIDTH + ENTRY_UNIT_WIDTH);
    libRow->addWidget(libLabel);
    libRow->addWidget(customWaveLibraryCombo, 1);
    customWaveButtons["DeleteFromLibrary"] = createSmallButton("Delete", libRow);
    customWaveLayout->addLayout(libRow);

    QHBoxLayout *nameRow = new QHBoxLayout();
    QLabel *nameLabel = new QLabel("Wave name", this);
    nameLabel->setFixedSize(ENTRY_LABEL_WIDTH, ENTRY_ROW_HEIGHT);
    customWaveNameEdit = new QLineEdit(this);
    customWaveNameEdit->setFixedHeight(ENTRY_ROW_HEIGHT);
    customWaveNameEdit->setPlaceholderText("Custom wave name");
    nameRow->addWidget(nameLabel);
    nameRow->addWidget(customWaveNameEdit, 1);
    customWaveLayout->addLayout(nameRow);

    QHBoxLayout *repetitionsRow = createEntryRow("Wave Repetitions", "-1=inf", loadEntryEdits);
    loadEntryEdits["Wave Repetitions"]->setText("-1");
    customWaveInfoLabel = new QLabel(this);
    customWaveInfoLabel->setStyleSheet("color: gray;");
    repetitionsRow->insertWidget(3, customWaveInfoLabel);
    customWaveLayout->addLayout(repetitionsRow);

    // File / table manipulation buttons
    QHBoxLayout *fileRow = new QHBoxLayout();
    customWaveButtons["LoadFile"]   = createSmallButton("Load file", fileRow);
    customWaveButtons["ExportFile"] = createSmallButton("Export file", fileRow);
    customWaveButtons["SaveWave"]   = createSmallButton("Save wave", fileRow);
    customWaveButtons["SaveWave"]->setToolTip("Save wave to library for later use");
    fileRow->addStretch();
    customWaveButtons["AddRow"]     = createSmallButton("Add chunk", fileRow);
    customWaveButtons["RemoveRow"]  = createSmallButton("Remove chunk", fileRow);
    customWaveButtons["Clear"]      = createSmallButton("Clear", fileRow);
    customWaveLayout->addLayout(fileRow);

    customWaveTable = createWaveTable(true);
    customWaveLayout->addWidget(customWaveTable, 1);

    customWaveWidget->setVisible(false);
    loadTopLayout->addWidget(customWaveWidget);

    connect(customWaveLibraryCombo, QOverload<int>::of(&QComboBox::activated), this, &EnergyControlWnd::onCustomWaveLibrarySelected);
    connect(customWaveButtons["DeleteFromLibrary"], &QPushButton::clicked, this, &EnergyControlWnd::onCustomWaveDeleteFromLibrary);
    connect(customWaveButtons["LoadFile"], &QPushButton::clicked, this, &EnergyControlWnd::onCustomWaveLoadFile);
    connect(customWaveButtons["ExportFile"], &QPushButton::clicked, this, &EnergyControlWnd::onCustomWaveExportFile);
    connect(customWaveButtons["SaveWave"], &QPushButton::clicked, this, &EnergyControlWnd::onLoadWaveSave);
    connect(customWaveButtons["AddRow"], &QPushButton::clicked, this, &EnergyControlWnd::onCustomWaveAddRow);
    connect(customWaveButtons["RemoveRow"], &QPushButton::clicked, this, &EnergyControlWnd::onCustomWaveRemoveRow);
    connect(customWaveButtons["Clear"], &QPushButton::clicked, this, &EnergyControlWnd::onCustomWaveClear);
    connect(customWaveTable, &QTableWidget::itemChanged, this, &EnergyControlWnd::onCustomWaveTableChanged);
    connect(&WaveformLibrary::instance(), &WaveformLibrary::sigLibraryChanged, this, &EnergyControlWnd::onWaveLibraryChanged);

    // --- Battery parameter extraction mode widget ---
    batParamRelaxCombo = NULL;
    batParamWidget = new QWidget(this);
    QVBoxLayout *batParamLayout = new QVBoxLayout(batParamWidget);
    batParamLayout->setContentsMargins(0, 0, 0, 0);

    batParamLayout->addLayout(createEntryRow("Battery Capacity", "mAh", loadEntryEdits));
    batParamLayout->addLayout(createEntryRow("Extraction Accuracy", "%", loadEntryEdits));
    batParamLayout->addLayout(createEntryRow("Maximum Current", "mA", loadEntryEdits));
    batParamLayout->addLayout(createEntryRow("Pause Duration", "ms", loadEntryEdits));
    batParamLayout->addLayout(createEntryRow("Marker Guard", "ms", loadEntryEdits));

    QHBoxLayout *batParamRelaxRow = new QHBoxLayout();
    QLabel *batParamRelaxLabel = new QLabel("Relaxation", this);
    batParamRelaxLabel->setFixedSize(ENTRY_LABEL_WIDTH, ENTRY_ROW_HEIGHT);
    batParamRelaxCombo = new QComboBox(this);
    batParamRelaxCombo->addItem("Fixed pause");
    batParamRelaxCombo->addItem("Until dV/dt");
    batParamRelaxCombo->setFixedHeight(ENTRY_ROW_HEIGHT);
    batParamRelaxCombo->setToolTip("Fixed pause: every pause lasts exactly the pause duration.\nUntil dV/dt: the pause ends as soon as the voltage settles, pause duration is then the maximum");
    batParamRelaxRow->addWidget(batParamRelaxLabel);
    batParamRelaxRow->addWidget(batParamRelaxCombo);
    batParamRelaxRow->addStretch();
    batParamLayout->addLayout(batParamRelaxRow);

    batParamLayout->addLayout(createEntryRow("Relax Threshold", "mV", loadEntryEdits));
    batParamLayout->addLayout(createEntryRow("Relax Window", "s", loadEntryEdits));
    batParamLayout->addLayout(createEntryRow("Relax Filter", "ms", loadEntryEdits));
    loadEntryEdits["Battery Capacity"]->setText("500");
    loadEntryEdits["Extraction Accuracy"]->setText("1");
    loadEntryEdits["Maximum Current"]->setText("1000");
    loadEntryEdits["Pause Duration"]->setText("1000");
    loadEntryEdits["Marker Guard"]->setText("1");
    loadEntryEdits["Relax Threshold"]->setText("5");
    loadEntryEdits["Relax Window"]->setText("60");
    loadEntryEdits["Relax Filter"]->setText("1000");
    loadEntryEdits["Battery Capacity"]->setToolTip("Nominal battery capacity");
    loadEntryEdits["Extraction Accuracy"]->setToolTip("Charge extracted by a single pulse, in percent of the battery capacity");
    loadEntryEdits["Maximum Current"]->setToolTip("Pulse amplitude, pause is always 0 mA");
    loadEntryEdits["Pause Duration"]->setToolTip("Relaxation time between two pulses");
    loadEntryEdits["Marker Guard"]->setToolTip("Distance between a current step and the marker that follows it.\nKeeps every marker inside a settled level instead of on the step itself.\nMust cover the load response time, a few sample periods at least");
    loadEntryEdits["Relax Threshold"]->setToolTip("Voltage span allowed inside the relaxation window, the pause ends when the battery stays within it");
    loadEntryEdits["Relax Window"]->setToolTip("Time window the voltage has to stay inside the threshold before the pause is ended");
    loadEntryEdits["Relax Filter"]->setToolTip("Voltage is averaged over this window, in milliseconds, before it is compared with the threshold.\nThe averaged voltage is drawn over the live voltage plot.\n0 turns the filter off");

    batParamInfoLabel = new QLabel(this);
    batParamInfoLabel->setStyleSheet("color: gray;");
    QHBoxLayout *batParamSaveRow = new QHBoxLayout();
    QPushButton *batParamSaveWaveButton = createSmallButton("Save wave", batParamSaveRow);
    batParamSaveWaveButton->setToolTip("Save generated wave to library for later use");
    loadButtons.insert("SaveBatParamWave", batParamSaveWaveButton);
    batParamSaveRow->addWidget(batParamInfoLabel, 1);
    batParamLayout->addLayout(batParamSaveRow);
    connect(batParamSaveWaveButton, &QPushButton::clicked, this, &EnergyControlWnd::onLoadWaveSave);

    batParamTable = createWaveTable(false);
    batParamLayout->addWidget(batParamTable, 1);

    batParamWidget->setVisible(false);
    loadTopLayout->addWidget(batParamWidget);

    connect(loadEntryEdits["Battery Capacity"], &QLineEdit::textChanged, this, &EnergyControlWnd::onBatParamChanged);
    connect(loadEntryEdits["Extraction Accuracy"], &QLineEdit::textChanged, this, &EnergyControlWnd::onBatParamChanged);
    connect(loadEntryEdits["Maximum Current"], &QLineEdit::textChanged, this, &EnergyControlWnd::onBatParamChanged);
    connect(loadEntryEdits["Pause Duration"], &QLineEdit::textChanged, this, &EnergyControlWnd::onBatParamChanged);
    connect(loadEntryEdits["Marker Guard"], &QLineEdit::textChanged, this, &EnergyControlWnd::onBatParamChanged);
    connect(loadEntryEdits["Relax Threshold"], &QLineEdit::textChanged, this, &EnergyControlWnd::onBatParamChanged);
    connect(loadEntryEdits["Relax Window"], &QLineEdit::textChanged, this, &EnergyControlWnd::onBatParamChanged);
    connect(loadEntryEdits["Relax Filter"], &QLineEdit::textChanged, this, &EnergyControlWnd::onBatParamChanged);
    connect(batParamRelaxCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onBatParamChanged()));

    refreshCustomWaveLibraryCombo();
    updateStdWaveInfo();
    updateCustomWaveInfo();
    updateBatParamInfo();

    /*IThis should always be visible*/
    loadTopLayout->addLayout(createButtonRow("Set", "Set", loadButtons));
    // Start/Stop row: "Clear wave" on the left (sends "device wave clear"), Start/Stop on the right
    QHBoxLayout *startStopRow = createButtonRow("StartStop", "Start", loadButtons);
    QPushButton *clearWaveButton = new QPushButton("Clear wave", this);
    clearWaveButton->setObjectName("ClearWaveButton");
    clearWaveButton->setFixedSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    clearWaveButton->setToolTip("Remove wave loaded on device");
    loadButtons.insert("ClearWave", clearWaveButton);
    startStopRow->insertWidget(0, clearWaveButton);
    loadTopLayout->addLayout(startStopRow);
    connect(clearWaveButton, &QPushButton::clicked, this, &EnergyControlWnd::onLoadWaveClear);



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

    chargerTopLayout->addLayout(createSectionHeader("Charger Info"));

    chargerTopLayout->addLayout(createEntryRow("Charger FW Version", "", chargerInfoEdits));
    chargerTopLayout->addLayout(createEntryRow("Charger Serial Number", "", chargerInfoEdits));
    chargerTopLayout->addLayout(createEntryRow("Max Charging Current", "mA", chargerInfoEdits));

    chargerInfoEdits["Charger FW Version"]->setReadOnly(true);
    chargerInfoEdits["Charger Serial Number"]->setReadOnly(true);
    chargerInfoEdits["Max Charging Current"]->setReadOnly(true);

    QString infoStyle = "background-color: rgb(245,245,245);";

    chargerInfoEdits["Charger FW Version"]->setStyleSheet(infoStyle);
    chargerInfoEdits["Charger Serial Number"]->setStyleSheet(infoStyle);
    chargerInfoEdits["Max Charging Current"]->setStyleSheet(infoStyle);



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

    QStringList dischargeProfiles = {"Static", "From Library"};
    paramLeftLayout->addLayout(createDropBoxEntry("Discharge Profile", dischargeProfiles, chdischComboBoxEdits));
    connect(chdischComboBoxEdits["Discharge Profile"], &QComboBox::currentTextChanged, this, &EnergyControlWnd::onChDschProfileChanged);

    chdischStaticProfileWidget = new QWidget(this);
    QVBoxLayout *chdischStaticProfileLayout = new QVBoxLayout(chdischStaticProfileWidget);
    chdischStaticProfileLayout->setContentsMargins(0, 0, 0, 0);
    chdischStaticProfileLayout->addLayout(createEntryRow("Discharge Current", "mA", chdischEntryEdits));
    paramLeftLayout->addWidget(chdischStaticProfileWidget);

    chdischLibraryProfileWidget = new QWidget(this);
    QVBoxLayout *chdischLibraryProfileLayout = new QVBoxLayout(chdischLibraryProfileWidget);
    chdischLibraryProfileLayout->setContentsMargins(0, 0, 0, 0);
    chdischLibraryProfileLayout->addLayout(createDropBoxEntry("Discharge Wave", QStringList(), chdischComboBoxEdits));
    chdischComboBoxEdits["Discharge Wave"]->view()->setMinimumWidth(2 * (ENTRY_EDIT_WIDTH + ENTRY_UNIT_WIDTH));
    connect(chdischComboBoxEdits["Discharge Wave"], &QComboBox::currentTextChanged, chdischComboBoxEdits["Discharge Wave"], &QComboBox::setToolTip);
    chdischLibraryProfileWidget->setVisible(false);
    paramLeftLayout->addWidget(chdischLibraryProfileWidget);
    chdischRefreshLibraryCombo();

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
    setChargingState(Unknown1);

    // ----- Add status section to main layout -----
    mainLayout->addLayout(statusSectionLayout);

    chargerConnected = false;
    chargerConnectionStatusSet(false);

    this->setFixedSize(this->sizeHint());
}

Mode EnergyControlWnd::getMode()
{
    return mode;
}

void EnergyControlWnd::chargerConnectionStatusSet(bool connected)
{
    chargerConnected = connected;

    int chargerTabIndex = tabWidget->indexOf(chargerTab);
    int chdischTabIndex = tabWidget->indexOf(chdischTab);

    tabWidget->setTabEnabled(chargerTabIndex, connected);
    tabWidget->setTabEnabled(chdischTabIndex, connected);

    tabWidget->tabBar()->setTabTextColor(chargerTabIndex,
                                         connected ? Qt::black : Qt::gray);

    tabWidget->tabBar()->setTabTextColor(chdischTabIndex,
                                         connected ? Qt::black : Qt::gray);

    if(!connected)
    {
        if(chargingStartStopStatus)
        {
            emit sigChargingCurrentStatusChanged(false);
        }

        if(chdischStartStopStatus)
        {
            chdischStartStatusSet(false, ChDschUnknown);
        }
    }
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
        chdischDischargeStop();
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
            chdischDischargeStart();
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
void EnergyControlWnd::loadStopRequest()
{
    /*Static mode drives the load current directly, every other mode runs a wave
      and has to be stopped through the wave state*/
    if(loadModeGet() == LoadModeStatic)
    {
        emit sigLoadCurrentStatusChanged(false);
        return;
    }

    emit sigLoadWaveStatusChanged(false);
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
            loadStopRequest();
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
            loadStopRequest();
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
            loadStopRequest();
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
        loadStopRequest();
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
            standardWaveWidget->setEnabled(false);
            customWaveWidget->setEnabled(false);
            batParamWidget->setEnabled(false);
            batParamViewButton->setEnabled(loadModeGet() == LoadModeBatParamExtraction);

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
            standardWaveWidget->setEnabled(true);
            customWaveWidget->setEnabled(true);
            batParamWidget->setEnabled(true);
            batParamViewButton->setEnabled(false);

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

void EnergyControlWnd::chargerFWVersionSet(const QString& name)
{
    chargerInfoEdits["Charger FW Version"]->setText(name);
}

void EnergyControlWnd::chargerSerialNumberSet(const QString& serial)
{
    chargerInfoEdits["Charger Serial Number"]->setText(serial);
}

void EnergyControlWnd::chargerMaxCurrentSet(int current)
{
    chargerInfoEdits["Max Charging Current"]->setText(QString::number(current));
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

            chdischDischargeStart();
            chdischDischargeTimer->start();
        }
        chdischTimeEdits["Charge Stop Time"]->setTime(QTime(0,0,0));
        chdischTimeEdits["Charge Stop Time"]->setEnabled(false);
        chdischTimeEdits["Discharge Stop Time"]->setTime(QTime(0,0,0));
        chdischTimeEdits["Discharge Stop Time"]->setEnabled(false);

        chdischTimeEdits["Discharge Relax Time"]->setEnabled(false);
        chdischTimeEdits["Charge Relax Time"]->setEnabled(false);
        chdischComboBoxEdits["Discharge Profile"]->setEnabled(false);
        chdischComboBoxEdits["Discharge Wave"]->setEnabled(false);
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
            chdischDischargeStop();
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
        chdischComboBoxEdits["Discharge Profile"]->setEnabled(true);
        chdischComboBoxEdits["Discharge Wave"]->setEnabled(true);
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
        case Unknown1:
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
    loadCurrentWidget->setVisible(mode == "Static");
    standardWaveWidget->setVisible(mode == "Standard Wave");
    customWaveWidget->setVisible(mode == "Custom Wave");
    batParamWidget->setVisible(mode == "Bat Param Extraction");
    batParamViewButton->setVisible(mode == "Bat Param Extraction");
}

LoadMode EnergyControlWnd::loadModeGet()
{
    QString mode = loadComboBoxEdits["Mode"]->currentText();
    if(mode == "Standard Wave") return LoadModeStandardWave;
    if(mode == "Custom Wave") return LoadModeCustomWave;
    if(mode == "Bat Param Extraction") return LoadModeBatParamExtraction;
    return LoadModeStatic;
}

Waveform EnergyControlWnd::loadActiveWaveGet()
{
    return loadActiveWave;
}

/* ---------------------------- Standard wave ---------------------------- */
Waveform EnergyControlWnd::buildStandardWave()
{
    Waveform wave;
    waveform_type_t type = Waveform::typeFromString(loadComboBoxEdits["Wave"]->currentText());
    unsigned int amplitude = loadEntryEdits["Amplitude"]->text().toUInt();
    unsigned int period = loadEntryEdits["Period"]->text().toUInt();
    unsigned int points = loadEntryEdits["Points"]->text().toUInt();
    int repetitions = loadEntryEdits["Repetitions"]->text().toInt();

    wave.generateStandard(type, amplitude, period, points, repetitions);
    return wave;
}

void EnergyControlWnd::updateStdWaveInfo()
{
    Waveform wave = buildStandardWave();
    if(!wave.isValid())
    {
        stdWaveInfoLabel->setText("Invalid parameters (period > 0, points >= 2)");
        return;
    }
    stdWaveInfoLabel->setText(QString::number(wave.chunks.size()) + " chunks, " +
                              QString::number(wave.chunks.first().duration) + " ms/chunk, " +
                              QString::number(wave.getTotalDuration()) + " ms per pass (max " +
                              QString::number(WAVEFORM_CHUNK_MAX_NO) + " chunks)");
}

void EnergyControlWnd::onStdWaveParamChanged()
{
    updateStdWaveInfo();
}

/* ----------------------------- Custom wave ----------------------------- */
QPushButton* EnergyControlWnd::createSmallButton(const QString& text, QHBoxLayout* layout)
{
    QPushButton *button = new QPushButton(text, this);
    button->setFixedHeight(BUTTON_HEIGHT);
    button->setMinimumWidth(BUTTON_WIDTH);
    layout->addWidget(button);
    return button;
}

void EnergyControlWnd::refreshCustomWaveLibraryCombo()
{
    QString current = customWaveLibraryCombo->currentText();
    QStringList userNames = WaveformLibrary::instance().getNames(WAVEFORM_ORIGIN_USER);
    QStringList stdNames = WaveformLibrary::instance().getNames(WAVEFORM_ORIGIN_STANDARD);
    int index;

    customWaveLibraryCombo->blockSignals(true);
    customWaveLibraryCombo->clear();
    customWaveLibraryCombo->addItem("<select from library>");
    for(int i = 0; i < userNames.size(); i++)
    {
        customWaveLibraryCombo->addItem(userNames[i]);
    }
    for(int i = 0; i < stdNames.size(); i++)
    {
        customWaveLibraryCombo->addItem("[std] " + stdNames[i]);
    }
    index = customWaveLibraryCombo->findText(current);
    if(index < 0) index = 0;
    customWaveLibraryCombo->setCurrentIndex(index);
    customWaveLibraryCombo->blockSignals(false);
}

QString EnergyControlWnd::getCustomWaveLibraryName(int index)
{
    QString name = customWaveLibraryCombo->itemText(index);
    if(name.startsWith("[std] "))
    {
        name = name.mid(6);
    }
    return name;
}

void EnergyControlWnd::onWaveLibraryChanged()
{
    refreshCustomWaveLibraryCombo();
    chdischRefreshLibraryCombo();
}

QTableWidgetItem* EnergyControlWnd::createCustomWaveTableItem(QString text)
{
    QTableWidgetItem *item = new QTableWidgetItem(text);
    item->setTextAlignment(Qt::AlignCenter);
    return item;
}

QString EnergyControlWnd::getCustomWaveTableText(int row, int column)
{
    QTableWidgetItem *item = customWaveTable->item(row, column);
    if(item == NULL) return "";
    return item->text();
}

QTableWidget* EnergyControlWnd::createWaveTable(bool editable)
{
    QStringList header;
    QTableWidget *table;

    header << "Value [mA]" << "Dev [mA]" << "Dur [ms]" << "Dev [ms]" << "Rep" << "Last" << "Marker" << "Pos";
    table = new QTableWidget(0, 8, this);
    table->setHorizontalHeaderLabels(header);
    table->horizontalHeaderItem(1)->setToolTip("Value deviation [mA]");
    table->horizontalHeaderItem(2)->setToolTip("Chunk duration [ms]");
    table->horizontalHeaderItem(3)->setToolTip("Duration deviation [ms]");
    table->horizontalHeaderItem(4)->setToolTip("Chunk repetitions");
    table->horizontalHeaderItem(5)->setToolTip("Last chunk in group");
    table->horizontalHeaderItem(6)->setToolTip("Energy debugger marker name (optional)");
    table->horizontalHeaderItem(7)->setToolTip("Marker position: s = chunk start, e = chunk end, s,e = both (marker \"Start name, End name\")");
    /*Sections are stretched to the viewport, so the horizontal scroll bar is never
      needed. It has to stay off: with both scroll bars on demand the scroll area
      oscillates between showing and hiding them and the window locks up*/
    table->horizontalHeader()->setStretchLastSection(false);
    table->horizontalHeader()->setMinimumSectionSize(40);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    table->verticalHeader()->setDefaultSectionSize(22);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setMinimumHeight(100);
    table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    if(!editable)
    {
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }

    return table;
}

void EnergyControlWnd::addWaveTableRow(QTableWidget *table, waveform_chunk_t chunk, int row)
{
    QTableWidgetItem *lastInGroupItem;

    if(row < 0)
    {
        row = table->rowCount();
    }
    table->insertRow(row);
    table->setItem(row, 0, createCustomWaveTableItem(QString::number(chunk.value)));
    table->setItem(row, 1, createCustomWaveTableItem(QString::number(chunk.valueDev)));
    table->setItem(row, 2, createCustomWaveTableItem(QString::number(chunk.duration)));
    table->setItem(row, 3, createCustomWaveTableItem(QString::number(chunk.durationDev)));
    table->setItem(row, 4, createCustomWaveTableItem(QString::number(chunk.repetitions)));

    lastInGroupItem = new QTableWidgetItem();
    lastInGroupItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    lastInGroupItem->setCheckState(chunk.lastInGroup ? Qt::Checked : Qt::Unchecked);
    lastInGroupItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 5, lastInGroupItem);
    table->setItem(row, 6, createCustomWaveTableItem(chunk.marker));
    table->setItem(row, 7, createCustomWaveTableItem(Waveform::chunkHasMarker(chunk) ? chunk.markerPos : ""));
}

void EnergyControlWnd::fillWaveTable(QTableWidget *table, Waveform wave)
{
    table->setRowCount(0);
    for(int i = 0; i < wave.chunks.size(); i++)
    {
        addWaveTableRow(table, wave.chunks[i], -1);
    }
}

void EnergyControlWnd::addCustomWaveTableRow(waveform_chunk_t chunk, int row)
{
    addWaveTableRow(customWaveTable, chunk, row);
}

void EnergyControlWnd::fillCustomWaveTable(Waveform wave)
{
    customWaveTableUpdating = true;
    customWaveTable->setRowCount(0);
    for(int i = 0; i < wave.chunks.size(); i++)
    {
        addCustomWaveTableRow(wave.chunks[i], -1);
    }
    customWaveTableUpdating = false;
    customWaveNameEdit->setText(wave.name);
    loadEntryEdits["Wave Repetitions"]->setText(QString::number(wave.repetitionCounter));
    updateCustomWaveInfo();
}

Waveform EnergyControlWnd::buildCustomWave()
{
    Waveform wave;
    QTableWidgetItem *lastInGroupItem;

    wave.name = customWaveNameEdit->text().trimmed();
    wave.type = WAVEFORM_TYPE_CUSTOM;
    wave.origin = WAVEFORM_ORIGIN_USER;
    wave.repetitionCounter = loadEntryEdits["Wave Repetitions"]->text().toInt();
    for(int row = 0; row < customWaveTable->rowCount(); row++)
    {
        waveform_chunk_t chunk = Waveform::chunkDefault();
        chunk.value = getCustomWaveTableText(row, 0).toUInt();
        chunk.valueDev = getCustomWaveTableText(row, 1).toUInt();
        chunk.duration = getCustomWaveTableText(row, 2).toUInt();
        chunk.durationDev = getCustomWaveTableText(row, 3).toUInt();
        chunk.repetitions = getCustomWaveTableText(row, 4).toInt();
        if(chunk.duration < 1) chunk.duration = 1;
        if(chunk.repetitions < 1) chunk.repetitions = 1;
        lastInGroupItem = customWaveTable->item(row, 5);
        chunk.lastInGroup = (lastInGroupItem != NULL) && (lastInGroupItem->checkState() == Qt::Checked);
        chunk.marker = getCustomWaveTableText(row, 6).trimmed();
        chunk.markerPos = getCustomWaveTableText(row, 7);
        if(chunk.marker.isEmpty())
        {
            chunk.markerPos = "";
        }
        else
        {
            chunk.markerPos = Waveform::chunkNormalizeMarkerPos(chunk.markerPos);
        }
        wave.chunks.append(chunk);
    }
    if(!wave.chunks.isEmpty())
    {
        wave.chunks.last().lastInGroup = true;
    }
    return wave;
}

void EnergyControlWnd::updateCustomWaveInfo()
{
    Waveform wave = buildCustomWave();
    customWaveInfoLabel->setText(QString::number(wave.chunks.size()) + " chunks, " +
                                 QString::number(wave.getTotalDuration()) + " ms per pass (max " +
                                 QString::number(WAVEFORM_CHUNK_MAX_NO) + " chunks)");
}

/* -------------------- Battery parameter extraction --------------------- */
bool EnergyControlWnd::batParamCompute(unsigned int *pulseDuration, int *repetitions, QString *error)
{
    bool capacityOk;
    bool accuracyOk;
    bool currentOk;
    double capacity = loadEntryEdits["Battery Capacity"]->text().toDouble(&capacityOk);
    double accuracy = loadEntryEdits["Extraction Accuracy"]->text().toDouble(&accuracyOk);
    double current = loadEntryEdits["Maximum Current"]->text().toDouble(&currentOk);
    unsigned int pause = loadEntryEdits["Pause Duration"]->text().toUInt();
    unsigned int guard = loadEntryEdits["Marker Guard"]->text().toUInt();
    double duration;

    if(!capacityOk || capacity <= 0)
    {
        if(error != NULL) *error = "Battery capacity must be greater than 0 mAh";
        return false;
    }
    if(!accuracyOk || accuracy <= 0 || accuracy > 100)
    {
        if(error != NULL) *error = "Extraction accuracy must be between 0 and 100 %";
        return false;
    }
    if(!currentOk || current <= 0)
    {
        if(error != NULL) *error = "Maximum current must be greater than 0 mA";
        return false;
    }

    /*Charge extracted by a single pulse [mAh] divided by pulse current [mA] gives pulse duration in hours*/
    duration = (capacity * accuracy / 100.0) / current * 3600000.0;

    if(duration < 1.0)
    {
        if(error != NULL) *error = "Calculated pulse is shorter than 1 ms, decrease maximum current or increase accuracy";
        return false;
    }
    if(duration > (double)UINT_MAX)
    {
        if(error != NULL) *error = "Calculated pulse is too long, increase maximum current";
        return false;
    }

    if(guard < 1)
    {
        if(error != NULL) *error = "Marker guard must be at least 1 ms";
        return false;
    }
    if(duration <= (double)guard)
    {
        if(error != NULL) *error = "Calculated pulse is shorter than the marker guard, decrease maximum current or increase accuracy";
        return false;
    }
    if(pause <= guard)
    {
        if(error != NULL) *error = "Pause duration must be longer than the marker guard";
        return false;
    }

    if(pulseDuration != NULL) *pulseDuration = (unsigned int)qRound(duration);
    if(repetitions != NULL) *repetitions = qRound(100.0 / accuracy);

    return true;
}

Waveform EnergyControlWnd::buildBatParamWave()
{
    Waveform wave;
    waveform_chunk_t pulseHead = Waveform::chunkDefault();
    waveform_chunk_t pulse = Waveform::chunkDefault();
    waveform_chunk_t pauseHead = Waveform::chunkDefault();
    waveform_chunk_t pause = Waveform::chunkDefault();
    unsigned int pulseDuration;
    unsigned int pauseDuration;
    unsigned int guard;

    if(!batParamCompute(&pulseDuration, NULL, NULL)) return wave;

    pauseDuration = loadEntryEdits["Pause Duration"]->text().toUInt();
    guard = loadEntryEdits["Marker Guard"]->text().toUInt();

    wave.name = "BatParam " + loadEntryEdits["Battery Capacity"]->text() + "mAh " +
                loadEntryEdits["Extraction Accuracy"]->text() + "%";
    wave.type = WAVEFORM_TYPE_CUSTOM;
    wave.origin = WAVEFORM_ORIGIN_USER;

    /*With the adaptive relaxation the device runs one pulse and one pause per pass,
      the application ends the pause as soon as the voltage settles and starts the
      next pass, so the repetitions are counted on the application side*/
    /*Procedure runs until the battery reaches the under voltage protection, the
      number of passes cannot be known in advance because the real capacity differs
      from the one written on the cell*/
    wave.repetitionCounter = batParamRelaxationEnabled() ? 1 : -1;

    /*Every level starts with an unmarked guard chunk, so all four markers land
      inside a settled level instead of on the current step itself. The load
      needs a few samples to reach the new level, and a marker placed on the
      step would still read the previous level*/
    pulseHead.value = loadEntryEdits["Maximum Current"]->text().toUInt();
    pulseHead.duration = guard;
    pulseHead.repetitions = 1;
    pulseHead.lastInGroup = false;

    pulse.value = pulseHead.value;
    pulse.duration = pulseDuration - guard;
    pulse.repetitions = 1;
    pulse.lastInGroup = false;
    pulse.marker = "Pulse Start, Pulse End";
    pulse.markerPos = "s,e";

    pauseHead.value = 0;
    pauseHead.duration = guard;
    pauseHead.repetitions = 1;
    pauseHead.lastInGroup = false;

    pause.value = 0;
    pause.duration = pauseDuration - guard;
    pause.repetitions = 1;
    pause.lastInGroup = true;
    pause.marker = "Pause Start, Pause End";
    pause.markerPos = "s,e";

    wave.chunks.append(pulseHead);
    wave.chunks.append(pulse);
    wave.chunks.append(pauseHead);
    wave.chunks.append(pause);

    return wave;
}

void EnergyControlWnd::updateBatParamInfo()
{
    QString error;
    Waveform wave;
    unsigned int pulseDuration;

    if(!batParamCompute(&pulseDuration, NULL, &error))
    {
        batParamInfoLabel->setText(error);
        batParamTable->setRowCount(0);
        return;
    }

    wave = buildBatParamWave();
    fillWaveTable(batParamTable, wave);

    QString info = QString::number(pulseDuration) + " ms pulse, " +
                   QString::number(wave.getTotalDuration()) + " ms per pass";

    if(batParamRelaxationEnabled())
    {
        info += " at most, pause ends on " + loadEntryEdits["Relax Threshold"]->text() +
                " mV in " + loadEntryEdits["Relax Window"]->text() + " s";
    }

    info += ", runs until under voltage protection";

    batParamInfoLabel->setText(info);
}

bool EnergyControlWnd::batParamRelaxationEnabled()
{
    if(batParamRelaxCombo == NULL) return false;
    return batParamRelaxCombo->currentIndex() == 1;
}

void EnergyControlWnd::onBatParamChanged()
{
    bool adaptive = batParamRelaxationEnabled();

    loadEntryEdits["Relax Threshold"]->setEnabled(adaptive);
    loadEntryEdits["Relax Window"]->setEnabled(adaptive);
    loadEntryEdits["Relax Filter"]->setEnabled(adaptive);

    updateBatParamInfo();
}

void EnergyControlWnd::onCustomWaveTableChanged()
{
    if(customWaveTableUpdating) return;
    updateCustomWaveInfo();
}

void EnergyControlWnd::onCustomWaveLibrarySelected(int index)
{
    Waveform wave;
    if(index <= 0) return;
    if(!WaveformLibrary::instance().get(getCustomWaveLibraryName(index), &wave)) return;
    fillCustomWaveTable(wave);
}

void EnergyControlWnd::onCustomWaveLoadFile()
{
    Waveform wave;
    QString error;
    QString path = QFileDialog::getOpenFileName(this, "Load wave file", QString(), "Wave files (*.txt *.wave);;All files (*)");
    if(path.isEmpty()) return;
    if(!wave.loadFromFile(path, &error))
    {
        QMessageBox::warning(this, "Load wave", error);
        return;
    }
    fillCustomWaveTable(wave);
    customWaveLibraryCombo->setCurrentIndex(0);
}

void EnergyControlWnd::onCustomWaveExportFile()
{
    Waveform wave = buildCustomWave();
    QString error;
    QString suggested;
    QString path;

    if(!wave.isValid())
    {
        QMessageBox::warning(this, "Export wave", "Wave table is empty");
        return;
    }
    suggested = wave.name.isEmpty() ? "wave.txt" : wave.name + ".txt";
    path = QFileDialog::getSaveFileName(this, "Export wave file", suggested, "Wave files (*.txt);;All files (*)");
    if(path.isEmpty()) return;
    if(!wave.saveToFile(path, &error))
    {
        QMessageBox::warning(this, "Export wave", error);
    }
}

void EnergyControlWnd::onCustomWaveAddRow()
{
    waveform_chunk_t chunk = Waveform::chunkDefault();
    int row = customWaveTable->currentRow();

    if(row >= 0)
    {
        /*Copy selected row as template and insert new row after it*/
        Waveform wave = buildCustomWave();
        chunk = wave.chunks[row];
        chunk.lastInGroup = false;
        row += 1;
    }
    addCustomWaveTableRow(chunk, row);
    updateCustomWaveInfo();
}

void EnergyControlWnd::onCustomWaveRemoveRow()
{
    QList<QTableWidgetSelectionRange> ranges = customWaveTable->selectedRanges();
    QList<int> rows;

    for(int i = 0; i < ranges.size(); i++)
    {
        for(int row = ranges[i].topRow(); row <= ranges[i].bottomRow(); row++)
        {
            if(!rows.contains(row))
            {
                rows.append(row);
            }
        }
    }
    if(rows.isEmpty() && customWaveTable->currentRow() >= 0)
    {
        rows.append(customWaveTable->currentRow());
    }

    /*Remove from bottom to top so remaining row indexes stay valid*/
    for(int row = customWaveTable->rowCount() - 1; row >= 0; row--)
    {
        if(rows.contains(row))
        {
            customWaveTable->removeRow(row);
        }
    }
    updateCustomWaveInfo();
}

void EnergyControlWnd::onCustomWaveClear()
{
    customWaveTable->setRowCount(0);
    updateCustomWaveInfo();
}

void EnergyControlWnd::onLoadWaveSave()
{
    Waveform wave;
    QString question;
    int index;

    switch(loadModeGet())
    {
    case LoadModeStandardWave:
        wave = buildStandardWave();
        if(!wave.isValid())
        {
            QMessageBox::warning(this, "Save wave", "Invalid wave parameters (period > 0, points >= 2)");
            return;
        }
        break;
    case LoadModeCustomWave:
        wave = buildCustomWave();
        if(wave.name.isEmpty())
        {
            QMessageBox::warning(this, "Save wave", "Please enter wave name");
            return;
        }
        if(!wave.isValid())
        {
            QMessageBox::warning(this, "Save wave", "Wave table is empty");
            return;
        }
        break;
    case LoadModeBatParamExtraction:
        {
            QString error;
            if(!batParamCompute(NULL, NULL, &error))
            {
                QMessageBox::warning(this, "Save wave", error);
                return;
            }
        }
        wave = buildBatParamWave();
        break;
    case LoadModeStatic:
    default:
        return;
    }

    question = "Wave named \"" + wave.name + "\" will be saved for later use.";
    if(WaveformLibrary::instance().contains(wave.name))
    {
        question += "\nWave with the same name already exists and will be overwritten.";
    }
    question += "\nContinue?";
    if(QMessageBox::question(this, "Save wave", question) != QMessageBox::Yes) return;

    WaveformLibrary::instance().addOrReplace(wave);

    if(loadModeGet() == LoadModeCustomWave)
    {
        index = customWaveLibraryCombo->findText(wave.name);
        if(index >= 0)
        {
            customWaveLibraryCombo->setCurrentIndex(index);
        }
    }
}

void EnergyControlWnd::onCustomWaveDeleteFromLibrary()
{
    int index = customWaveLibraryCombo->currentIndex();
    QString name;

    if(index <= 0) return;
    name = getCustomWaveLibraryName(index);
    if(QMessageBox::question(this, "Delete wave", "Delete \"" + name + "\" from library?") != QMessageBox::Yes) return;
    WaveformLibrary::instance().remove(name);
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
    Waveform wave;

    switch(loadModeGet())
    {
    case LoadModeStatic:
        staticLoadCurrent = loadEntryEdits["Current"]->text().toInt();
        chdischDischargeCurrentSet(staticLoadCurrent);
        emit sigLoadCurrentChanged(staticLoadCurrent);
        break;
    case LoadModeStandardWave:
        wave = buildStandardWave();
        if(!wave.isValid())
        {
            QMessageBox::warning(this, "Warning", "Invalid wave parameters (period > 0, points >= 2)");
            return;
        }
        if(wave.chunks.size() > WAVEFORM_CHUNK_MAX_NO)
        {
            QMessageBox::warning(this, "Warning", "Wave exceeds maximum of " + QString::number(WAVEFORM_CHUNK_MAX_NO) + " chunks");
            return;
        }
        loadActiveWave = wave;
        emit sigLoadWaveChanged(wave);
        break;
    case LoadModeBatParamExtraction:
        {
            QString error;
            if(!batParamCompute(NULL, NULL, &error))
            {
                QMessageBox::warning(this, "Warning", error);
                return;
            }
        }
        wave = buildBatParamWave();
        if(!waveMarkersConfirm(wave)) return;
        loadActiveWave = wave;
        emit sigBatParamCapacityChanged(loadEntryEdits["Battery Capacity"]->text().toDouble());
        emit sigBatParamRelaxationChanged(batParamRelaxationEnabled(),
                                          loadEntryEdits["Relax Threshold"]->text().toDouble(),
                                          loadEntryEdits["Relax Window"]->text().toDouble(),
                                          loadEntryEdits["Relax Filter"]->text().toDouble());
        emit sigLoadWaveChanged(wave);
        break;
    case LoadModeCustomWave:
        wave = buildCustomWave();
        if(!wave.isValid())
        {
            QMessageBox::warning(this, "Warning", "Wave table is empty");
            return;
        }
        for(int i = 0; i < wave.chunks.size(); i++)
        {
            QString markerError;
            if(!Waveform::chunkMarkerValid(wave.chunks[i], &markerError))
            {
                QMessageBox::warning(this, "Warning", "Chunk " + QString::number(i + 1) + ": " + markerError);
                return;
            }
        }
        if(wave.chunks.size() > WAVEFORM_CHUNK_MAX_NO)
        {
            QMessageBox::warning(this, "Warning", "Wave exceeds maximum of " + QString::number(WAVEFORM_CHUNK_MAX_NO) + " chunks");
            return;
        }
        if(wave.name.isEmpty())
        {
            wave.name = "Unnamed custom wave";
        }
        if(!waveMarkersConfirm(wave)) return;
        loadActiveWave = wave;
        emit sigLoadWaveChanged(wave);
        break;
    }
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
    if(loadModeGet() == LoadModeStatic)
    {
        emit sigLoadCurrentStatusChanged(!loadStartStopStatus);
    }
    else
    {
        if(!loadStartStopStatus && !loadActiveWave.isValid())
        {
            QMessageBox::warning(this, "Warning", "Wave is not set. Press Set first!");
            return;
        }
        emit sigLoadWaveStatusChanged(!loadStartStopStatus);
    }
}

/*Device reported that wave finished all repetitions: reset Start/Stop UI*/
void EnergyControlWnd::epEnabledStatusSet(bool enabled)
{
    epEnabled = enabled;
}

bool EnergyControlWnd::waveMarkersConfirm(Waveform wave)
{
    bool hasMarkers = false;

    for(int i = 0; i < wave.chunks.size(); i++)
    {
        if(Waveform::chunkHasMarker(wave.chunks[i]))
        {
            hasMarkers = true;
            break;
        }
    }
    if(!hasMarkers) return true;
    if(epEnabled) return true;

    return QMessageBox::warning(this, "Warning",
                                "Wave \"" + wave.name + "\" contains energy point markers, but \"Enable EP\" is not checked on device window.\n"
                                "Markers will not be received and shown on plots.\n\nContinue anyway?",
                                QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes;
}

bool EnergyControlWnd::loadWaveStopped()
{
    switch(mode)
    {
    case ModeLoad:
        if(!loadStartStopStatus) return true;
        if(loadModeGet() == LoadModeStatic) return true;
        return loadCurrentStatusSet(false);
    case ModeChDisch:
        if(!chdischStartStopStatus) return true;
        if(chdischDischargeProfileGet() == LoadModeStatic) return true;
        if(chdischCycleSteps[chdischCurrentStepIndex].state != ChDschDischarge) return true;
        onChDschNextStep();
        return true;
    default:
        return true;
    }
}

void EnergyControlWnd::onLoadWaveClear()
{
    if(loadStartStopStatus && loadModeGet() != LoadModeStatic)
    {
        QMessageBox::warning(this, "Warning", "Stop the wave before clearing it");
        return;
    }
    loadActiveWave = Waveform();
    emit sigLoadWaveClear();
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
    if(!chdischStartStopStatus && !chdischDischargeProfileValid()) return;
    mode = ModeChDisch;
    chdischStartStatusSet(!chdischStartStopStatus, chdischCycleStartStep.state);
}

void EnergyControlWnd::onChDschProfileChanged(const QString &profile)
{
    chdischStaticProfileWidget->setVisible(profile == "Static");
    chdischLibraryProfileWidget->setVisible(profile == "From Library");
}

LoadMode EnergyControlWnd::chdischDischargeProfileGet()
{
    if(chdischComboBoxEdits["Discharge Profile"]->currentText() == "From Library") return LoadModeCustomWave;
    return LoadModeStatic;
}

void EnergyControlWnd::chdischRefreshLibraryCombo()
{
    QString current = chdischComboBoxEdits["Discharge Wave"]->currentText();
    QStringList names = WaveformLibrary::instance().getNames();
    int index;

    chdischComboBoxEdits["Discharge Wave"]->blockSignals(true);
    chdischComboBoxEdits["Discharge Wave"]->clear();
    for(int i = 0; i < names.size(); i++)
    {
        chdischComboBoxEdits["Discharge Wave"]->addItem(names[i]);
    }
    index = chdischComboBoxEdits["Discharge Wave"]->findText(current);
    if(index < 0) index = 0;
    chdischComboBoxEdits["Discharge Wave"]->setCurrentIndex(index);
    chdischComboBoxEdits["Discharge Wave"]->blockSignals(false);
}

bool EnergyControlWnd::chdischDischargeProfileValid()
{
    Waveform wave;

    if(chdischDischargeProfileGet() == LoadModeStatic) return true;
    if(!WaveformLibrary::instance().get(chdischComboBoxEdits["Discharge Wave"]->currentText(), &wave))
    {
        QMessageBox::warning(this, "Warning", "Please select discharge wave from library");
        return false;
    }
    if(!wave.isValid())
    {
        QMessageBox::warning(this, "Warning", "Selected discharge wave is empty");
        return false;
    }
    return waveMarkersConfirm(wave);
}

void EnergyControlWnd::chdischDischargeStart()
{
    Waveform wave;

    if(chdischDischargeProfileGet() == LoadModeStatic)
    {
        emit sigLoadCurrentChanged(chdischEntryEdits["Discharge Current"]->text().toUInt());
        emit sigLoadCurrentStatusChanged(true);
        return;
    }

    if(!WaveformLibrary::instance().get(chdischComboBoxEdits["Discharge Wave"]->currentText(), &wave)) return;
    chdischActiveWave = wave;
    emit sigLoadWaveChanged(wave);
    emit sigLoadWaveStatusChanged(true);
}

void EnergyControlWnd::chdischDischargeStop()
{
    if(chdischDischargeProfileGet() == LoadModeStatic)
    {
        emit sigLoadCurrentStatusChanged(false);
    }
    else
    {
        emit sigLoadWaveStatusChanged(false);
    }
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
