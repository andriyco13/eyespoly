cmake_minimum_required(VERSION 3.16)
project(Eyespoly VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick DBus Qml Multimedia Sql)
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
        notification.wav
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