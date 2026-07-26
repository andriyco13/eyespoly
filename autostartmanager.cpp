#include "autostartmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>

#if defined(Q_OS_WIN)
#include <QSettings>
#endif

namespace {
const QString kAppKey = QStringLiteral("Eyespoly");

#if defined(Q_OS_WIN)
const QString kWindowsRunKey =
    QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run");
#endif
}

AutostartManager::AutostartManager(QObject *parent)
    : QObject(parent)
{
}

QString AutostartManager::autostartFilePath()
{
#if defined(Q_OS_LINUX)
    const QString autostartDir =
        QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
        + QStringLiteral("/autostart");
    return autostartDir + QStringLiteral("/eyespoly.desktop");
#else
    return QString();
#endif
}

bool AutostartManager::isEnabled() const
{
#if defined(Q_OS_LINUX)
    const QString path = autostartFilePath();
    return !path.isEmpty() && QFile::exists(path);
#elif defined(Q_OS_WIN)
    QSettings settings(kWindowsRunKey, QSettings::NativeFormat);
    return settings.contains(kAppKey);
#else
    return false;
#endif
}

void AutostartManager::setEnabled(bool enabled)
{
    if (enabled) {
        updateShortcut();
    } else {
#if defined(Q_OS_LINUX)
        QFile::remove(autostartFilePath());
#elif defined(Q_OS_WIN)
        QSettings settings(kWindowsRunKey, QSettings::NativeFormat);
        settings.remove(kAppKey);
#endif
    }
    emit enabledChanged();
}

bool AutostartManager::startMinimized() const { return m_startMinimized; }

void AutostartManager::setStartMinimized(bool minimized) {
    if (m_startMinimized == minimized) return;
    m_startMinimized = minimized;
    emit startMinimizedChanged();
    if (isEnabled()) updateShortcut(); // Оновлюємо ярлик, якщо автозапуск увімкнено
}

bool AutostartManager::startTimer() const { return m_startTimer; }

void AutostartManager::setStartTimer(bool start) {
    if (m_startTimer == start) return;
    m_startTimer = start;
    emit startTimerChanged();
    if (isEnabled()) updateShortcut(); // Оновлюємо ярлик, якщо автозапуск увімкнено
}

void AutostartManager::updateShortcut()
{
    // Формуємо рядок з аргументами
    QString args;
    if (m_startMinimized) args += QStringLiteral(" --minimized");
    if (m_startTimer) args += QStringLiteral(" --start-timer");

#if defined(Q_OS_LINUX)
    const QString path = autostartFilePath();
    if (path.isEmpty()) return;

    QDir().mkpath(QFileInfo(path).absolutePath());

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "[Desktop Entry]\n";
        out << "Type=Application\n";
        out << "Name=Eyespoly\n";
        out << "Comment=Автозапуск Eyespoly разом із системою\n";
        out << "Exec=\"" << QCoreApplication::applicationFilePath() << "\"" << args << "\n";
        out << "Icon=eyespoly\n";
        out << "Terminal=false\n";
        out << "Hidden=false\n";
        out << "NoDisplay=false\n";
        out << "X-GNOME-Autostart-enabled=true\n";
        file.close();
    }
#elif defined(Q_OS_WIN)
    QSettings settings(kWindowsRunKey, QSettings::NativeFormat);
    const QString exePath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    settings.setValue(kAppKey, QStringLiteral("\"%1\"%2").arg(exePath, args));
#endif
}