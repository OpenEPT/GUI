#include "batteryparamssettingsdlg.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QTabWidget>
#include <QVBoxLayout>

#define BATTERYPARAMSSETTINGS_EDIT_WIDTH    110
#define BATTERYPARAMSSETTINGS_ROW_HEIGHT    28

BatteryParamsSettingsDlg::BatteryParamsSettingsDlg(QWidget *parent) :
    QDialog(parent)
{
    QFont defaultFont("Arial", 10);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QGroupBox *batteryGroup = new QGroupBox("Battery", this);
    QGroupBox *relaxationGroup = new QGroupBox("Relaxation", this);
    QGroupBox *extractionGroup = new QGroupBox("Extraction", this);
    QGridLayout *batteryLayout = new QGridLayout(batteryGroup);
    QGridLayout *relaxationLayout = new QGridLayout(relaxationGroup);
    QGridLayout *extractionLayout = new QGridLayout(extractionGroup);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    setFont(defaultFont);
    setWindowTitle("Battery parameters settings");

    capacityEdit = createEntry(batteryLayout, 0, "Battery capacity", "mAh",
                               "Nominal capacity, used to convert the extracted charge into state of charge");
    initialSocEdit = createEntry(batteryLayout, 1, "Initial SoC", "%",
                                 "State of charge the battery starts the measurement with");

    relaxationThresholdEdit = createEntry(relaxationLayout, 0, "Relaxation threshold", "mV",
                                          "Battery counts as relaxed once the voltage stays inside this band for the whole window");
    relaxationWindowEdit = createEntry(relaxationLayout, 1, "Relaxation window", "s",
                                       "Time the voltage has to stay inside the band");

    QLabel *modelLabel = new QLabel("Model", extractionGroup);
    modelCombo = new QComboBox(extractionGroup);
    modelCombo->addItem("First order (R1 C1)");
    modelCombo->addItem("Second order (R1 C1 R2 C2)");
    modelCombo->setFixedHeight(BATTERYPARAMSSETTINGS_ROW_HEIGHT);
    modelCombo->setToolTip("Equivalent circuit fitted to the relaxation curve");
    extractionLayout->addWidget(modelLabel, 0, 0);
    extractionLayout->addWidget(modelCombo, 0, 1, 1, 2);

    QLabel *fitEndLabel = new QLabel("Fit interval", extractionGroup);
    fitEndCombo = new QComboBox(extractionGroup);
    fitEndCombo->addItem("Pause start to relaxation point");
    fitEndCombo->addItem("Pause start to pause end");
    fitEndCombo->setFixedHeight(BATTERYPARAMSSETTINGS_ROW_HEIGHT);
    fitEndCombo->setToolTip("Part of the pause the RC model is fitted over.\n"
                            "Up to the relaxation point leaves out the tail that is still moving,\n"
                            "up to the end of the pause uses every sample");
    extractionLayout->addWidget(fitEndLabel, 1, 0);
    extractionLayout->addWidget(fitEndCombo, 1, 1, 1, 2);

    tauGridPointsEdit = createEntry(extractionLayout, 2, "Tau search points", "",
                                    "Number of candidate time constants tried per decade of the search range.\nMore points give a finer fit and take longer");
    tauRefinementEdit = createEntry(extractionLayout, 3, "Tau refinement steps", "",
                                    "How many times the search range is narrowed around the best result");
    tauSeparationEdit = createEntry(extractionLayout, 4, "Tau separation", "x",
                                    "Smallest allowed ratio between the slow and the fast time constant.\nBranches closer than this describe the same effect");

    QTabWidget *tabWidget = new QTabWidget(this);
    QWidget *analysisTab = new QWidget(this);
    QVBoxLayout *analysisLayout = new QVBoxLayout(analysisTab);
    QWidget *plotTab = new QWidget(this);
    QGridLayout *plotLayout = new QGridLayout(plotTab);

    analysisLayout->addWidget(batteryGroup);
    analysisLayout->addWidget(relaxationGroup);
    analysisLayout->addWidget(extractionGroup);
    analysisLayout->addStretch();
    tabWidget->addTab(analysisTab, "Analysis");

    plotFontCombo = new QFontComboBox(plotTab);
    plotFontCombo->setCurrentFont(QFont(BATTERYPARAMSPLOT_FONT_DEFAULT));
    plotFontCombo->setFixedHeight(BATTERYPARAMSSETTINGS_ROW_HEIGHT);
    plotFontCombo->setToolTip("Font used for the axis labels, the ticks and the legend of the analysis plots");
    plotLayout->addWidget(new QLabel("Label font", plotTab), 0, 0);
    plotLayout->addWidget(plotFontCombo, 0, 1, 1, 2);

    plotLegendFontCombo = new QFontComboBox(plotTab);
    plotLegendFontCombo->setFixedHeight(BATTERYPARAMSSETTINGS_ROW_HEIGHT);
    plotLegendFontCombo->setToolTip("Font used for the legend entries");

    plotLegendPositionCombo = new QComboBox(plotTab);
    plotLegendPositionCombo->addItem("Left");
    plotLegendPositionCombo->addItem("Right");
    plotLegendPositionCombo->setFixedHeight(BATTERYPARAMSSETTINGS_ROW_HEIGHT);
    plotLegendPositionCombo->setToolTip("Corner the legend sits in");

    plotLabelSizeEdit = createEntry(plotLayout, 1, "Label font size", "pt", "Size of the axis label text");
    plotTickSizeEdit = createEntry(plotLayout, 2, "Tick font size", "pt", "Size of the numbers along the axes");
    plotLegendSizeEdit = createEntry(plotLayout, 3, "Legend font size", "pt", "Size of the legend text");
    plotLineWidthEdit = createEntry(plotLayout, 4, "Line width", "px", "Thickness of the drawn curves");
    plotMarkerSizeEdit = createEntry(plotLayout, 5, "Marker size", "px", "Size of the point markers");

    plotLayout->addWidget(new QLabel("Legend font", plotTab), 6, 0);
    plotLayout->addWidget(plotLegendFontCombo, 6, 1, 1, 2);
    plotLayout->addWidget(new QLabel("Legend position", plotTab), 7, 0);
    plotLayout->addWidget(plotLegendPositionCombo, 7, 1, 1, 2);

    plotGridCheckBox = new QCheckBox("Show grid", plotTab);
    plotMinorGridCheckBox = new QCheckBox("Show minor grid", plotTab);
    plotLegendCheckBox = new QCheckBox("Show legend", plotTab);
    plotLayout->addWidget(plotGridCheckBox, 8, 0, 1, 3);
    plotLayout->addWidget(plotMinorGridCheckBox, 9, 0, 1, 3);
    plotLayout->addWidget(plotLegendCheckBox, 10, 0, 1, 3);
    plotLayout->setRowStretch(11, 1);

    tabWidget->addTab(plotTab, "Plots");

    mainLayout->addWidget(tabWidget);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &BatteryParamsSettingsDlg::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &BatteryParamsSettingsDlg::reject);

    setSettings(BatteryParamsExtraction::settingsDefault());
    setPlotSettings(BATTERYPARAMSPLOT_SettingsDefault());
}

