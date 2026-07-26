#ifndef APPTRACKER_H
#define APPTRACKER_H

#include <QObject>
#include <QTimer>
#include <QHash>
#include <QString>
#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariantList>
#include <QVariantMap>

class AppTracker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentApp READ currentApp NOTIFY currentAppChanged)
    Q_PROPERTY(QString currentWindowTitle READ currentWindowTitle NOTIFY currentWindowTitleChanged)

public:
    explicit AppTracker(QObject *parent = nullptr);
    ~AppTracker();

    QString currentApp() const { return m_currentApp; }
    QString currentWindowTitle() const { return m_currentWindowTitle; }

    Q_INVOKABLE QVariantList getTodayStats();
    Q_INVOKABLE QVariantList getStatsForDate(const QString &date);
    Q_INVOKABLE QVariantList getStatsForLastDays(int days);
    Q_INVOKABLE int getTotalScreenTimeForDate(const QString &date);
    Q_INVOKABLE QStringList getAvailableDates();

signals:
    void currentAppChanged();
    void currentWindowTitleChanged();
    void statsUpdated();

public slots:
    void updateCurrentApp();

private:
    bool initDatabase();
    void saveCurrentSession(bool emitSignal = true);
    void closeCurrentSession();
    QString getActiveWindowInfo();
    QString getActiveWindowInfoXdotool();
    QString getWindowTitleXdotool();
    
    QTimer *m_timer;
    QString m_currentApp;
    QString m_currentWindowTitle;
    QString m_currentWindowId;
    QDateTime m_sessionStart;
    QDateTime m_lastCheckTime;
    QSqlDatabase m_db;
    bool m_firstRun;
};

#endif // APPTRACKER_H