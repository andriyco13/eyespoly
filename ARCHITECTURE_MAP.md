# Eyespoly Architecture Map
- CMakeLists.txt: C++17, Qt6, таргет appEyespoly.
- main.cpp: QApplication, QQmlApplicationEngine, QSystemTrayIcon (trayMenu). Реєструє через setContextProperty: langManager, argMinimized, argStartTimer, autostartManager, idleMonitor, appTracker, updateChecker, trayIcon.
- main.qml: SwipeView (Timer, StatsPage.qml, Settings). Налаштування у Settings { id: appSettings; category: "timer" }. UI налаштувань у settingsCol через CenteredSetting.
- BreakOverlay.qml: Оверлей перерви (керується з main.qml).
