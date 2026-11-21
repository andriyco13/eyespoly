#include "waylandtopleveltracker.h"

#include <QGuiApplication>
#include <QHash>
#include <QSocketNotifier>
#include <QString>

#if defined(Q_OS_LINUX) && defined(HAVE_WAYLAND_CLIENT)

#include <algorithm>
#include <cstring>

#include <wayland-client.h>

#include "cosmic-toplevel-info-unstable-v1-client-protocol.h"
#include "wlr-foreign-toplevel-management-unstable-v1-client-protocol.h"

namespace {

struct ToplevelData
{
    QString appId;
    QString title;
    bool activated = false;
    bool cosmic = false;
    bool wlr = false;
};

bool isWaylandSession()
{
    const QString platform = QGuiApplication::platformName();
    return platform.startsWith(QStringLiteral("wayland"))
        || qEnvironmentVariable("XDG_SESSION_TYPE") == QStringLiteral("wayland");
}

QString proxyWindowId(void *handle)
{
    return QStringLiteral("wayland-")
        + QString::number(static_cast<qulonglong>(reinterpret_cast<quintptr>(handle)));
}

} // namespace

class WaylandToplevelTracker::Private
{
public:
    explicit Private(WaylandToplevelTracker *owner)
        : q(owner)
    {
    }

    ~Private() { teardown(); }

    bool init();
    QString activeWindowInfo() const;
    void teardown();
    void removeToplevel(void *handle);
    void onReadable();

    static void registryGlobal(void *data, wl_registry *registry, uint32_t name,
                               const char *interface, uint32_t version);
    static void registryGlobalRemove(void *data, wl_registry *registry, uint32_t name);

    static void cosmicToplevel(void *data, zcosmic_toplevel_info_v1 *info,
                               zcosmic_toplevel_handle_v1 *handle);
    static void cosmicFinished(void *data, zcosmic_toplevel_info_v1 *info);

    static void cosmicHandleClosed(void *data, zcosmic_toplevel_handle_v1 *handle);
    static void cosmicHandleDone(void *data, zcosmic_toplevel_handle_v1 *handle);
    static void cosmicHandleTitle(void *data, zcosmic_toplevel_handle_v1 *handle,
                                  const char *title);
    static void cosmicHandleAppId(void *data, zcosmic_toplevel_handle_v1 *handle,
                                  const char *appId);
    static void cosmicHandleOutputEnter(void *data, zcosmic_toplevel_handle_v1 *handle,
                                        wl_output *output);
    static void cosmicHandleOutputLeave(void *data, zcosmic_toplevel_handle_v1 *handle,
                                        wl_output *output);
    static void cosmicHandleWorkspaceEnter(void *data, zcosmic_toplevel_handle_v1 *handle,
                                           zcosmic_workspace_handle_v1 *workspace);
    static void cosmicHandleWorkspaceLeave(void *data, zcosmic_toplevel_handle_v1 *handle,
                                           zcosmic_workspace_handle_v1 *workspace);
    static void cosmicHandleState(void *data, zcosmic_toplevel_handle_v1 *handle,
                                  wl_array *state);

    static void wlrToplevel(void *data, zwlr_foreign_toplevel_manager_v1 *manager,
                            zwlr_foreign_toplevel_handle_v1 *handle);
    static void wlrFinished(void *data, zwlr_foreign_toplevel_manager_v1 *manager);

    static void wlrHandleTitle(void *data, zwlr_foreign_toplevel_handle_v1 *handle,
                               const char *title);
    static void wlrHandleAppId(void *data, zwlr_foreign_toplevel_handle_v1 *handle,
                               const char *appId);
    static void wlrHandleOutputEnter(void *data, zwlr_foreign_toplevel_handle_v1 *handle,
                                     wl_output *output);
    static void wlrHandleOutputLeave(void *data, zwlr_foreign_toplevel_handle_v1 *handle,
                                     wl_output *output);
    static void wlrHandleState(void *data, zwlr_foreign_toplevel_handle_v1 *handle,
                               wl_array *state);
    static void wlrHandleDone(void *data, zwlr_foreign_toplevel_handle_v1 *handle);
    static void wlrHandleClosed(void *data, zwlr_foreign_toplevel_handle_v1 *handle);

