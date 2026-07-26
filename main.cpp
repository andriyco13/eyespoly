#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
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
// AppIndicator / KStatusNotifierItem (розширення GNOME для трея) не вміє
// читати іконку напряму з Qt-ресурсів (qrc:/...) — йому потрібен реальний
// файл на диску або ім'я з теми іконок. Тому копіюємо іконку з ресурсів
// у теку даних застосунку один раз при старті й повертаємо абсолютний шлях.
QString extractTrayIconPath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString destPath = dir + QStringLiteral("/icon.png");

    QDir().mkpath(dir);

    // Перезаписуємо щоразу (на випадок оновлення іконки між збірками)
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

    // Підстраховка: на Linux фонові потоки QtMultimedia (аудіо-бекенд)
    // або D-Bus з'єднання трея інколи не приєднуються (join) коректно
    // при виході з event loop, і процес "висить" в консолі навіть
    // після Qt.quit(). ВАЖЛИВО: тут не можна використовувати
    // QTimer::singleShot — aboutToQuit спрацьовує саме тоді, коли
    // event loop от-от зупиниться, тож запланований у ньому Qt-таймер
    // просто не встигає відпрацювати. Тому — окремий системний потік,
    // який не залежить від Qt event loop взагалі.
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