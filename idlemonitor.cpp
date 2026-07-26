#include "idlemonitor.h"
#include <QDBusReply>
#include <QProcess>
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

// ПЕРЕВІРКА АУДІО (Щоб таймер не скидався під час YouTube/Кіно)
bool IdleMonitor::isAudioPlaying() const
{
    // Спосіб 1: Через PipeWire / PulseAudio (стандарт для сучасних Linux)
    QProcess process;
    process.start("sh", QStringList() << "-c" << "pactl list short sinks | grep -q RUNNING");
    process.waitForFinished(200); // Чекаємо максимум 200мс
    if (process.exitCode() == 0) return true;

    // Спосіб 2: Резервний через ядро (ALSA) на випадок, якщо pactl не встановлено
    QProcess processAlsa;
    processAlsa.start("sh", QStringList() << "-c" << "grep -q RUNNING /proc/asound/card*/pcm*/sub*/status 2>/dev/null");
    processAlsa.waitForFinished(200);
    return (processAlsa.exitCode() == 0);
}

void IdleMonitor::poll()
{
    if (!m_available) return;

    QDBusReply<qulonglong> reply = m_iface->call(QStringLiteral("GetIdletime"));
    if (!reply.isValid()) return;

    m_idleTimeMs = static_cast<qint64>(reply.value());
    emit idleTimeMsChanged();

    updateIdleState();
}

void IdleMonitor::updateIdleState()
{
    bool newIdle = (m_idleTimeMs >= m_idleThresholdMs);
    
    // Якщо система каже, що мишка не рухалась, але грає звук — ігноруємо неактивність!
    if (newIdle && isAudioPlaying()) {
        newIdle = false; 
    }

    if (newIdle != m_isIdle) {
        m_isIdle = newIdle;
        emit isIdleChanged();
    }
}