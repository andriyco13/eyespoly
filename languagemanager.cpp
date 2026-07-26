#include "languagemanager.h"

LanguageManager::LanguageManager(QQmlApplicationEngine *engine, QObject *parent)
    : QObject(parent), m_engine(engine)
{
    // Читаємо збережену мову (за замовчуванням "uk" - українська)
    QSettings settings;
    m_currentLanguage = settings.value("language", "uk").toString();
    
    // Застосовуємо мову при старті
    setCurrentLanguage(m_currentLanguage);
}

QString LanguageManager::currentLanguage() const { return m_currentLanguage; }

void LanguageManager::setCurrentLanguage(const QString &lang)
{
    m_currentLanguage = lang;
    
    // Зберігаємо вибір користувача
    QSettings settings;
    settings.setValue("language", lang);

    // Знімаємо старий переклад
    qApp->removeTranslator(&m_translator);

    // Якщо вибрана англійська - вантажимо файл перекладу
    if (lang == "en") {
        if (m_translator.load(":/i18n/eyespoly_en.qm")) {
            qApp->installTranslator(&m_translator);
        }
    }

    // МАГІЯ: Миттєво оновлюємо всі qsTr() у запущеному QML
    m_engine->retranslate();
    
    emit currentLanguageChanged();
}