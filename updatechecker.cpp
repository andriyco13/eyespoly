#include "updatechecker.h"
#include <QNetworkRequest>
#include <QUrl>

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent), m_manager(new QNetworkAccessManager(this)), m_showUpToDateMsg(false)
{
}

void UpdateChecker::checkForUpdates(const QString &currentVersion, bool showUpToDateMsg)
{
    m_currentVersion = currentVersion;
    m_showUpToDateMsg = showUpToDateMsg;

    QUrl url("https://api.github.com/repos/andriyco13/eyespoly/releases/latest");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Eyespoly-App");

    QNetworkReply *reply = m_manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
        reply->deleteLater();
    });
}

void UpdateChecker::onReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    if (!jsonDoc.isObject()) {
        emit errorOccurred("Invalid JSON response");
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    QString latestVersion = jsonObj.value("tag_name").toString();
    QString releaseUrl = jsonObj.value("html_url").toString();

    if (!latestVersion.isEmpty() && latestVersion != m_currentVersion) {
        emit updateAvailable(latestVersion, releaseUrl);
    } else {
        emit upToDate(m_showUpToDateMsg);
    }
}