QLineEdit* BatteryParamsSettingsDlg::createEntry(QGridLayout *layout, int row, QString name, QString unit, QString tooltip)
{
    QLabel *label = new QLabel(name, this);
    QLineEdit *edit = new QLineEdit(this);
    QLabel *unitLabel = new QLabel(unit.isEmpty() ? "" : "[" + unit + "]", this);

    edit->setFixedSize(BATTERYPARAMSSETTINGS_EDIT_WIDTH, BATTERYPARAMSSETTINGS_ROW_HEIGHT);
    edit->setToolTip(tooltip);
    label->setToolTip(tooltip);

    layout->addWidget(label, row, 0);
    layout->addWidget(edit, row, 1);
    layout->addWidget(unitLabel, row, 2);

    return edit;
}

void BatteryParamsSettingsDlg::setSettings(batteryparams_settings_t settings)
{
    capacityEdit->setText(QString::number(settings.capacity));
    initialSocEdit->setText(QString::number(settings.initialSoc));
    relaxationThresholdEdit->setText(QString::number(settings.relaxationThreshold));
    relaxationWindowEdit->setText(QString::number(settings.relaxationWindow));
    modelCombo->setCurrentIndex(settings.model == BATTERYPARAMS_MODEL_FIRST_ORDER ? 0 : 1);
    fitEndCombo->setCurrentIndex(settings.fitEnd == BATTERYPARAMS_FIT_END_RELAXATION ? 0 : 1);
    tauGridPointsEdit->setText(QString::number(settings.tauGridPoints));
    tauRefinementEdit->setText(QString::number(settings.tauRefinementNo));
    tauSeparationEdit->setText(QString::number(settings.tauSeparation));
}

