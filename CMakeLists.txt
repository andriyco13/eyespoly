cmake_minimum_required(VERSION 3.16)
project(Eyespoly VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

# --- Qt-компоненти: DBus потрібен лише на Linux (там і GNOME Mutter
#     IdleMonitor, і виклики через gdbus), на Windows/macOS цей модуль
#     не використовується і не лінкується. ---
set(EYESPOLY_QT_COMPONENTS Core Gui Quick Qml Multimedia Sql LinguistTools QuickControls2 Widgets Network)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND EYESPOLY_QT_COMPONENTS DBus)
endif()

find_package(Qt6 REQUIRED COMPONENTS ${EYESPOLY_QT_COMPONENTS})
find_package(Threads REQUIRED)

qt_standard_project_setup()

qt_policy(SET QTP0001 NEW)

# --- Платформозалежні вихідні файли ---
# idlemonitor.* та apptracker.* самі містять #ifdef Q_OS_WIN / Q_OS_LINUX
# усередині і компілюються на обох платформах без змін.
# autostartmanager.* так само: на Windows працює через QSettings/реєстр,
# на Linux — через .desktop файл в автозапуску.
set(EYESPOLY_SOURCES
    main.cpp
    autostartmanager.cpp
    autostartmanager.h
    idlemonitor.cpp
    idlemonitor.h
    apptracker.cpp
    apptracker.h
    languagemanager.cpp
    languagemanager.h
    updatechecker.cpp
    updatechecker.h
)

if(WIN32)
    qt_add_executable(appEyespoly
        WIN32
        ${EYESPOLY_SOURCES}
    )
else()
    qt_add_executable(appEyespoly
        ${EYESPOLY_SOURCES}
    )
endif()

qt_add_qml_module(appEyespoly
    URI Eyespoly
    VERSION 1.0
    QML_FILES
        main.qml
        BreakOverlay.qml
        StatsPage.qml
    RESOURCES
        icon.png
        icon.ico
        logo.png
        notification.wav
)

qt_add_translations(appEyespoly
    TS_FILES
        eyespoly_en.ts
    RESOURCE_PREFIX "/i18n"
)

target_link_libraries(appEyespoly PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Multimedia
    Qt6::Qml
    Qt6::Sql
    Qt6::Widgets
    Qt6::Network
    Threads::Threads
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(appEyespoly PRIVATE Qt6::DBus)
endif()

# --- Системні бібліотеки Windows ---
# user32   -> GetForegroundWindow, GetWindowTextW, GetWindowThreadProcessId,
#             GetLastInputInfo (idlemonitor.cpp, apptracker.cpp)
# psapi    -> GetModuleFileNameExW (apptracker.cpp, визначення процесу
#             активного вікна за PID)
# advapi32 не потрібен напряму: доступ до реєстру для автозапуску
# виконується через QSettings::NativeFormat (autostartmanager.cpp).
if(WIN32)
    target_link_libraries(appEyespoly PRIVATE
        user32
        psapi
    )
endif()

# --- Встановлення для Linux-пакетів (deb / rpm / pkg) ---
set_target_properties(appEyespoly PROPERTIES OUTPUT_NAME "eyespoly")

install(TARGETS appEyespoly
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    install(FILES packaging/linux/eyespoly.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )

    install(FILES icon.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
        RENAME eyespoly.png
    )

    install(FILES packaging/linux/eyespoly.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo
    )
endif()