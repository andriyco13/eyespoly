#include "apptracker.h"
#include <QProcess>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QSqlRecord>
#include <QSqlField>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm> // ДОДАНО ДЛЯ СОРТУВАННЯ!

AppTracker::AppTracker(QObject *parent)
    : QObject(parent)
    , m_firstRun(true)
{
    if (!initDatabase()) {
        qWarning() << "Failed to initialize database!";
        return;
    }

    m_timer = new QTimer(this);
    m_timer->setInterval(5000);
    connect(m_timer, &QTimer::timeout, this, &AppTracker::updateCurrentApp);
    m_timer->start();

    updateCurrentApp();
}

AppTracker::~AppTracker()
{
    closeCurrentSession();
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool AppTracker::initDatabase()
{
    const QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/app_usage.db";
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));

    m_db = QSqlDatabase::addDatabase("QSQLITE", "AppTracker");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "Cannot open database:" << m_db.lastError().text();
        return false;
    }

    QSqlQuery query(m_db);
    bool success = query.exec(
        "CREATE TABLE IF NOT EXISTS app_sessions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "app_name TEXT NOT NULL, "
        "window_title TEXT, "
        "start_time INTEGER NOT NULL, "
        "end_time INTEGER, "
        "duration_seconds INTEGER, "
        "date TEXT NOT NULL"
        ")"
    );

    if (!success) {
        qWarning() << "Failed to create table:" << query.lastError().text();
        return false;
    }

    query.exec("CREATE INDEX IF NOT EXISTS idx_app_sessions_date ON app_sessions(date)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_app_sessions_app ON app_sessions(app_name)");

    return true;
}

void AppTracker::updateCurrentApp()
{

    QDateTime now = QDateTime::currentDateTime();

    // ЗАХИСТ ВІД СПЛЯЧОГО РЕЖИМУ:
    // Наш таймер працює кожні 5 секунд. Якщо між перевірками пройшло 
    // більше 30 секунд, значить комп'ютер спав або завис.
    if (!m_lastCheckTime.isNull() && m_lastCheckTime.secsTo(now) > 30) {
        m_sessionStart = now; // Починаємо нову сесію, відкидаючи час сну
    }
    m_lastCheckTime = now;

    QString windowInfo = getActiveWindowInfo();
    if (windowInfo.isEmpty()) {
        if (!m_currentApp.isEmpty()) {
            saveCurrentSession();
            m_currentApp = "";
            m_currentWindowTitle = "";
            m_currentWindowId = "";
            emit currentAppChanged();
            emit currentWindowTitleChanged();
        }
        return;
    }

    QStringList parts = windowInfo.split('|');
    if (parts.size() < 3) {
        return;
    }

    QString windowId = parts[0];
    QString appName = parts[1];
    QString windowTitle = parts[2];

    if (m_currentWindowId != windowId && !m_currentApp.isEmpty()) {
        saveCurrentSession();
    }

    m_currentWindowId = windowId;
    m_currentApp = appName;
    m_currentWindowTitle = windowTitle;

    if (m_firstRun || m_sessionStart.isNull()) {
        m_sessionStart = QDateTime::currentDateTime();
        m_firstRun = false;
    }

    emit currentAppChanged();
    emit currentWindowTitleChanged();
}

void AppTracker::saveCurrentSession(bool emitSignal)
{
    if (m_currentApp.isEmpty() || m_sessionStart.isNull()) {
        return;
    }

    QDateTime now = QDateTime::currentDateTime();
    qint64 duration = m_sessionStart.secsTo(now);

    if (duration < 2) {
        return;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO app_sessions (app_name, window_title, start_time, end_time, duration_seconds, date) "
        "VALUES (?, ?, ?, ?, ?, ?)"
    );

    QString dateStr = m_sessionStart.toString("yyyy-MM-dd");
    query.addBindValue(m_currentApp);
    query.addBindValue(m_currentWindowTitle);
    query.addBindValue(m_sessionStart.toSecsSinceEpoch());
    query.addBindValue(now.toSecsSinceEpoch());
    query.addBindValue(duration);
    query.addBindValue(dateStr);

    bool success = query.exec();

    m_sessionStart = now;

    if (!success) {
        qWarning() << "Failed to save session:" << query.lastError().text();
    } else if (emitSignal) {
        emit statsUpdated();
    }
}