    WaylandToplevelTracker *q = nullptr;
    wl_display *display = nullptr;
    wl_registry *registry = nullptr;
    QSocketNotifier *notifier = nullptr;

    uint32_t cosmicName = 0;
    uint32_t cosmicVersion = 0;
    uint32_t wlrName = 0;
    uint32_t wlrVersion = 0;

    zcosmic_toplevel_info_v1 *cosmicInfo = nullptr;
    zwlr_foreign_toplevel_manager_v1 *wlrManager = nullptr;

    QHash<void *, ToplevelData> toplevels;
    void *activeHandle = nullptr;
    bool available = false;
};

bool WaylandToplevelTracker::Private::init()
{
    if (!isWaylandSession()) {
        return false;
    }

    display = wl_display_connect(nullptr);
    if (!display) {
        return false;
    }

    registry = wl_display_get_registry(display);
    if (!registry) {
        teardown();
        return false;
    }

    static const wl_registry_listener registryListener = {
        &Private::registryGlobal,
        &Private::registryGlobalRemove
    };
    wl_registry_add_listener(registry, &registryListener, this);

    if (wl_display_roundtrip(display) < 0) {
        teardown();
        return false;
    }

    if (cosmicName != 0) {
        const uint32_t version = std::min<uint32_t>(cosmicVersion, 1);
        cosmicInfo = static_cast<zcosmic_toplevel_info_v1 *>(
            wl_registry_bind(registry, cosmicName, &zcosmic_toplevel_info_v1_interface, version));
        if (cosmicInfo) {
            static const zcosmic_toplevel_info_v1_listener listener = {
                &Private::cosmicToplevel,
                &Private::cosmicFinished,
                nullptr
            };
            zcosmic_toplevel_info_v1_add_listener(cosmicInfo, &listener, this);
        }
    } else if (wlrName != 0) {
        const uint32_t version = std::min<uint32_t>(wlrVersion, 1);
        wlrManager = static_cast<zwlr_foreign_toplevel_manager_v1 *>(
            wl_registry_bind(registry, wlrName, &zwlr_foreign_toplevel_manager_v1_interface,
                             version));
        if (wlrManager) {
            static const zwlr_foreign_toplevel_manager_v1_listener listener = {
                &Private::wlrToplevel,
                &Private::wlrFinished
            };
            zwlr_foreign_toplevel_manager_v1_add_listener(wlrManager, &listener, this);
        }
    }

    if (!cosmicInfo && !wlrManager) {
        teardown();
        return false;
    }

    if (wl_display_roundtrip(display) < 0) {
        teardown();
        return false;
    }

    notifier = new QSocketNotifier(wl_display_get_fd(display), QSocketNotifier::Read, q);
    QObject::connect(notifier, &QSocketNotifier::activated, q, [this]() { onReadable(); });

    available = true;
    return true;
}

void WaylandToplevelTracker::Private::teardown()
{
    if (notifier) {
        notifier->setEnabled(false);
        delete notifier;
        notifier = nullptr;
    }

    for (auto it = toplevels.cbegin(); it != toplevels.cend(); ++it) {
        const ToplevelData data = it.value();
        void *handle = it.key();
        if (data.cosmic) {
            zcosmic_toplevel_handle_v1_destroy(
                static_cast<zcosmic_toplevel_handle_v1 *>(handle));
        } else if (data.wlr) {
            zwlr_foreign_toplevel_handle_v1_destroy(
                static_cast<zwlr_foreign_toplevel_handle_v1 *>(handle));
        }
    }
    toplevels.clear();
    activeHandle = nullptr;

    if (cosmicInfo) {
        zcosmic_toplevel_info_v1_stop(cosmicInfo);
        wl_proxy_destroy(reinterpret_cast<wl_proxy *>(cosmicInfo));
        cosmicInfo = nullptr;
    }

    if (wlrManager) {
        zwlr_foreign_toplevel_manager_v1_stop(wlrManager);
        wl_proxy_destroy(reinterpret_cast<wl_proxy *>(wlrManager));
        wlrManager = nullptr;
    }

    if (display) {
        wl_display_flush(display);
        wl_display_disconnect(display);
        display = nullptr;
    }

    registry = nullptr;
    available = false;
}

void WaylandToplevelTracker::Private::removeToplevel(void *handle)
{
    toplevels.remove(handle);
    if (activeHandle == handle) {
        activeHandle = nullptr;
    }
}