void BatteryParamsSettingsDlg::setPlotSettings(batteryparams_plot_settings_t settings)
{
    plotFontCombo->setCurrentFont(QFont(settings.fontFamily));
    plotLabelSizeEdit->setText(QString::number(settings.labelFontSize));
    plotTickSizeEdit->setText(QString::number(settings.tickFontSize));
    plotLegendSizeEdit->setText(QString::number(settings.legendFontSize));
    plotLineWidthEdit->setText(QString::number(settings.lineWidth));
    plotMarkerSizeEdit->setText(QString::number(settings.markerSize));
    plotLegendFontCombo->setCurrentFont(QFont(settings.legendFontFamily));
    plotLegendPositionCombo->setCurrentIndex(settings.legendPosition == BATTERYPARAMSPLOT_LEGEND_LEFT ? 0 : 1);
    plotGridCheckBox->setChecked(settings.gridVisible);
    plotMinorGridCheckBox->setChecked(settings.minorGridVisible);
    plotLegendCheckBox->setChecked(settings.legendVisible);
}

batteryparams_plot_settings_t BatteryParamsSettingsDlg::getPlotSettings()
{
    batteryparams_plot_settings_t settings = BATTERYPARAMSPLOT_SettingsDefault();

    settings.fontFamily = plotFontCombo->currentFont().family();

    if(plotLabelSizeEdit->text().toInt() > 0) settings.labelFontSize = plotLabelSizeEdit->text().toInt();
    if(plotTickSizeEdit->text().toInt() > 0) settings.tickFontSize = plotTickSizeEdit->text().toInt();
    if(plotLegendSizeEdit->text().toInt() > 0) settings.legendFontSize = plotLegendSizeEdit->text().toInt();
    if(plotLineWidthEdit->text().toInt() > 0) settings.lineWidth = plotLineWidthEdit->text().toInt();
    if(plotMarkerSizeEdit->text().toInt() > 0) settings.markerSize = plotMarkerSizeEdit->text().toInt();

    settings.legendFontFamily = plotLegendFontCombo->currentFont().family();
    settings.legendPosition = (plotLegendPositionCombo->currentIndex() == 0) ?
                              BATTERYPARAMSPLOT_LEGEND_LEFT : BATTERYPARAMSPLOT_LEGEND_RIGHT;
    settings.gridVisible = plotGridCheckBox->isChecked();
    settings.minorGridVisible = plotMinorGridCheckBox->isChecked();
    settings.legendVisible = plotLegendCheckBox->isChecked();

    return settings;
}

batteryparams_settings_t BatteryParamsSettingsDlg::getSettings()
{
    batteryparams_settings_t settings = BatteryParamsExtraction::settingsDefault();

    if(capacityEdit->text().toDouble() > 0) settings.capacity = capacityEdit->text().toDouble();
    if(initialSocEdit->text().toDouble() > 0) settings.initialSoc = initialSocEdit->text().toDouble();
    if(relaxationThresholdEdit->text().toDouble() > 0) settings.relaxationThreshold = relaxationThresholdEdit->text().toDouble();
    if(relaxationWindowEdit->text().toDouble() > 0) settings.relaxationWindow = relaxationWindowEdit->text().toDouble();
    if(tauGridPointsEdit->text().toInt() > 1) settings.tauGridPoints = tauGridPointsEdit->text().toInt();
    if(tauRefinementEdit->text().toInt() > 0) settings.tauRefinementNo = tauRefinementEdit->text().toInt();
    if(tauSeparationEdit->text().toDouble() > 1) settings.tauSeparation = tauSeparationEdit->text().toDouble();

    settings.model = (modelCombo->currentIndex() == 0) ? BATTERYPARAMS_MODEL_FIRST_ORDER : BATTERYPARAMS_MODEL_SECOND_ORDER;
    settings.fitEnd = (fitEndCombo->currentIndex() == 0) ? BATTERYPARAMS_FIT_END_RELAXATION : BATTERYPARAMS_FIT_END_PAUSE;

    return settings;
}
