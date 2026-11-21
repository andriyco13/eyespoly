#ifndef NIGHTLIGHTMANAGER_H
#define NIGHTLIGHTMANAGER_H

#include <QColor>
#include <QHash>
#include <QObject>
#include <QSet>

class QScreen;
class NightLightOverlay;

class NightLightManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(int intensity READ intensity WRITE setIntensity NOTIFY intensityChanged)
    Q_PROPERTY(int temperature READ temperature WRITE setTemperature NOTIFY temperatureChanged)

public:
    explicit NightLightManager(QObject *parent = nullptr);
    ~NightLightManager() override;

    bool isEnabled() const;
    void setEnabled(bool enabled);

    int intensity() const;
    void setIntensity(int intensity);

    int temperature() const;
    void setTemperature(int temperature);

signals:
    void enabledChanged();
    void intensityChanged();
    void temperatureChanged();

private:
    void handleScreenAdded(QScreen *screen);
    void handleScreenRemoved(QScreen *screen);
    void handleGeometryChanged();
    void connectScreen(QScreen *screen);
    void removeOverlay(QScreen *screen);
    void syncOverlays();
    void updateOverlays();
    void applyColorToOverlays();
    QColor overlayColor() const;

    bool m_enabled = false;
    int m_intensity = 30;
    int m_temperature = 4000;
    QHash<QScreen *, NightLightOverlay *> m_overlays;
    QSet<QScreen *> m_connectedScreens;
};

#endif // NIGHTLIGHTMANAGER_H
