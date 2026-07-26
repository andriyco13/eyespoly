#pragma once

#include <QObject>
#include <QQmlApplicationEngine>
#include <QTranslator>
#include <QGuiApplication>
#include <QSettings>

class LanguageManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentLanguage READ currentLanguage WRITE setCurrentLanguage NOTIFY currentLanguageChanged)

public:
    explicit LanguageManager(QQmlApplicationEngine *engine, QObject *parent = nullptr);

    QString currentLanguage() const;
    void setCurrentLanguage(const QString &lang);

signals:
    void currentLanguageChanged();

private:
    QQmlApplicationEngine *m_engine;
    QTranslator m_translator;
    QString m_currentLanguage;
};