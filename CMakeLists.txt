cmake_minimum_required(VERSION 3.16)
project(Eyespoly VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick DBus Qml Multimedia)

qt_standard_project_setup()

# Вимикаємо попередження QTP0001 (вмикаємо нові правила шляхів QML)
qt_policy(SET QTP0001 NEW)

# Називаємо файл програми appEyespoly, щоб не було конфлікту з папкою
qt_add_executable(appEyespoly
    main.cpp
    autostartmanager.cpp
    autostartmanager.h
    idlemonitor.cpp
    idlemonitor.h
)

qt_add_qml_module(appEyespoly
    URI Eyespoly
    VERSION 1.0
    QML_FILES 
       main.qml
       BreakOverlay.qml
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
)