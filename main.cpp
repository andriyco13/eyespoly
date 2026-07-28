#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCommandLineParser>
#include <cstdlib>
#include <cstdio>
#include <thread>
#include <chrono>
#include <QIcon>
#include <QQuickStyle>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QWindow>
#include <QStyle>
#include <QTimer>
#include <QPixmap>

#include "languagemanager.h"
#include "autostartmanager.h"
#include "idlemonitor.h"
#include "apptracker.h"

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    // Ініціалізація імен
    QCoreApplication::setOrganizationName(QStringLiteral("Eyespoly"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("eyespoly.local"));
    QCoreApplication::setApplicationName(QStringLiteral("Eyespoly"));
    QGuiApplication::setDesktopFileName(QStringLiteral("eyespoly"));

    // Запобігаємо появі білих квадратів на Windows
    QQuickStyle::setStyle("Basic");

    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false); // Не вбивати програму при закритті вікна
    app.setWindowIcon(QIcon(":/qt/qml/Eyespoly/icon.png"));

    // Парсинг аргументів командного рядка
    QCommandLineParser parser;
    QCommandLineOption minOption("minimized", "Start minimized in tray");
    QCommandLineOption startOption("start-timer", "Start timer automatically");
    parser.addOption(minOption);
    parser.addOption(startOption);
    parser.process(app);

    bool argMinimized = parser.isSet(minOption);
    bool argStartTimer = parser.isSet(startOption);

    // Watchdog для гарантованого завершення
    QObject::connect(&app, &QApplication::aboutToQuit, []() {
        std::fprintf(stderr, "[watchdog] aboutToQuit спрацював\n");
        std::fflush(stderr);
        std::thread([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            std::_Exit(0);
        }).detach();
    });

    AutostartManager autostartManager;
    IdleMonitor idleMonitor;
    AppTracker appTracker;

    QQmlApplicationEngine engine;
    LanguageManager langManager(&engine);

    // --- БРОНЕБІЙНИЙ СИСТЕМНИЙ ТРЕЙ ---
    QIcon myIcon(":/qt/qml/Eyespoly/icon.png");

    // Перевірка: якщо іконки немає - малюємо червоний квадрат
    if (myIcon.isNull() || myIcon.availableSizes().isEmpty()) {
        QPixmap fallback(32, 32);
        fallback.fill(Qt::red);
        myIcon = QIcon(fallback);
    }

    QSystemTrayIcon *trayIcon = new QSystemTrayIcon(myIcon, &app);
    trayIcon->setToolTip(QStringLiteral("Eyespoly"));

    QMenu *trayMenu = new QMenu();
    QAction *toggleAction = trayMenu->addAction("Показати / Сховати");
    trayMenu->addSeparator();
    QAction *quitAction = trayMenu->addAction("Вийти");
    trayIcon->setContextMenu(trayMenu);

    QObject::connect(quitAction, &QAction::triggered, &app, &QCoreApplication::quit);

    // Передаємо всі об'єкти в QML, включаючи наш новий трей
    engine.rootContext()->setContextProperty("langManager", &langManager);
    engine.rootContext()->setContextProperty("argMinimized", argMinimized);
    engine.rootContext()->setContextProperty("argStartTimer", argStartTimer);
    engine.rootContext()->setContextProperty("autostartManager", &autostartManager);
    engine.rootContext()->setContextProperty("idleMonitor", &idleMonitor);
    engine.rootContext()->setContextProperty("appTracker", &appTracker);
    engine.rootContext()->setContextProperty("trayIcon", trayIcon);

    const QUrl url(u"qrc:/qt/qml/Eyespoly/main.qml"_s);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url, trayIcon, toggleAction, argMinimized](QObject *obj, const QUrl &objUrl) {
                         if (!obj && url == objUrl) {
                             QCoreApplication::exit(-1);
                             return;
                         }

                         QWindow *window = qobject_cast<QWindow*>(obj);
                         if (window) {
                             // Якщо програму не просили запустити згорнутою — показуємо вікно
                             if (!argMinimized) {
                                 window->show();
                                 window->raise();
                                 window->requestActivate();
                             }

                             // Клік по кнопці в меню трею
                             QObject::connect(toggleAction, &QAction::triggered, [window]() {
                                 if (window->isVisible()) {
                                     window->hide();
                                 } else {
                                     window->show();
                                     window->raise();
                                     window->requestActivate();
                                 }
                             });

                             // Клік по самій іконці трею
                             QObject::connect(trayIcon, &QSystemTrayIcon::activated, [window](QSystemTrayIcon::ActivationReason reason) {
                                 if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
                                     if (window->isVisible()) {
                                         window->hide();
                                     } else {
                                         window->show();
                                         window->raise();
                                         window->requestActivate();
                                     }
                                 }
                             });
                         }
                     }, Qt::QueuedConnection);

    engine.load(url);

    // ДОДАЙ ЦЕЙ БЛОК: Відкладений запуск трею на 500 мілісекунд (півсекунди)
    QTimer::singleShot(500, trayIcon, [trayIcon]() {
        trayIcon->show();
    });

    return app.exec();
}