void AppTracker::closeCurrentSession()
{
    if (!m_currentApp.isEmpty()) {
        saveCurrentSession();
    }
}

QString AppTracker::getActiveWindowInfo()
{
    // Спершу пробуємо розширення "Focused Window D-Bus"
    // (https://extensions.gnome.org/extension/5592/focused-window-d-bus/),
    // яке треба встановити й увімкнути окремо в GNOME Extensions.
    // Це реальний публічний D-Bus інтерфейс (на відміну від
    // попереднього org.gnome.Shell.Extensions.WindowsExt.FocusWindow,
    // якого не існує в жодному відомому розширенні).
    QProcess process;
    process.start("gdbus", QStringList() 
        << "call" << "--session"
        << "--dest" << "org.gnome.Shell" 
        << "--object-path" << "/org/gnome/shell/extensions/FocusedWindow" 
        << "--method" << "org.gnome.shell.extensions.FocusedWindow.Get"
    );

    if (process.waitForFinished(500)) {
        QString output = process.readAllStandardOutput().trimmed();
        QString errOutput = process.readAllStandardError().trimmed();

        if (!output.isEmpty() && output.contains("wm_class")) {
            int jsonStart = output.indexOf('{');
            int jsonEnd = output.lastIndexOf('}');
            
            if (jsonStart != -1 && jsonEnd != -1) {
                QString jsonStr = output.mid(jsonStart, jsonEnd - jsonStart + 1);
                
                QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonStr.toUtf8());
                if (!jsonDoc.isNull() && jsonDoc.isObject()) {
                    QJsonObject obj = jsonDoc.object();
                    QString appName = obj["wm_class"].toString();
                    QString title = obj["title"].toString();
                    qint64 winId = obj["id"].toVariant().toLongLong();

                    QString windowId = winId != 0 ? QString::number(winId) : ("wayland-" + appName);
                    return windowId + "|" + appName + "|" + title;
                }
            }
        } else if (!errOutput.isEmpty()) {
            qWarning() << "AppTracker: FocusedWindow D-Bus виклик не вдався:" << errOutput
                       << "(розширення 'Focused Window D-Bus' встановлено й увімкнено?)";
        }
    } else {
        qWarning() << "AppTracker: gdbus не відповів вчасно на FocusedWindow.Get";
    }

    // Якщо ми на GNOME Wayland і розширення немає, xdotool не спрацює для Wayland вікон.
    // Пробуємо отримати назву процесу через назву активного вікна (хоча б щось)
    QString xInfo = getActiveWindowInfoXdotool();
    if (!xInfo.isEmpty() && !xInfo.startsWith("|unknown|")) {
        return xInfo;
    }

    // Резервний метод для GNOME Wayland без розширень
    QProcess gnomeProcess;
    gnomeProcess.start("bash", QStringList() << "-c" << "busctl --user call org.gnome.Shell /org/gnome/Shell org.gnome.Shell Eval s 'global.display.focus_window ? global.display.focus_window.get_wm_class() : \"\"'");
    if (gnomeProcess.waitForFinished(500)) {
        QString out = gnomeProcess.readAllStandardOutput().trimmed();
        // busctl повертає щось на кшталт: b true "назва"
        int firstQuote = out.indexOf('"');
        int lastQuote = out.lastIndexOf('"');
        if (firstQuote != -1 && lastQuote > firstQuote) {
            QString appName = out.mid(firstQuote + 1, lastQuote - firstQuote - 1);
            if (!appName.isEmpty()) {
                return "gnome-" + appName + "|" + appName + "|Active Window";
            }
        }
    }

    return xInfo;
}

