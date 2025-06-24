#ifndef ENERGYCONTROLWND_H
#define ENERGYCONTROLWND_H

#include <QWidget>
#include <QTabWidget>
#include <QString>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QTimeEdit>
#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>
#include <QTimer>
#include <QMessageBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QCheckBox>


#define ENTRY_LABEL_WIDTH 180
#define ENTRY_EDIT_WIDTH  120
#define ENTRY_UNIT_WIDTH  60
#define ENTRY_ROW_HEIGHT  28
#define BUTTON_WIDTH  100
#define INDICATOR_WIDTH 30
#define BUTTON_HEIGHT 28


enum Mode {
    ModeUknown,
    ModeLoad,
    ModeCharge,
    ModeChDisch
};


enum ChargingState {
    Discharge,
    Charge,
    Relax,
    Unknown
};

enum ConfigurationState {
    Ongoing,
    Configured
};

enum ChDschState {
    ChDschUnknown = 0,
    ChDschDischarge,
    ChDschCharge,
    ChDschDischargeRelax,
    ChDschChargeRelax
};

enum StepStatus {
    StepUnknown = 0,
    StepOngoing,
    StepDone
};

struct ChDschCycleStep {
    ChDschState state;
    StepStatus status;
};

namespace Ui {
class EnergyControlWnd;
}

class EnergyControlWnd : public QWidget
{
    Q_OBJECT

public:
    explicit EnergyControlWnd(QWidget *parent = nullptr);

    Mode getMode();

    void underVoltageStatusSet(bool enabled);
    void overVoltageStatusSet(bool enabled);
    void overCurrentStatusSet(bool enabled);


    void loadStatusSet(bool enabled);
    void pPathStatusSet(bool enabled);
    void batteryStatusSet(bool enabled);

    // Load tab
    void loadCurrentSet(int current);
    bool loadCurrentStatusSet(bool status);

    // Charger tab
    void chargerCurrentSet(int current);
    void chargerTermVoltageSet(float voltage);
    void chargerTermCurrentSet(int current);
    bool chargerCurrentStatusSet(bool status);
    void chargingDone();

    // Charge/Discharge tab
    void chdischDischargeCurrentSet(int current);
    void chdischChargeCurrentSet(int current);
    void chdischDischargeRelaxTimeSet(const QTime &time);
    void chdischChargeRelaxTimeSet(const QTime &time);
    void chdischDischargeRelaxDone();
    void chdischChargeRelaxDone();
    bool chdischStartStatusSet(bool status, ChDschState state);

    void setChargingState(ChargingState state);
    void setConfigurationState(ConfigurationState state);



    ~EnergyControlWnd();

signals:
    void sigLoadStatusChanged(bool status);
    void sigPPathStatusChanged(bool status);
    void sigBatteryStatusChanged(bool status);

    void sigLoadCurrentChanged(unsigned int current);
    void sigLoadCurrentStatusChanged(bool start);

    void sigChargingCurrentChanged(unsigned int newCurrent);
    void sigChargingTermVoltageChanged(float newVoltage);
    void sigChargingTermCurrentChanged(unsigned int newTermCurrent);
    void sigChargingCurrentStatusChanged(bool start);

    void sigChDschWriteToFileToogled(bool state);

    void sigResetProtection();

private slots:
    void onLoadModeChanged(const QString &mode);

    void onLoadStatusChanged();
    void onPPathStatusChanged();
    void onBatteryStatusChanged();

    void onLoadSet();
    void onLoadStartStop();
    void onDischargeTimerTimeout();

    void onChargerEntryChanged(const QString &);
    void onChargingConfigSet();
    void onChargingTimerTimeout();
    void onChargingStartStop();

    void onChDschDischargeRelaxTick();
    void onChDschChargeRelaxTick();
    void onChDschChargingTimerTimeout();
    void onChDschDischargeTimerTimeout();
    void onChdischDirectionChanged();
    void onChDschConfigSet();
    void onChDschNextStep();
    void onChDschStartStop();
    void onChDschWriteToFileToogled();


    void onResetProtection();

private:
    Ui::EnergyControlWnd *ui;

    Mode                        mode;

    QMap<QString, QLineEdit*> loadEntryEdits;
    QMap<QString, QTimeEdit*> loadTimeEdits;
    QMap<QString, QPushButton*> loadButtons;
    QMap<QString, QComboBox*> loadComboBoxEdits;
    QMap<QString, QTextEdit*> loadTextEdits;

    QMap<QString, QLineEdit*> chargerEntryEdits;
    QMap<QString, QTimeEdit*> chargerTimeEdits;
    QMap<QString, QPushButton*> chargerButtons;
    QMap<QString, bool> chargerEntryChanged;

