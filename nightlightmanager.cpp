#include "nightlightmanager.h"

#include <QEvent>
#include <QGuiApplication>
#include <QPainter>
#include <QRegion>
#include <QScreen>
#include <QWidget>
#include <QWindow>

#include <utility>

#ifdef HAVE_LAYER_SHELL_QT
#include <LayerShellQt/Window>
#endif

namespace {
constexpr int kMinIntensity = 5;
constexpr int kMaxIntensity = 100;
constexpr int kMinTemperature = 2700;
constexpr int kMaxTemperature = 5500;
}

class NightLightOverlay : public QWidget
{
public:
    NightLightOverlay(QScreen *screen, const QColor &color)
        : QWidget(nullptr,
                  Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
                      | Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus
                      | Qt::Tool)
        , m_color(color)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setFocusPolicy(Qt::NoFocus);
        setWindowTitle(QStringLiteral("Eyespoly night light"));
        setScreen(screen);
        setGeometry(screen->geometry());

#ifdef HAVE_LAYER_SHELL_QT
        if (QGuiApplication::platformName() == QLatin1String("wayland")) {
            createWinId();
            if (QWindow *window = windowHandle()) {
                if (LayerShellQt::Window *layerShell = LayerShellQt::Window::get(window)) {
                    layerShell->setScreen(screen);
                    layerShell->setLayer(LayerShellQt::Window::LayerOverlay);
                    LayerShellQt::Window::Anchors anchors;
                    anchors |= LayerShellQt::Window::AnchorTop;
                    anchors |= LayerShellQt::Window::AnchorBottom;
                    anchors |= LayerShellQt::Window::AnchorLeft;
                    anchors |= LayerShellQt::Window::AnchorRight;
                    layerShell->setAnchors(anchors);
                    layerShell->setExclusiveZone(-1);
                    layerShell->setKeyboardInteractivity(
                        LayerShellQt::Window::KeyboardInteractivityNone);
                    layerShell->setActivateOnShow(false);
                }
            }
            m_usesLayerShell = true;
        }
#endif
    }

    void setOverlayColor(const QColor &color)
    {
        if (m_color == color)
            return;
        m_color = color;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(rect(), m_color);
    }

    void showEvent(QShowEvent *event) override
    {
        QWidget::showEvent(event);
        if (m_usesLayerShell)
            setMask(QRegion());
    }

    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        if (m_usesLayerShell)
            setMask(QRegion());
    }

private:
    QColor m_color;
    bool m_usesLayerShell = false;
};

NightLightManager::NightLightManager(QObject *parent)
    : QObject(parent)
{
    for (QScreen *screen : QGuiApplication::screens())
        connectScreen(screen);

    connect(qGuiApp, &QGuiApplication::screenAdded, this, &NightLightManager::handleScreenAdded);
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, &NightLightManager::handleScreenRemoved);
}

NightLightManager::~NightLightManager()
{
    qDeleteAll(m_overlays);
    m_overlays.clear();
}

bool NightLightManager::isEnabled() const
{
    return m_enabled;
}

void NightLightManager::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;
    updateOverlays();
    emit enabledChanged();
}

int NightLightManager::intensity() const
{
    return m_intensity;
}

void NightLightManager::setIntensity(int intensity)
{
    const int value = qBound(kMinIntensity, intensity, kMaxIntensity);
    if (m_intensity == value)
        return;

    m_intensity = value;
    applyColorToOverlays();
    emit intensityChanged();
}

int NightLightManager::temperature() const
{
    return m_temperature;
}

void NightLightManager::setTemperature(int temperature)
{
    const int value = qBound(kMinTemperature, temperature, kMaxTemperature);
    if (m_temperature == value)
        return;

    m_temperature = value;
    applyColorToOverlays();
    emit temperatureChanged();
}

void NightLightManager::handleScreenAdded(QScreen *screen)
{
    connectScreen(screen);
    if (m_enabled)
        updateOverlays();
}

void NightLightManager::handleScreenRemoved(QScreen *screen)
{
    removeOverlay(screen);
}

void NightLightManager::handleGeometryChanged()
{
    QScreen *screen = qobject_cast<QScreen *>(sender());
    if (!screen)
        return;

    if (NightLightOverlay *overlay = m_overlays.value(screen))
        overlay->setGeometry(screen->geometry());
}

void NightLightManager::connectScreen(QScreen *screen)
{
    if (!screen || m_connectedScreens.contains(screen))
        return;

    m_connectedScreens.insert(screen);
    connect(screen, &QScreen::geometryChanged, this, &NightLightManager::handleGeometryChanged);
    connect(screen, &QObject::destroyed, this, [this, screen]() { removeOverlay(screen); });
}

void NightLightManager::removeOverlay(QScreen *screen)
{
    m_connectedScreens.remove(screen);
    if (NightLightOverlay *overlay = m_overlays.take(screen)) {
        overlay->hide();
        overlay->deleteLater();
    }
}

void NightLightManager::syncOverlays()
{
    const QList<QScreen *> screens = QGuiApplication::screens();

    for (QScreen *screen : screens) {
        if (!m_overlays.contains(screen))
            m_overlays.insert(screen, new NightLightOverlay(screen, overlayColor()));
    }

    for (auto it = m_overlays.begin(); it != m_overlays.end();) {
        if (!screens.contains(it.key())) {
            NightLightOverlay *overlay = it.value();
            it = m_overlays.erase(it);
            overlay->hide();
            overlay->deleteLater();
        } else {
            ++it;
        }
    }
}

void NightLightManager::updateOverlays()
{
    if (!m_enabled) {
        for (NightLightOverlay *overlay : std::as_const(m_overlays))
            overlay->hide();
        return;
    }

    syncOverlays();
    applyColorToOverlays();

    for (NightLightOverlay *overlay : std::as_const(m_overlays)) {
        overlay->show();
        overlay->raise();
    }
}

void NightLightManager::applyColorToOverlays()
{
    const QColor color = overlayColor();
    for (NightLightOverlay *overlay : std::as_const(m_overlays))
        overlay->setOverlayColor(color);
}

QColor NightLightManager::overlayColor() const
{
    const qreal ratio = qreal(m_temperature - kMinTemperature)
        / qreal(kMaxTemperature - kMinTemperature);
    const qreal t = qBound(qreal(0.0), ratio, qreal(1.0));

    const int green = qRound(175.0 + t * 45.0);
    const int blue = qRound(15.0 + t * 60.0);
    const int alpha = qRound(255.0 * m_intensity / 100.0);

    return QColor(255, green, blue, alpha);
}
