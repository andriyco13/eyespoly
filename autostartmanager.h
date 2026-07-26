#pragma once

#include <QObject>

class AutostartManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool startMinimized READ startMinimized WRITE setStartMinimized NOTIFY startMinimizedChanged)
    Q_PROPERTY(bool startTimer READ startTimer WRITE setStartTimer NOTIFY startTimerChanged)

public:
    explicit AutostartManager(QObject *parent = nullptr);

    bool isEnabled() const;
    void setEnabled(bool enabled);

    bool startMinimized() const;
    void setStartMinimized(bool minimized);

    bool startTimer() const;
    void setStartTimer(bool start);

signals:
    void enabledChanged();
    void startMinimizedChanged();
    void startTimerChanged();

private:
    static QString autostartFilePath();
    void updateShortcut();

    bool m_startMinimized = false;
    bool m_startTimer = false;
};