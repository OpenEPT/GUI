#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QNetworkAccessManager>

QT_FORWARD_DECLARE_CLASS(QNetworkReply)

#ifndef APP_VERSION
#define APP_VERSION "0.0.0-dev"
#endif

#ifndef UPDATE_MANIFEST_URL
#define UPDATE_MANIFEST_URL "https://www.openept.net/downloads/manifest.json"
#endif

class UpdateChecker : public QObject
{
    Q_OBJECT

public:
    explicit            UpdateChecker(QWidget *aParent = nullptr);

    void                checkForUpdates(bool silent);

private slots:
    void                onManifestFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager   networkManager;
    QWidget                *parentWidget;
    bool                    silentMode;

    static QString          normalizeVersion(const QString &version);
    static int              compareVersions(const QString &a, const QString &b);
};

#endif // UPDATECHECKER_H
