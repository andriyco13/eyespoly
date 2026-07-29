#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>

class UpdateChecker : public QObject
{
    Q_OBJECT
public:
    explicit UpdateChecker(QObject *parent = nullptr);

    Q_INVOKABLE void checkForUpdates(const QString &currentVersion, bool showUpToDateMsg = false);

signals:
    void updateAvailable(QString latestVersion, QString releaseUrl);
    void upToDate(bool showMsg);
    void errorOccurred(QString errorString);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_manager;
    QString m_currentVersion;
    bool m_showUpToDateMsg;
};

#endif // UPDATECHECKER_H
