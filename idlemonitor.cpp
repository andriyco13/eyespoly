#include "idlemonitor.h"

#include <QDBusReply>
#include <QDebug>

namespace {
const QString kService = QStringLiteral("org.gnome.Mutter.IdleMonitor");
const QString kPath = QStringLiteral("/org/gnome/Mutter/IdleMonitor/Core");
const QString kInterface = QStringLiteral("org.gnome.Mutter.IdleMonitor");
}

IdleMonitor::IdleMonitor(QObject *parent)
    : QObject(parent)
{
    m_iface = new QDBusInterface(kService, kPath, kInterface,
                                  QDBusConnection::sessionBus(), this);
    m_available = m_iface->isValid();

    if (!m_available) {
        qWarning() << "IdleMonitor: org.gnome.Mutter.IdleMonitor недоступний через D-Bus"
                   << "(ймовірно, це не GNOME/Mutter, або сесія його не надає)."
                   << m_iface->lastError().message();
    }

    // Простий поллінг раз на секунду — надійніше й простіше за підписку
    // на AddIdleWatch/AddUserActiveWatch сигнали, а точності 1с цілком
    // достатньо для порогу в кілька хвилин.
    connect(&m_pollTimer, &QTimer::timeout, this, &IdleMonitor::poll);
    m_pollTimer.setInterval(1000);
    m_pollTimer.start();
}

qint64 IdleMonitor::idleTimeMs() const
{
    return m_idleTimeMs;
}

bool IdleMonitor::isIdle() const
{
    return m_isIdle;
}

qint64 IdleMonitor::idleThresholdMs() const
{
    return m_idleThresholdMs;
}

void IdleMonitor::setIdleThresholdMs(qint64 ms)
{
    if (m_idleThresholdMs == ms) {
        return;
    }
    m_idleThresholdMs = ms;
    emit idleThresholdMsChanged();
    updateIdleState();
}

void IdleMonitor::poll()
{
    if (!m_available) {
        return;
    }

    QDBusReply<qulonglong> reply = m_iface->call(QStringLiteral("GetIdletime"));
    if (!reply.isValid()) {
        return;
    }

    m_idleTimeMs = static_cast<qint64>(reply.value());
    emit idleTimeMsChanged();

    updateIdleState();
}

void IdleMonitor::updateIdleState()
{
    const bool newIdle = m_idleTimeMs >= m_idleThresholdMs;
    if (newIdle != m_isIdle) {
        m_isIdle = newIdle;
        emit isIdleChanged();
    }
}
