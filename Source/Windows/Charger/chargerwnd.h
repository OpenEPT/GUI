#ifndef CHARGERWND_H
#define CHARGERWND_H

#include <QWidget>
#include <QByteArray>
#include <QVector>

class Device;
class Plot;
class QLineEdit;
class QComboBox;
class QLabel;
class QPushButton;
class QTextEdit;

/**
 * @brief Dedicated settings window for a Charger device (BQ25180).
 *
 * Unlike EPP devices (which open the full DeviceWnd), a Charger device opens
 * only this focused window: charging parameters, protection thresholds
 * (BUVLO/VLOWV), battery-chemistry presets, start/stop control, live charging
 * mode (pushed by the firmware) and device identification. It talks to the
 * charger through the Device charger wrappers, which use the serial ControlLink.
 */
class ChargerWnd : public QWidget
{
    Q_OBJECT
public:
    explicit ChargerWnd(Device* device, QWidget *parent = nullptr);

private slots:
    void    onReadClicked();
    void    onSetClicked();
    void    onStoreClicked();
    void    onStartChargingClicked();
    void    onStopChargingClicked();
    void    onApplyChemistryClicked();
    void    onChargerModeObtained(QString mode);
    void    onChargerStatusObtained(int stat0);
    void    onChargerVoltageObtained(double voltageV, quint16 rawAdc);
    void    onStartAcquisitionClicked();
    void    onPauseAcquisitionClicked();
    void    onStopAcquisitionClicked();
    void    onCalibrateClicked();
    void    onEepromReadClicked();
    void    onEepromWriteClicked();
    void    onEepromFormatClicked();
    void    onEepromExportClicked();
    void    onEepromImportClicked();

private:
    void    buildUi();
    int     comboIndexForData(QComboBox* combo, int value);
    void    updateChargingModeLabel(const QString& modeCode);
    void    updateStatusFromStat0(int stat0);
    bool    setRegisterField(int reg, int mask, int shift, int value);
    bool    readRegisterField(int reg, int mask, int shift, int* value);
    void    readStatus();
    void    renderEeprom(const QByteArray& data);

    Device*         m_device;

    /* Live input-voltage acquisition (left side) */
    Plot*           voltageChart;
    QPushButton*    acqStartPusb;       /* play  */
    QPushButton*    acqPausePusb;       /* pause */
    QPushButton*    acqStopPusb;        /* stop (pause + clear) */
    double          m_acqTimeMs;        /* running x-axis time for the plot */

    /* Auto-calibration: displayed voltage = rawAdc * m_calGain [V/count].
     * Calibrated from a known reference applied to the input. */
    QLineEdit*      calVoltageEdit;     /* known reference voltage [V] */
    QPushButton*    calibratePusb;
    QLabel*         calValueLabel;      /* live calibrated reading */
    double          m_calGain;          /* volts per ADC count */
    quint16         m_lastRaw;          /* most recent raw sample */

    /* Identification */
    QLineEdit*      hwSerialEdit;
    QLineEdit*      fwVersionEdit;

    /* Charging parameters */
    QLineEdit*      termVoltageEdit;
    QComboBox*      termCurrentCombo;
    QLineEdit*      chargeCurrentEdit;
    QComboBox*      maxCurrentCombo;

    /* Protection thresholds */
    QComboBox*      buvloCombo;         /* Battery UVLO falling threshold (CHARGECTRL1[5:3]) */
    QComboBox*      vlowvCombo;         /* Precharge voltage threshold   (IC_CTRL[6])        */

    /* Battery chemistry presets */
    QComboBox*      chemistryCombo;
    QPushButton*    applyChemistryPusb;

    /* Status */
    QLabel*         chargingModeLabel;
    QLabel*         stat0Label;
    QLabel*         statusLabel;

    QPushButton*    readPusb;
    QPushButton*    setPusb;
    QPushButton*    storePusb;
    QPushButton*    startPusb;
    QPushButton*    stopPusb;

    /* EEPROM (AT24CS01) memory view */
    QTextEdit*      eepromEdit;
    QPushButton*    eepromReadPusb;
    QPushButton*    eepromWritePusb;
    QPushButton*    eepromFormatPusb;
    QPushButton*    eepromExportPusb;
    QPushButton*    eepromImportPusb;
    QByteArray      m_eepromData;       /* current buffer (from Read or Import) */
    QByteArray      m_prevEepromData;   /* for changed-byte highlighting        */
};

#endif // CHARGERWND_H