QString WaylandToplevelTracker::Private::activeWindowInfo() const
{
    if (!available || !activeHandle) {
        return QString();
    }

    const auto it = toplevels.constFind(activeHandle);
    if (it == toplevels.constEnd()) {
        return QString();
    }

    QString appId = it->appId.trimmed();
    QString title = it->title.trimmed();

    if (appId.isEmpty()) {
        appId = title;
    }
    if (appId.isEmpty()) {
        appId = QStringLiteral("Unknown");
    }
    if (title.isEmpty()) {
        title = appId;
    }

    return proxyWindowId(activeHandle) + QLatin1Char('|') + appId + QLatin1Char('|') + title;
}

void WaylandToplevelTracker::Private::onReadable()
{
    if (!display) {
        return;
    }

    while (wl_display_prepare_read(display) != 0) {
        if (wl_display_dispatch_pending(display) < 0) {
            available = false;
            if (notifier) {
                notifier->setEnabled(false);
            }
            return;
        }
    }

    wl_display_flush(display);

    if (wl_display_read_events(display) < 0) {
        wl_display_cancel_read(display);
        available = false;
        if (notifier) {
            notifier->setEnabled(false);
        }
        return;
    }

    if (wl_display_dispatch_pending(display) < 0) {
        available = false;
        if (notifier) {
            notifier->setEnabled(false);
        }
    }
}

void WaylandToplevelTracker::Private::registryGlobal(void *data, wl_registry *registry,
                                                     uint32_t name, const char *interface,
                                                     uint32_t version)
{
    Q_UNUSED(registry);
    auto *self = static_cast<Private *>(data);
    if (!self || !interface) {
        return;
    }

    if (std::strcmp(interface, zcosmic_toplevel_info_v1_interface.name) == 0) {
        self->cosmicName = name;
        self->cosmicVersion = version;
    } else if (std::strcmp(interface, zwlr_foreign_toplevel_manager_v1_interface.name) == 0) {
        self->wlrName = name;
        self->wlrVersion = version;
    }
}

void WaylandToplevelTracker::Private::registryGlobalRemove(void *data, wl_registry *registry,
                                                           uint32_t name)
{
    Q_UNUSED(data);
    Q_UNUSED(registry);
    Q_UNUSED(name);
}

void WaylandToplevelTracker::Private::cosmicToplevel(void *data, zcosmic_toplevel_info_v1 *info,
                                                     zcosmic_toplevel_handle_v1 *handle)
{
    Q_UNUSED(info);
    auto *self = static_cast<Private *>(data);
    if (!self || !handle) {
        return;
    }

    ToplevelData item;
    item.cosmic = true;
    self->toplevels.insert(handle, item);

    static const zcosmic_toplevel_handle_v1_listener listener = {
        &Private::cosmicHandleClosed,
        &Private::cosmicHandleDone,
        &Private::cosmicHandleTitle,
        &Private::cosmicHandleAppId,
        &Private::cosmicHandleOutputEnter,
        &Private::cosmicHandleOutputLeave,
        &Private::cosmicHandleWorkspaceEnter,
        &Private::cosmicHandleWorkspaceLeave,
        &Private::cosmicHandleState,
        nullptr,
        nullptr,
        nullptr
    };
    zcosmic_toplevel_handle_v1_add_listener(handle, &listener, self);
}

void WaylandToplevelTracker::Private::cosmicFinished(void *data, zcosmic_toplevel_info_v1 *info)
{
    auto *self = static_cast<Private *>(data);
    if (!self) {
        return;
    }

    self->available = false;
    if (self->cosmicInfo == info) {
        self->cosmicInfo = nullptr;
        wl_proxy_destroy(reinterpret_cast<wl_proxy *>(info));
    }
    if (self->notifier) {
        self->notifier->setEnabled(false);
    }
}

void WaylandToplevelTracker::Private::cosmicHandleClosed(void *data,
                                                         zcosmic_toplevel_handle_v1 *handle)
{
    auto *self = static_cast<Private *>(data);
    if (!self || !handle) {
        return;
    }
    self->removeToplevel(handle);
    zcosmic_toplevel_handle_v1_destroy(handle);
}