QString AppTracker::getActiveWindowInfoXdotool()
{
    QProcess process;
    process.start("xdotool", QStringList() << "getactivewindow" << "getwindowname");
    if (!process.waitForFinished(1000)) {
        return "";
    }

    QString output = process.readAllStandardOutput().trimmed();
    if (output.isEmpty()) {
        return "";
    }

    QProcess idProcess;
    idProcess.start("xdotool", QStringList() << "getactivewindow");
    if (!idProcess.waitForFinished(500)) {
        return "";
    }

    QString windowId = idProcess.readAllStandardOutput().trimmed();

    QProcess classProcess;
    classProcess.start("xprop", QStringList() << "-id" << windowId << "WM_CLASS");
    if (!classProcess.waitForFinished(500)) {
        return windowId + "|unknown|" + output;
    }

    QString classOutput = classProcess.readAllStandardOutput();
    int start = classOutput.indexOf('"') + 1;
    int end = classOutput.indexOf('"', start);
    if (start < 0 || end < 0) {
        return windowId + "|unknown|" + output;
    }

    QString appName = classOutput.mid(start, end - start);
    return windowId + "|" + appName + "|" + output;
}

QString AppTracker::getWindowTitleXdotool()
{
    QProcess process;
    process.start("xdotool", QStringList() << "getactivewindow" << "getwindowname");
    if (process.waitForFinished(500)) {
        return process.readAllStandardOutput().trimmed();
    }
    return "";
}

QVariantList AppTracker::getTodayStats()
{
    QString today = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    return getStatsForDate(today);
}

QVariantList AppTracker::getStatsForDate(const QString &date)
{
    saveCurrentSession(false); // Не відправляємо сигнал, щоб уникнути рекурсії

    QVariantList result;
    
    QSqlQuery query(m_db);
    query.prepare(
        "SELECT app_name, SUM(duration_seconds) as total_time "
        "FROM app_sessions "
        "WHERE date = ? "
        "GROUP BY app_name "
        "ORDER BY total_time DESC"
    );
    query.addBindValue(date);

    if (!query.exec()) {
        qWarning() << "Failed to get stats:" << query.lastError().text();
        return result;
    }

    while (query.next()) {
        QVariantMap item;
        item["appName"] = query.value("app_name").toString();
        item["duration"] = query.value("total_time").toInt();
        result.append(item);
    }

    return result;
}

QVariantList AppTracker::getStatsForLastDays(int days)
{
    saveCurrentSession(false); // Не відправляємо сигнал

    QVariantList result;
    
    QSqlQuery query(m_db);
    query.prepare(
        "SELECT app_name, SUM(duration_seconds) as total_time, date "
        "FROM app_sessions "
        "WHERE date >= date('now', ?) "
        "GROUP BY app_name "
        "ORDER BY total_time DESC"
    );
    query.addBindValue(QString("-%1 days").arg(days));

    if (!query.exec()) {
        qWarning() << "Failed to get stats:" << query.lastError().text();
        return result;
    }

    QVariantMap grouped;
    while (query.next()) {
        QString appName = query.value("app_name").toString();
        int duration = query.value("total_time").toInt();
        
        if (grouped.contains(appName)) {
            grouped[appName] = grouped[appName].toInt() + duration;
        } else {
            grouped[appName] = duration;
        }
    }

    for (auto it = grouped.begin(); it != grouped.end(); ++it) {
        QVariantMap item;
        item["appName"] = it.key();
        item["duration"] = it.value().toInt();
        result.append(item);
    }

    std::sort(result.begin(), result.end(), [](const QVariant &a, const QVariant &b) {
        return a.toMap()["duration"].toInt() > b.toMap()["duration"].toInt();
    });

    return result;
}

int AppTracker::getTotalScreenTimeForDate(const QString &date)
{
    saveCurrentSession(false); // Не відправляємо сигнал

    QSqlQuery query(m_db);
    query.prepare(
        "SELECT SUM(duration_seconds) as total "
        "FROM app_sessions "
        "WHERE date = ?"
    );
    query.addBindValue(date);

    if (query.exec() && query.next()) {
        return query.value("total").toInt();
    }
    return 0;
}

QStringList AppTracker::getAvailableDates()
{
    QStringList dates;
    QSqlQuery query(m_db);
    query.exec("SELECT DISTINCT date FROM app_sessions ORDER BY date DESC");

    while (query.next()) {
        dates.append(query.value(0).toString());
    }
    return dates;
}