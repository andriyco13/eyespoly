#include <QGuiApplication>
#include <QQmlApplicationEngine>

using namespace Qt::StringLiterals; // Додаємо простір імен для суфікса _s

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    
    // Новий правильний шлях для Qt 6.11 та суфікс _s замість _qs
    const QUrl url(u"qrc:/qt/qml/Eyespoly/main.qml"_s);
    
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);
        
    engine.load(url);

    return app.exec();
}