void WaylandToplevelTracker::Private::cosmicHandleDone(void *data,
                                                       zcosmic_toplevel_handle_v1 *handle)
{
    Q_UNUSED(data);
    Q_UNUSED(handle);
}

void WaylandToplevelTracker::Private::cosmicHandleTitle(void *data,
                                                        zcosmic_toplevel_handle_v1 *handle,
                                                        const char *title)
{
    auto *self = static_cast<Private *>(data);
    if (!self) {
        return;
    }
    auto it = self->toplevels.find(handle);
    if (it != self->toplevels.end()) {
        it->title = QString::fromUtf8(title ? title : "");
    }
}

void WaylandToplevelTracker::Private::cosmicHandleAppId(void *data,
                                                        zcosmic_toplevel_handle_v1 *handle,
                                                        const char *appId)
{
    auto *self = static_cast<Private *>(data);
    if (!self) {
        return;
    }
    auto it = self->toplevels.find(handle);
    if (it != self->toplevels.end()) {
        it->appId = QString::fromUtf8(appId ? appId : "");
    }
}

void WaylandToplevelTracker::Private::cosmicHandleOutputEnter(
    void *data, zcosmic_toplevel_handle_v1 *handle, wl_output *output)
{
    Q_UNUSED(data);
    Q_UNUSED(handle);
    Q_UNUSED(output);
}

void WaylandToplevelTracker::Private::cosmicHandleOutputLeave(
    void *data, zcosmic_toplevel_handle_v1 *handle, wl_output *output)
{
    Q_UNUSED(data);
    Q_UNUSED(handle);
    Q_UNUSED(output);
}

void WaylandToplevelTracker::Private::cosmicHandleWorkspaceEnter(
    void *data, zcosmic_toplevel_handle_v1 *handle, zcosmic_workspace_handle_v1 *workspace)
{
    Q_UNUSED(data);
    Q_UNUSED(handle);
    Q_UNUSED(workspace);
}

void WaylandToplevelTracker::Private::cosmicHandleWorkspaceLeave(
    void *data, zcosmic_toplevel_handle_v1 *handle, zcosmic_workspace_handle_v1 *workspace)
{
    Q_UNUSED(data);
    Q_UNUSED(handle);
    Q_UNUSED(workspace);
}

void WaylandToplevelTracker::Private::cosmicHandleState(void *data,
                                                        zcosmic_toplevel_handle_v1 *handle,
                                                        wl_array *state)
{
    auto *self = static_cast<Private *>(data);
    if (!self || !handle || !state) {
        return;
    }

    bool activated = false;
    const char *bytes = static_cast<const char *>(state->data);
    for (size_t offset = 0; offset + sizeof(uint32_t) <= state->size; offset += sizeof(uint32_t)) {
        uint32_t value = 0;
        std::memcpy(&value, bytes + offset, sizeof(value));
        if (value == ZCOSMIC_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED) {
            activated = true;
            break;
        }
    }

    auto it = self->toplevels.find(handle);
    if (it != self->toplevels.end()) {
        it->activated = activated;
    }

    if (activated) {
        self->activeHandle = handle;
    } else if (self->activeHandle == handle) {
        self->activeHandle = nullptr;
    }
}

void WaylandToplevelTracker::Private::wlrToplevel(
    void *data, zwlr_foreign_toplevel_manager_v1 *manager,
    zwlr_foreign_toplevel_handle_v1 *handle)
{
    Q_UNUSED(manager);
    auto *self = static_cast<Private *>(data);
    if (!self || !handle) {
        return;
    }

    ToplevelData item;
    item.wlr = true;
    self->toplevels.insert(handle, item);

    static const zwlr_foreign_toplevel_handle_v1_listener listener = {
        &Private::wlrHandleTitle,
        &Private::wlrHandleAppId,
        &Private::wlrHandleOutputEnter,
        &Private::wlrHandleOutputLeave,
        &Private::wlrHandleState,
        &Private::wlrHandleDone,
        &Private::wlrHandleClosed,
        nullptr
    };
    zwlr_foreign_toplevel_handle_v1_add_listener(handle, &listener, self);
}

