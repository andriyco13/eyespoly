#pragma once

#include <QObject>

// Керує запуском програми разом із системою.
// Наразі реалізовано для Linux (створення/видалення .desktop файлу
// в ~/.config/autostart), оскільки саме там точка входу стандартна
// для GNOME/KDE/XFCE тощо (freedesktop.org autostart spec).
class AutostartManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)

public:
    explicit AutostartManager(QObject *parent = nullptr);

    bool isEnabled() const;
    void setEnabled(bool enabled);

signals:
    void enabledChanged();

private:
    static QString autostartFilePath();
};
