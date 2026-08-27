#include "idlemonitor.h"
#include <QDebug>

#if defined(Q_OS_LINUX)
#include <QDBusReply>
#include <QProcess>
#elif defined(Q_OS_WIN)
#include <windows.h>
#endif

namespace {
#if defined(Q_OS_LINUX)
const QString kService = QStringLiteral("org.gnome.Mutter.IdleMonitor");
const QString kPath = QStringLiteral("/org/gnome/Mutter/IdleMonitor/Core");
const QString kInterface = QStringLiteral("org.gnome.Mutter.IdleMonitor");
#endif
}

IdleMonitor::IdleMonitor(QObject *parent)
    : QObject(parent)
{
#if defined(Q_OS_LINUX)
    m_iface = new QDBusInterface(kService, kPath, kInterface,
                                  QDBusConnection::sessionBus(), this);
    m_available = m_iface->isValid();

    if (!m_available) {
        m_fallbackIface = new QDBusInterface(QStringLiteral("org.freedesktop.ScreenSaver"),
                                             QStringLiteral("/org/freedesktop/ScreenSaver"),
                                             QStringLiteral("org.freedesktop.ScreenSaver"),
                                             QDBusConnection::sessionBus(), this);
        m_available = m_fallbackIface->isValid();
    }
#elif defined(Q_OS_WIN)
    // GetLastInputInfo доступний завжди в межах поточної сесії користувача.
    m_available = true;
#else
    m_available = false;
#endif

    connect(&m_pollTimer, &QTimer::timeout, this, &IdleMonitor::poll);
    m_pollTimer.setInterval(1000);
    m_pollTimer.start();
}

qint64 IdleMonitor::idleTimeMs() const { return m_idleTimeMs; }
bool IdleMonitor::isIdle() const { return m_isIdle; }
qint64 IdleMonitor::idleThresholdMs() const { return m_idleThresholdMs; }

void IdleMonitor::setIdleThresholdMs(qint64 ms)
{
    if (m_idleThresholdMs == ms) return;
    m_idleThresholdMs = ms;
    emit idleThresholdMsChanged();
    updateIdleState();
}

bool IdleMonitor::queryIdleTimeMs(qint64 &outIdleMs)
{
#if defined(Q_OS_LINUX)
    if (!m_available) return false;

    QDBusReply<qulonglong> reply;

    if (m_iface && m_iface->isValid()) {
        reply = m_iface->call(QStringLiteral("GetIdletime"));
    } else if (m_fallbackIface && m_fallbackIface->isValid()) {
        reply = m_fallbackIface->call(QStringLiteral("GetSessionIdleTime"));
    } else {
        return false;
    }

    if (!reply.isValid()) return false;

    outIdleMs = static_cast<qint64>(reply.value());
    return true;
#elif defined(Q_OS_WIN)
    LASTINPUTINFO lii;
    lii.cbSize = sizeof(LASTINPUTINFO);
    if (!GetLastInputInfo(&lii)) {
        return false;
    }

    const DWORD tickCount = GetTickCount();
    // GetTickCount переповнюється кожні ~49.7 днів; при переповненні
    // різниця все одно рахується коректно завдяки арифметиці без знаку.
    const DWORD idleTicks = tickCount - lii.dwTime;
    outIdleMs = static_cast<qint64>(idleTicks);
    return true;
#else
    Q_UNUSED(outIdleMs);
    return false;
#endif
}

void IdleMonitor::poll()
{
    qint64 newIdleMs = 0;
    if (!queryIdleTimeMs(newIdleMs)) return;

    m_idleTimeMs = newIdleMs;
    emit idleTimeMsChanged();

    updateIdleState();
}

void IdleMonitor::updateIdleState()
{
    bool newIdle = (m_idleTimeMs >= m_idleThresholdMs);

    if (newIdle != m_isIdle) {
        m_isIdle = newIdle;
        emit isIdleChanged();
    }
}