void WaylandToplevelTracker::Private::wlrFinished(void *data,
                                                  zwlr_foreign_toplevel_manager_v1 *manager)
{
    auto *self = static_cast<Private *>(data);
    if (!self) {
        return;
    }

    self->available = false;
    if (self->wlrManager == manager) {
        self->wlrManager = nullptr;
        wl_proxy_destroy(reinterpret_cast<wl_proxy *>(manager));
    }
    if (self->notifier) {
        self->notifier->setEnabled(false);
    }
}

void WaylandToplevelTracker::Private::wlrHandleTitle(void *data,
                                                     zwlr_foreign_toplevel_handle_v1 *handle,
                                                     const char *title)
{
    auto *self = static_cast<Private *>(data);
    if (!self) {
        return;
    }
    auto it = self->toplevels.find(handle);
    if (it != self->toplevels.end()) {
        it->title = QString::fromUtf8(title ? title : "");
    }
}

void WaylandToplevelTracker::Private::wlrHandleAppId(void *data,
                                                     zwlr_foreign_toplevel_handle_v1 *handle,
                                                     const char *appId)
{
    auto *self = static_cast<Private *>(data);
    if (!self) {
        return;
    }
    auto it = self->toplevels.find(handle);
    if (it != self->toplevels.end()) {
        it->appId = QString::fromUtf8(appId ? appId : "");
    }
}

void WaylandToplevelTracker::Private::wlrHandleOutputEnter(
    void *data, zwlr_foreign_toplevel_handle_v1 *handle, wl_output *output)
{
    Q_UNUSED(data);
    Q_UNUSED(handle);
    Q_UNUSED(output);
}

void WaylandToplevelTracker::Private::wlrHandleOutputLeave(
    void *data, zwlr_foreign_toplevel_handle_v1 *handle, wl_output *output)
{
    Q_UNUSED(data);
    Q_UNUSED(handle);
    Q_UNUSED(output);
}

void WaylandToplevelTracker::Private::wlrHandleState(void *data,
                                                     zwlr_foreign_toplevel_handle_v1 *handle,
                                                     wl_array *state)
{
    auto *self = static_cast<Private *>(data);
    if (!self || !handle || !state) {
        return;
    }

    bool activated = false;
    const char *bytes = static_cast<const char *>(state->data);
    for (size_t offset = 0; offset + sizeof(uint32_t) <= state->size; offset += sizeof(uint32_t)) {
        uint32_t value = 0;
        std::memcpy(&value, bytes + offset, sizeof(value));
        if (value == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED) {
            activated = true;
            break;
        }
    }

    auto it = self->toplevels.find(handle);
    if (it != self->toplevels.end()) {
        it->activated = activated;
    }

    if (activated) {
        self->activeHandle = handle;
    } else if (self->activeHandle == handle) {
        self->activeHandle = nullptr;
    }
}

void WaylandToplevelTracker::Private::wlrHandleDone(void *data,
                                                    zwlr_foreign_toplevel_handle_v1 *handle)
{
    Q_UNUSED(data);
    Q_UNUSED(handle);
}

void WaylandToplevelTracker::Private::wlrHandleClosed(void *data,
                                                      zwlr_foreign_toplevel_handle_v1 *handle)
{
    auto *self = static_cast<Private *>(data);
    if (!self || !handle) {
        return;
    }
    self->removeToplevel(handle);
    zwlr_foreign_toplevel_handle_v1_destroy(handle);
}

#endif // Q_OS_LINUX && HAVE_WAYLAND_CLIENT

WaylandToplevelTracker::WaylandToplevelTracker(QObject *parent)
    : QObject(parent)
{
#if defined(Q_OS_LINUX) && defined(HAVE_WAYLAND_CLIENT)
    d = new Private(this);
    if (!d->init()) {
        delete d;
        d = nullptr;
    }
#endif
}

WaylandToplevelTracker::~WaylandToplevelTracker()
{
#if defined(Q_OS_LINUX) && defined(HAVE_WAYLAND_CLIENT)
    delete d;
    d = nullptr;
#endif
}

bool WaylandToplevelTracker::isAvailable() const
{
#if defined(Q_OS_LINUX) && defined(HAVE_WAYLAND_CLIENT)
    return d && d->available;
#else
    return false;
#endif
}

QString WaylandToplevelTracker::activeWindowInfo() const
{
#if defined(Q_OS_LINUX) && defined(HAVE_WAYLAND_CLIENT)
    return d ? d->activeWindowInfo() : QString();
#else
    return QString();
#endif
}
