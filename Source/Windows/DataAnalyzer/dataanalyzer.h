#ifndef DATAANALYZER_H
#define DATAANALYZER_H

#include <QWidget>
#include <QString>
#include <QStringList>
#include <QComboBox>
#include <QBoxLayout>
#include <QLabel>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QDir>
#include <QShowEvent>
#include <QMap>
#include <QToolButton>
#include <QEvent>

#include "dataanalyzerworker.h"
#include "dataanalyzerprofile.h"

namespace Ui {
class DataAnalyzer;
}

class DataAnalyzer : public QWidget
{
    Q_OBJECT

public:
    explicit DataAnalyzer(QWidget *parent = nullptr, QString aWsDirPath="");
    ~DataAnalyzer();

    void                                setWorkspacePath(QString aWsDirPath);

protected:
    void                                showEvent(QShowEvent *event) override;
    bool                                eventFilter(QObject *object, QEvent *event) override;

public slots:
    void                                onDeleteConsumptionProfile();
    void                                onRealoadConsumptionProfiles();
    void                                onConsumptionProfileChanged(int index);
    void                                onLoadConsumptionProfileData();
    void                                onProfileDockStateToggleRequested();
    void                                onMaximizeProfile();
    void                                onProfileDestroyed(QObject *object);

private:
    Ui::DataAnalyzer                    *ui;

    QMdiArea                            *mdiArea;
    QToolButton                         *maximizeProfilePushb;
    QLabel*                             detectedProfilesLabe;
    QComboBox                           *consumptionProfilesCB;
    QStringList                         consumptionProfilesName;

    QString                             wsDirPath;
    QString                             selectedConsumptionProfile;

    QVector<DataAnalyzerProfile*>       profiles;
    QMap<QString, int>                  profileCounters;

    QStringList                         listConsumptionProfiles();
    void                                listConsumptionProfilesInDir(QDir dir, QString relativePath, QStringList& profiles, int depth);
    void                                realoadConsumptionProfiles();
    QMdiSubWindow*                      subWindowFor(DataAnalyzerProfile *profile);
    void                                attachProfile(DataAnalyzerProfile *profile, QString title);
    void                                toggleProfileDockState(DataAnalyzerProfile *profile);
    void                                updateMaximizeButton();
};

#endif // DATAANALYZER_H
