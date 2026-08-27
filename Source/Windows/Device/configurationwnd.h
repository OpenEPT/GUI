#ifndef CONFIGURATIONWND_H
#define CONFIGURATIONWND_H

#include <QWidget>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QTextEdit>
#include <QFont>
#include <QProgressBar>

#include "Processing/Parameters/parameterstore.h"
#include "Processing/Parameters/deviceparamdefs.h"

class QLineEdit;
class QLabel;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QGroupBox;
class QTabWidget;
class QLayout;
class QComboBox;

namespace Ui {
class ConfigurationWnd;
}

class ConfigurationWnd : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigurationWnd(QWidget *parent = nullptr,
                              ParameterStore *params = nullptr);
    ~ConfigurationWnd();

    void setParameters(ParameterStore *params);

    void setParamValue(const QString &key, const QString &value);
    void setFieldEditable(const QString &key, bool editable);

    void setConfigurationAcquiredStatus(bool status);
    void setConfigurationAppliedStatus(bool status);
    void setBDContent(const QString &content);
    void setBDProgress(int percent, const QString &text);
    void resetBDProgress();

    void setChargerBDContent(const QString &content);
    void setChargerBDProgress(int percent, const QString &text);
    void resetChargerBDProgress();

signals:
    void sigDeviceConfigSet(QMap<QString, QString> changedFields);
    void sigDeviceConfigStore();
    void sigApplicationConfigSet(QMap<QString, QString> changedFields);
    void sigConfigSet(QMap<QString, QString> changedFields);
    void sigResetDevice();

    void sigDeviceConfigAcquireRequest();

    void sigBDContentGetRequest();
    void sigBDContentSetRequest(QByteArray content);
    void sigBDFormatRequest();

    void sigChargerBDContentGetRequest();
    void sigChargerBDContentSetRequest(QByteArray content);
    void sigChargerBDFormatRequest();
    void sigChargerConfigSet(QMap<QString, QString> changedFields);


private slots:
    void onFieldChanged();
    void onParamChanged(QString key, QString value);
    void onSetConfigClicked();
    void onAcquireConfigClicked();
    void onStoreConfigClicked();
    void onResetDevice();

    void onBDGetClicked();
    void onBDUpdateClicked();
    void onBDFormatClicked();
    void onBDExportClicked();

    void onChargerBDGetClicked();
    void onChargerBDUpdateClicked();
    void onChargerBDFormatClicked();
    void onChargerBDExportClicked();

private:
    Ui::ConfigurationWnd *ui;

    ParameterStore *m_params;

    QTabWidget *tabWidget;

    QLabel *configurationStatusLabel;
    QLabel *changedFieldsLabel;

    QPushButton *setConfigButton;
    QPushButton *storeConfigButton;
    QPushButton *acquireConfigButton;
    QPushButton *resetDeviceButton;

    QMap<QString, QWidget*> fields;
    QMap<QString, QString> appliedValues;
    QMap<QString, QString> displayNames;

    void rebuildUi();
    void clearUiState();

    QWidget *createTab(Params::GroupId group);
    QVBoxLayout *createGroupLayout(Params::GroupId group);
    QGroupBox *createSubGroupBox(Params::GroupId group,
                                 Params::SubGroupId subGroup);

    QGridLayout *createParamsGrid(const QList<Params::Param> &params);
    QWidget *createParamWidget(const Params::Param &param);

    QHBoxLayout *createButtonsRow();
    QVBoxLayout *createStatusBarLayout();

    void registerField(const Params::Param &param, QWidget *field);

    void setFieldValue(const QString &key,
                       const QString &value,
                       bool markAsApplied);

    QString getFieldValue(QWidget *field) const;
    void setFieldWidgetValue(QWidget *field, const QString &value);
    void setFieldWidgetEditable(QWidget *field, bool editable);

    QMap<QString, QString> getChangedFields() const;
    QMap<QString, QString> getChangedFields(Params::GroupId group) const;

    QStringList getChangedFieldDisplayNames() const;

    void refreshStatusBar();

    bool isDeviceParam(const QString &key) const;
    bool isApplicationParam(const QString &key) const;

    QTextEdit *bdContentTextEdit;
    QPushButton *bdGetButton;
    QPushButton *bdUpdateButton;
    QPushButton *bdFormatButton;
    QPushButton *bdExportButton;
    QProgressBar *bdProgressBar;
    QLabel *bdProgressLabel;
    QWidget* createBDMemoryWidget();
    QByteArray m_currentBDData;
    QByteArray m_prevBDData;

    QTextEdit *chargerBDContentTextEdit;
    QPushButton *chargerBDGetButton;
    QPushButton *chargerBDUpdateButton;
    QPushButton *chargerBDFormatButton;
    QPushButton *chargerBDExportButton;
    QProgressBar *chargerBDProgressBar;
    QLabel *chargerBDProgressLabel;

    QWidget *createChargerBDMemoryWidget();

    QByteArray m_currentChargerBDData;
    QByteArray m_prevChargerBDData;


};

#endif // CONFIGURATIONWND_H
