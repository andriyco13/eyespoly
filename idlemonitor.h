#pragma once

#include <QObject>
#include <QDBusInterface>
#include <QTimer>

// Відстежує час простою системи (без руху миші/клавіатури) через
// D-Bus інтерфейс org.gnome.Mutter.IdleMonitor (GNOME, X11 та Wayland).
// Якщо застосунок запущено не під GNOME/Mutter, інтерфейс буде
// недоступний — у такому разі клас просто нічого не робить
// (isIdle завжди false), щоб не ламати решту програми.
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

    QDBusInterface *m_iface = nullptr;
    QTimer m_pollTimer;

    qint64 m_idleTimeMs = 0;
    qint64 m_idleThresholdMs = 5 * 60 * 1000; // 5 хвилин за замовчуванням
    bool m_isIdle = false;
    bool m_available = false;
};
