#ifndef BATTERYPARAMSSETTINGSDLG_H
#define BATTERYPARAMSSETTINGSDLG_H

#include <QCheckBox>
#include <QComboBox>
#include <QFontComboBox>
#include <QDialog>
#include <QLineEdit>

#include <QGridLayout>

#include "Processing/batteryparamsextraction.h"
#include "batteryparamsplotsettings.h"

class BatteryParamsSettingsDlg : public QDialog
{
    Q_OBJECT

public:
    explicit                    BatteryParamsSettingsDlg(QWidget *parent = nullptr);

    void                        setSettings(batteryparams_settings_t settings);
    batteryparams_settings_t    getSettings();

    void                        setPlotSettings(batteryparams_plot_settings_t settings);
    batteryparams_plot_settings_t getPlotSettings();

private:
    QLineEdit                  *capacityEdit;
    QLineEdit                  *initialSocEdit;
    QLineEdit                  *relaxationThresholdEdit;
    QLineEdit                  *relaxationWindowEdit;
    QComboBox                  *modelCombo;
    QComboBox                  *fitEndCombo;
    QLineEdit                  *tauGridPointsEdit;
    QLineEdit                  *tauRefinementEdit;
    QLineEdit                  *tauSeparationEdit;

    QFontComboBox              *plotFontCombo;
    QLineEdit                  *plotLabelSizeEdit;
    QLineEdit                  *plotTickSizeEdit;
    QLineEdit                  *plotLegendSizeEdit;
    QLineEdit                  *plotLineWidthEdit;
    QLineEdit                  *plotMarkerSizeEdit;
    QFontComboBox              *plotLegendFontCombo;
    QComboBox                  *plotLegendPositionCombo;
    QCheckBox                  *plotGridCheckBox;
    QCheckBox                  *plotMinorGridCheckBox;
    QCheckBox                  *plotLegendCheckBox;

    QLineEdit*                  createEntry(QGridLayout *layout, int row, QString name, QString unit, QString tooltip);
};

#endif // BATTERYPARAMSSETTINGSDLG_H