    QMap<QString, QLineEdit*> chdischEntryEdits;
    QMap<QString, QTimeEdit*> chdischTimeEdits;
    QMap<QString, QPushButton*> chdischButtons;
    QMap<QString, QButtonGroup*> chdischDirectionGroups;
    QMap<QString, QCheckBox*> chdischCheckBox;
    QMap<QString, QCheckBox*> chdischWriteToFileCheckBox;

    QMap<QString, QLabel*>      statusIndicators;
    QMap<QString, QPushButton*> statusControlButtons;
    QMap<QString, QLabel*>      statusControlLabels;


    QMap<QString, QPushButton*> protectionButtons;

    QLabel *configurationStatus;
    QLabel *chargingStatus;

    QWidget *loadCurrentWidget;             // for Static mode
    QWidget *dynamicWidget;  // Wrapper for QTextEdit
    QTextEdit *loadCurrentProfileText;

    bool loadStatus;
    bool ppathStatus;
    bool battStatus;

    bool uVoltage;
    bool oVoltage;
    bool oCurrent;

    QTabWidget *tabWidget;

    unsigned int staticLoadCurrent;
    bool         loadStartStopStatus;
    QTimer*      dischargeTimer;
    QWidget*     loadTab;


    unsigned int staticChargingCurrent;
    unsigned int staticChargingTermCurrent;
    float        staticChargingTermVoltage;
    bool         chargingStartStopStatus;
    QTimer*      chargeTimer;
    QWidget*     chargerTab;

    QTimer      *chdischChargeRelaxTimer = nullptr;
    QTime        chdischChargeRelaxRemaining;
    QTimer      *chdischDischargeRelaxTimer = nullptr;
    QTime        chdischDischargeRelaxRemaining;
    unsigned int chdischCurrentStepIndex;
    bool         chdischStartStopStatus;
    ChDschCycleStep chdischCycleStartStep;
    ChDschCycleStep chdischCycleSteps[4];
    QMap<QString, QLabel*> chdischCycleDirectionLabels;
    QTimer*      chdischChargeTimer;
    QTimer*      chdischDischargeTimer;
    QWidget*     chdischTab;

    QHBoxLayout* createEntryRow(const QString& entryName, const QString& unit, QMap<QString, QLineEdit*>& entryMap);
    QHBoxLayout* createTimeEntryRow(const QString &entryName,QMap<QString, QTimeEdit*> &entryMap, bool enabled=true, bool readOnly=false);
    QHBoxLayout* createButtonRow(const QString& entryName, const QString& displayName, QMap<QString, QPushButton*>& buttonMap);
    QHBoxLayout* createSectionHeader(const QString &title);
    QHBoxLayout* createIndicationStatusRow(const QString &entryName,const QString &statusText,QMap<QString, QLabel*> &statusMap);
    QHBoxLayout* createDropBoxEntry(const QString& entryName, const QStringList& values, QMap<QString, QComboBox*>& comboMap);
    QVBoxLayout* createQTextEditRow(const QString &entryName, QMap<QString, QTextEdit*> &entryMap);
    QHBoxLayout* createControlRow(const QString& entryName, const QString& labelPrefix, QMap<QString, QPushButton*>& buttonMap);
    QVBoxLayout* createRadioButtonRow(const QString &groupName,int count,const QStringList &names, QMap<QString, QButtonGroup*> &radioGroups);
    QHBoxLayout* createCheckButtonRow(const QString& entryName, QMap<QString, QCheckBox*>& checkBoxMap);
    QHBoxLayout* createCheckBoxesRow(int count, const QStringList& checkBoxNames, QMap<QString, QCheckBox*>& checkBoxMap);
    QHBoxLayout* createCycleDirectionStatusRow();
    void         setTabControlsEnabled(QWidget *tab, bool enabled);
    void         setTabColor(QTabWidget *tabWidget, QWidget *tab, bool enabled);
    void         chDischStartDischargeRelaxTimeout();
    void         chDischStartChargeRelaxTimeout();
    void         chdischResetCycles();
    void         assignCycleStatusRowDirection(const ChDschCycleStep steps[4]);
    void         chdischCycleStepDone(ChDschState step);
    int          chdischStateToIndex(ChDschState step);
    void         chdischCycleStepOngoing(ChDschState step);
    void         chdischUpdateCycleStatusSteps(const ChDschCycleStep steps[4]);
    bool         chdischDoneCurrentStep();
    bool         chdischMoveToNextStep();
    void         chdischCycleCompleted();

};

#endif // ENERGYCONTROLWND_H
