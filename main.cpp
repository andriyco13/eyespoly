#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCommandLineParser> // ДОДАНО
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <cstdlib>
#include <cstdio>
#include <thread>
#include <chrono>

#include "autostartmanager.h"
#include "idlemonitor.h"
#include "apptracker.h"

using namespace Qt::StringLiterals;

namespace {
QString extractTrayIconPath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString destPath = dir + QStringLiteral("/icon.png");

    QDir().mkpath(dir);

    QFile::remove(destPath);
    if (!QFile::copy(QStringLiteral(":/qt/qml/Eyespoly/icon.png"), destPath)) {
        qWarning() << "Не вдалося скопіювати іконку трея на диск:" << destPath;
        return QString();
    }
    QFile::setPermissions(destPath, QFile::ReadOwner | QFile::WriteOwner
                                     | QFile::ReadGroup | QFile::ReadOther);
    return destPath;
}
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("Eyespoly"));
    app.setOrganizationDomain(QStringLiteral("eyespoly.local"));
    app.setApplicationName(QStringLiteral("Eyespoly"));

    // --- ПАРСИНГ АРГУМЕНТІВ ---
    QCommandLineParser parser;
    QCommandLineOption minOption("minimized", "Start minimized in tray");
    QCommandLineOption startOption("start-timer", "Start timer automatically");
    parser.addOption(minOption);
    parser.addOption(startOption);
    parser.process(app);

    bool argMinimized = parser.isSet(minOption);
    bool argStartTimer = parser.isSet(startOption);

    QObject::connect(&app, &QGuiApplication::aboutToQuit, []() {
        std::fprintf(stderr, "[watchdog] aboutToQuit спрацював, запускаю watchdog-потік\n");
        std::fflush(stderr);
        std::thread([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            std::fprintf(stderr, "[watchdog] минула 1с, викликаю std::_Exit(0)\n");
            std::fflush(stderr);
            std::_Exit(0);
        }).detach();
    });

    AutostartManager autostartManager;
    IdleMonitor idleMonitor;
    AppTracker appTracker;

    QQmlApplicationEngine engine;
    
    // Передаємо аргументи в QML
    engine.rootContext()->setContextProperty("argMinimized", argMinimized);
    engine.rootContext()->setContextProperty("argStartTimer", argStartTimer);
    
    engine.rootContext()->setContextProperty("autostartManager", &autostartManager);
    engine.rootContext()->setContextProperty("idleMonitor", &idleMonitor);
    engine.rootContext()->setContextProperty("appTracker", &appTracker);

    const QString trayIconPath = extractTrayIconPath();
    engine.rootContext()->setContextProperty(
        "trayIconUrl",
        trayIconPath.isEmpty() ? QString() : QUrl::fromLocalFile(trayIconPath).toString());

    const QUrl url(u"qrc:/qt/qml/Eyespoly/main.qml"_s);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}