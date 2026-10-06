#include "updatechecker.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QPushButton>
#include <QDesktopServices>
#include <QStringList>

UpdateChecker::UpdateChecker(QWidget *aParent) :
    QObject(aParent)
{
    parentWidget = aParent;
    silentMode = false;

    connect(&networkManager, &QNetworkAccessManager::finished,
            this, &UpdateChecker::onManifestFinished);
}

void UpdateChecker::checkForUpdates(bool silent)
{
    silentMode = silent;

    QNetworkRequest request((QUrl(UPDATE_MANIFEST_URL)));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader, QString("OpenEPT/%1").arg(APP_VERSION));

    networkManager.get(request);
}

QString UpdateChecker::normalizeVersion(const QString &version)
{
    QString value = version.trimmed();

    if(value.startsWith('v') || value.startsWith('V')) value.remove(0, 1);

    int cut = value.indexOf('-');
    if(cut >= 0) value = value.left(cut);

    cut = value.indexOf('+');
    if(cut >= 0) value = value.left(cut);

    return value;
}

int UpdateChecker::compareVersions(const QString &a, const QString &b)
{
    QStringList partsA = normalizeVersion(a).split('.');
    QStringList partsB = normalizeVersion(b).split('.');
    int count = qMax(partsA.size(), partsB.size());

    for(int i = 0; i < count; i++)
    {
        int valueA = i < partsA.size() ? partsA[i].toInt() : 0;
        int valueB = i < partsB.size() ? partsB[i].toInt() : 0;

        if(valueA != valueB) return valueA < valueB ? -1 : 1;
    }

    return 0;
}

void UpdateChecker::onManifestFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    if(reply->error() != QNetworkReply::NoError)
    {
        if(!silentMode)
            QMessageBox::warning(parentWidget, "Check for Updates",
                                 "Could not reach the update server.\n\n" + reply->errorString());
        return;
    }

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);

    if(parseError.error != QJsonParseError::NoError || !document.isObject())
    {
        if(!silentMode)
            QMessageBox::warning(parentWidget, "Check for Updates",
                                 "The update information could not be read.");
        return;
    }

    QJsonArray components = document.object().value("components").toArray();

    bool updateAvailable = false;
    QString selfUrl;
    QString firstUrl;
    QString listText;

    for(const QJsonValue &value : components)
    {
        QJsonObject component = value.toObject();
        QString name = component.value("name").toString();
        QString latest = component.value("latest").toString();
        QString url = component.value("url").toString();
        bool isSelf = component.value("self").toBool();

        if(name.isEmpty() || latest.isEmpty()) continue;

        QString status;

        if(isSelf)
        {
            selfUrl = url;

            if(compareVersions(APP_VERSION, latest) < 0)
            {
                updateAvailable = true;
                status = "   (update available, you have " + QString(APP_VERSION) + ")";
            }
            else
            {
                status = "   (up to date)";
            }
        }

        listText += "    " + name + "  " + latest + status + "\n";

        if(firstUrl.isEmpty() && !url.isEmpty()) firstUrl = url;
    }

    if(listText.isEmpty())
    {
        if(!silentMode)
            QMessageBox::information(parentWidget, "Check for Updates",
                                     "No update information is available.");
        return;
    }

    if(silentMode && !updateAvailable) return;

    QString downloadUrl = !selfUrl.isEmpty() ? selfUrl : firstUrl;
    QString header = updateAvailable
        ? "A newer OpenEPT version is available."
        : "You are running the latest OpenEPT version.";

    QMessageBox box(parentWidget);
    box.setWindowTitle("Check for Updates");
    box.setIcon(QMessageBox::Information);
    box.setText(header);
    box.setInformativeText("Latest available versions:\n\n" + listText);

    QPushButton *downloadButton = box.addButton("Go to Downloads", QMessageBox::AcceptRole);
    box.addButton(QMessageBox::Close);
    if(updateAvailable) box.setDefaultButton(downloadButton);

    box.exec();

    if(box.clickedButton() == downloadButton && !downloadUrl.isEmpty())
        QDesktopServices::openUrl(QUrl(downloadUrl));
}
