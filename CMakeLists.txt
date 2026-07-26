cmake_minimum_required(VERSION 3.16)
project(Eyespoly VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick DBus Qml Multimedia Sql LinguistTools)
find_package(Threads REQUIRED)

qt_standard_project_setup()

qt_policy(SET QTP0001 NEW)

qt_add_executable(appEyespoly
    main.cpp
    autostartmanager.cpp
    autostartmanager.h
    idlemonitor.cpp
    idlemonitor.h
    apptracker.cpp
    apptracker.h
    languagemanager.cpp 
    languagemanager.h
)

qt_add_qml_module(appEyespoly
    URI Eyespoly
    VERSION 1.0
    QML_FILES 
        main.qml
        BreakOverlay.qml
        StatsPage.qml
    RESOURCES
        icon.png
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
    Qt6::DBus
    Qt6::Multimedia
    Qt6::Qml
    Qt6::Sql
    Threads::Threads
)

# --- Встановлення для Linux-пакетів (deb / rpm / pkg) ---
set_target_properties(appEyespoly PROPERTIES OUTPUT_NAME "eyespoly")

install(TARGETS appEyespoly
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

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
