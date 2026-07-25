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
// Ім'я значення в реєстрі Windows / ім'я .desktop файлу на Linux
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
    // Не використовується на Windows/macOS — там інші механізми (реєстр / plist)
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
    // TODO: macOS (LaunchAgents plist у ~/Library/LaunchAgents)
    return false;
#endif
}

void AutostartManager::setEnabled(bool enabled)
{
#if defined(Q_OS_LINUX)
    const QString path = autostartFilePath();
    if (path.isEmpty()) {
        return;
    }

    if (enabled) {
        QDir().mkpath(QFileInfo(path).absolutePath());

        QFile file(path);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "[Desktop Entry]\n";
            out << "Type=Application\n";
            out << "Name=Eyespoly\n";
            out << "Comment=Автозапуск Eyespoly разом із системою\n";
            out << "Exec=\"" << QCoreApplication::applicationFilePath() << "\"\n";
            out << "Icon=eyespoly\n";
            out << "Terminal=false\n";
            out << "Hidden=false\n";
            out << "NoDisplay=false\n";
            out << "X-GNOME-Autostart-enabled=true\n";
            file.close();
        }
    } else {
        QFile::remove(path);
    }
#elif defined(Q_OS_WIN)
    QSettings settings(kWindowsRunKey, QSettings::NativeFormat);
    if (enabled) {
        const QString exePath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        // Обов'язково в лапках — шлях може містити пробіли (наприклад, "Program Files")
        settings.setValue(kAppKey, QStringLiteral("\"%1\"").arg(exePath));
    } else {
        settings.remove(kAppKey);
    }
#else
    Q_UNUSED(enabled);
#endif

    emit enabledChanged();
}
