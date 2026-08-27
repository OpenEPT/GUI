#ifndef APPLICATIONCONFWND_H
#define APPLICATIONCONFWND_H

#include <QWidget>
#include <QMap>
#include <QString>
#include <QStringList>

#include "Processing/Parameters/parameterstore.h"

class QLineEdit;
class QLabel;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QGroupBox;
class QTabWidget;

namespace Ui {
class ApplicationConfWnd;
}

class ApplicationConfWnd : public QWidget
{
    Q_OBJECT

public:
    explicit ApplicationConfWnd(QWidget *parent = nullptr,
                                ParameterStore *params = nullptr);
    ~ApplicationConfWnd();

    void setParameters(ParameterStore *params);

    void setParamValue(const QString &key, const QString &value);
    void setFieldEditable(const QString &key, bool editable);

    void setConfigurationAppliedStatus(bool status);

signals:
    void sigApplicationConfigSet(QMap<QString, QString> changedFields);
    void sigConfigSet(QMap<QString, QString> changedFields);

private slots:
    void onFieldChanged();
    void onParamChanged(QString key, QString value);
    void onApplyConfigClicked();

private:
    Ui::ApplicationConfWnd *ui;

    ParameterStore *m_params;

    QTabWidget *tabWidget;

    QLabel *configurationStatusLabel;
    QLabel *changedFieldsLabel;

    QPushButton *applyConfigButton;

    QMap<QString, QLineEdit*> fields;
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

    void registerField(const Params::Param &param,
                       QLineEdit *field);

    void setFieldValue(const QString &key,
                       const QString &value,
                       bool markAsApplied);

    QMap<QString, QString> getChangedFields() const;
    QStringList getChangedFieldDisplayNames() const;

    void refreshStatusBar();
};

#endif // APPLICATIONCONFWND_H
