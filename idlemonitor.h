#pragma once

#include <QObject>
#include <QTimer>

#if defined(Q_OS_LINUX)
#include <QDBusInterface>
#endif

class IdleMonitor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qint64 idleTimeMs READ idleTimeMs NOTIFY idleTimeMsChanged)
    Q_PROPERTY(bool isIdle READ isIdle NOTIFY isIdleChanged)
    Q_PROPERTY(qint64 idleThresholdMs READ idleThresholdMs WRITE setIdleThresholdMs NOTIFY idleThresholdMsChanged)

public:
    explicit IdleMonitor(QObject *parent = nullptr);

    qint64 idleTimeMs() const;
    bool isIdle() const;

    qint64 idleThresholdMs() const;
    void setIdleThresholdMs(qint64 ms);

signals:
    void idleTimeMsChanged();
    void isIdleChanged();
    void idleThresholdMsChanged();

private slots:
    void poll();

private:
    void updateIdleState();
    bool isAudioPlaying() const; // Захист від скидання таймера під час відео/музики
    bool queryIdleTimeMs(qint64 &outIdleMs); // Платформозалежний запит часу бездіяльності; false = недоступно

#if defined(Q_OS_LINUX)
    QDBusInterface *m_iface = nullptr;
#endif
    bool m_available = false;

    QTimer m_pollTimer;

    qint64 m_idleTimeMs = 0;
    qint64 m_idleThresholdMs = 5 * 60 * 1000;
    bool m_isIdle = false;
};
