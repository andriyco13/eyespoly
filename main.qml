import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import Qt.labs.platform
import QtCore
import QtQuick.Effects

Window {
    id: mainWindow
    width: 640
    height: 600
    // ОДРАЗУ ховаємо, якщо був переданий аргумент --minimized
    visible: !argMinimized 
    title: qsTr("Eyespoly")
    
    property bool isDarkTheme: appSettings.isDarkTheme
    
    color: isDarkTheme ? "#0f172a" : "#f1f5f9"
    Behavior on color { ColorAnimation { duration: 300 } }

    onClosing: function(close) {
        close.accepted = false;
        mainWindow.visible = false;
    }

    function quitApp() {
        console.log("[quit] quitApp() викликано");
        trayIcon.visible = false;
        quitTimer.start();
    }

    Timer {
        id: quitTimer
        interval: 150
        repeat: false
        onTriggered: {
            console.log("[quit] quitTimer triggered -> Qt.exit(0)");
            Qt.exit(0);
        }
    }

    Shortcut {
        sequence: "Ctrl+Q"
        onActivated: mainWindow.quitApp()
    }

    Settings {
        id: appSettings
        category: "timer"
        property int workMinutes: 20
        property int breakSeconds: 20
        property bool soundEnabled: true
        property bool smartPause: true
        property bool isDarkTheme: true
        // Зберігаємо нові налаштування
        property bool autostartMinimized: false
        property bool autostartStartTimer: false
    }

    // Синхронізуємо налаштування з бекендом C++
    Connections {
        target: appSettings
        function onAutostartMinimizedChanged() { autostartManager.startMinimized = appSettings.autostartMinimized; }
        function onAutostartStartTimerChanged() { autostartManager.startTimer = appSettings.autostartStartTimer; }
    }

    Component.onCompleted: {
        // Передаємо стартові налаштування бекенду
        autostartManager.startMinimized = appSettings.autostartMinimized;
        autostartManager.startTimer = appSettings.autostartStartTimer;
        
        // Якщо був прапорець автозапуску таймера - запускаємо його одразу
        if (argStartTimer) {
            startButton.isActive = true;
            startButton.remainingTime = appSettings.workMinutes * 60;
            countTimer.start();
        }
    }

    SoundEffect {
        id: notificationSound
        source: "notification.wav"
        volume: 0.8
    }

    function playNotificationSound() {
        if (appSettings.soundEnabled) {
            notificationSound.play();
        }
    }

    Connections {
        target: idleMonitor
        function onIsIdleChanged() {
            if (!appSettings.smartPause) return;
            if (!startButton.isActive || startButton.isResting) return;

            if (idleMonitor.isIdle) countTimer.stop();
            else countTimer.start();
        }
    }

    Connections {
        target: appSettings
        function onWorkMinutesChanged() {
            if (!startButton.isActive && !startButton.isResting) {
                startButton.remainingTime = appSettings.workMinutes * 60;
            }
        }
    }

    Connections {
        target: startButton
        function onIsRestingChanged() {
            if (startButton.isResting) {
                if (!mainWindow.visible) mainWindow.visible = true;
                mainWindow.requestActivate();
                breakOverlay.show();
                breakOverlay.requestActivate();
            } else {
                breakOverlay.hide();
            }
        }
    }

    // --- 1. ГОЛОВНА ОБГОРТКА СТОРІНОК (SWIPE VIEW) ---
    SwipeView {
        id: mainView
        anchors.fill: parent
        currentIndex: 0
        interactive: true

        // --- Вкладка 1: Таймер ---
        Item {
            id: timerPage

            Button {
                id: startButton
                property bool isActive: false
                property bool isResting: false
                property bool oneMinuteWarningShown: false
                property int remainingTime: appSettings.workMinutes * 60

                function formatTime(seconds) {
                    let minutes = Math.floor(seconds / 60);
                    let secs = seconds % 60;
                    return (minutes < 10 ? "0" + minutes : minutes) + ":" + (secs < 10 ? "0" + secs : secs);
                }

                Timer {
                    id: countTimer
                    interval: 1000
                    repeat: true
                    running: false
                    onTriggered: {
                        if (startButton.remainingTime > 0) {
                            startButton.remainingTime -= 1;
                            if (!startButton.isResting && !startButton.oneMinuteWarningShown && startButton.remainingTime === 60) {
                                trayIcon.showMessage(qsTr("Скоро перерва"), qsTr("Час дати очам відпочити за хвилину!"), SystemTrayIcon.Information, 5000);
                                startButton.oneMinuteWarningShown = true;
                            }
                        } else {
                            if (!startButton.isResting) {
                                startButton.isResting = true;
                                startButton.oneMinuteWarningShown = false;
                                startButton.remainingTime = appSettings.breakSeconds;
                                mainWindow.playNotificationSound();
                                trayIcon.showMessage(qsTr("Перерва!"), qsTr("Час дати очам відпочити!"), SystemTrayIcon.Information, 5000);
                            } else {
                                startButton.isResting = false;
                                startButton.oneMinuteWarningShown = false;
                                startButton.remainingTime = appSettings.workMinutes * 60;
                                mainWindow.playNotificationSound();
                                trayIcon.showMessage(qsTr("Перерва завершена"), qsTr("Час повернутися до роботи!"), SystemTrayIcon.Information, 5000);
                            }
                        }
                    }
                }

                anchors.centerIn: parent
                anchors.verticalCenterOffset: -40 
                width: 260
                height: 260

                contentItem: Text {
                    text: startButton.formatTime(startButton.remainingTime)
                    font.pixelSize: 48
                    font.bold: true
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: parent.width / 2
                    color: {
                        if (startButton.isResting) return "#10b981"; 
                        if (startButton.isActive) return "#ef4444"; 
                        return "#3b82f6"; 
                    }
                    Behavior on color { ColorAnimation { duration: 300 } }
                    
                    layer.enabled: true
                    layer.effect: MultiEffect {
                        shadowEnabled: true
                        shadowColor: mainWindow.isDarkTheme ? "#80000000" : "#30000000"
                        shadowBlur: 25
                    }
                }

                onClicked: {
                    if (isActive || isResting) {
                        isActive = false;
                        isResting = false;
                        oneMinuteWarningShown = false;
                        countTimer.stop();
                        remainingTime = appSettings.workMinutes * 60;
                    } else {
                        isActive = true;
                        oneMinuteWarningShown = false;
                        countTimer.start();
                    }
                }
            }

            BreakOverlay {
                id: breakOverlay
                remainingSeconds: startButton.remainingTime
                onSkipRequested: {
                    startButton.isResting = false;
                    startButton.isActive = true;
                    startButton.oneMinuteWarningShown = false;
                    startButton.remainingTime = appSettings.workMinutes * 60;
                }
                onSnoozeRequested: {
                    startButton.isResting = false;
                    startButton.isActive = true;
                    startButton.oneMinuteWarningShown = false;
                    startButton.remainingTime = 5 * 60;
                }
            }
        }

        // --- Вкладка 2: Статистика ---
        StatsPage {
            id: statsPage
        }

        // --- Вкладка 3: Налаштування ---
        Item {
            id: settingsPage
            
            ScrollView {
                anchors.fill: parent
                clip: true
                contentWidth: availableWidth 

                ColumnLayout {
                    anchors.top: parent.top
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.margins: 20
                    width: Math.min(parent.width - 40, 400)
                    spacing: 25

                    Text {
                        text: qsTr("Налаштування")
                        color: mainWindow.isDarkTheme ? "white" : "#0f172a"
                        font.pixelSize: 28
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                        Layout.bottomMargin: 10
                        Layout.topMargin: 20
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: settingsCol.implicitHeight + 40
                        color: mainWindow.isDarkTheme ? "#1e293b" : "#ffffff"
                        radius: 20
                        border.color: mainWindow.isDarkTheme ? "#334155" : "#e2e8f0"
                        
                        layer.enabled: true
                        layer.effect: MultiEffect {
                            shadowEnabled: true
                            shadowColor: mainWindow.isDarkTheme ? "#60000000" : "#15000000"
                            shadowBlur: 20
                        }

                        ColumnLayout {
                            id: settingsCol
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.margins: 20
                            spacing: 25

                            component CenteredSetting : Column {
                                Layout.alignment: Qt.AlignHCenter
                                spacing: 12
                                property alias text: label.text
                                default property alias content: container.data

                                Text {
                                    id: label
                                    color: mainWindow.isDarkTheme ? "white" : "#1e293b"
                                    font.pixelSize: 16
                                    font.bold: true
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                                Item {
                                    id: container
                                    width: childrenRect.width
                                    height: childrenRect.height
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                            }

                            CenteredSetting {
                                text: qsTr("Час роботи (хвилин)")
                                SpinBox {
                                    from: 1; to: 180
                                    value: appSettings.workMinutes
                                    enabled: !startButton.isActive && !startButton.isResting
                                    onValueModified: appSettings.workMinutes = value
                                }
                            }

                            Rectangle { Layout.alignment: Qt.AlignHCenter; width: 150; height: 1; color: mainWindow.isDarkTheme ? "#334155" : "#e2e8f0" }

                            CenteredSetting {
                                text: qsTr("Час перерви (секунд)")
                                SpinBox {
                                    from: 5; to: 600
                                    value: appSettings.breakSeconds
                                    enabled: !startButton.isActive && !startButton.isResting
                                    onValueModified: appSettings.breakSeconds = value
                                }
                            }

                            Rectangle { Layout.alignment: Qt.AlignHCenter; width: 150; height: 1; color: mainWindow.isDarkTheme ? "#334155" : "#e2e8f0" }

                            CenteredSetting {
                                text: qsTr("Темна тема")
                                Switch {
                                    checked: appSettings.isDarkTheme
                                    onCheckedChanged: appSettings.isDarkTheme = checked
                                }
                            }

                            Rectangle { Layout.alignment: Qt.AlignHCenter; width: 150; height: 1; color: mainWindow.isDarkTheme ? "#334155" : "#e2e8f0" }

                            CenteredSetting {
                                text: qsTr("Звукові сповіщення")
                                Switch {
                                    checked: appSettings.soundEnabled
                                    onCheckedChanged: appSettings.soundEnabled = checked
                                }
                            }

                            Rectangle { Layout.alignment: Qt.AlignHCenter; width: 150; height: 1; color: mainWindow.isDarkTheme ? "#334155" : "#e2e8f0" }

                            CenteredSetting {
                                text: qsTr("Розумна пауза (автостоп)")
                                Switch {
                                    checked: appSettings.smartPause
                                    onCheckedChanged: appSettings.smartPause = checked
                                }
                            }

                            Rectangle { Layout.alignment: Qt.AlignHCenter; width: 150; height: 1; color: mainWindow.isDarkTheme ? "#334155" : "#e2e8f0" }

                            // НОВИЙ БЛОК АВТОЗАПУСКУ З ПУНКТАМИ
                            CenteredSetting {
                                text: qsTr("Автозапуск з системою")
                                Switch {
                                    checked: autostartManager.enabled
                                    onCheckedChanged: autostartManager.enabled = checked
                                }
                            }

                            // Додаткові опції автозапуску (видимі лише якщо він увімкнений)
                            CenteredSetting {
                                text: qsTr("└ Запускати згорнутим у трей")
                                visible: autostartManager.enabled
                                Switch {
                                    checked: appSettings.autostartMinimized
                                    onCheckedChanged: appSettings.autostartMinimized = checked
                                }
                            }

                            CenteredSetting {
                                text: qsTr("└ Одразу запускати таймер")
                                visible: autostartManager.enabled
                                Switch {
                                    checked: appSettings.autostartStartTimer
                                    onCheckedChanged: appSettings.autostartStartTimer = checked
                                }
                            }
                        }
                    }

                    Item { Layout.fillWidth: true; height: 100 }
                }
            }
        }
    }

    // --- 2. ПЛАВАЮЧА ПАНЕЛЬ (FLOATING TAB BAR) ---
    Rectangle {
        id: floatingTabBar
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 30
        anchors.horizontalCenter: parent.horizontalCenter
        width: 270
        height: 64
        radius: 32
        color: mainWindow.isDarkTheme ? "#1e40af" : "#2563eb"
        
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: mainWindow.isDarkTheme ? "#90000000" : "#40000000"
            shadowBlur: 20
            shadowVerticalOffset: 6
        }

        Rectangle {
            id: activeIndicator
            width: 50
            height: 50
            radius: 25
            color: "#ef4444" 
            y: (floatingTabBar.height - height) / 2
            x: ((floatingTabBar.width / 3) * mainView.currentIndex) + (((floatingTabBar.width / 3) - width) / 2)
            
            Behavior on x {
                SpringAnimation { spring: 4; damping: 0.3 }
            }
        }

        Row {
            anchors.fill: parent
            
            component TabBtn : MouseArea {
                width: floatingTabBar.width / 3
                height: floatingTabBar.height
                property string iconText: ""
                
                Text {
                    anchors.centerIn: parent
                    text: parent.iconText
                    font.pixelSize: 22
                    color: "white"
                }
            }

            TabBtn { iconText: "⏱"; onClicked: mainView.currentIndex = 0 }
            TabBtn { iconText: "📊"; onClicked: { mainView.currentIndex = 1; statsPage.updateStats(); } }
            TabBtn { iconText: "⚙️"; onClicked: mainView.currentIndex = 2 }
        }
    }

    // --- 3. СИСТЕМНИЙ ТРЕЙ ---
    SystemTrayIcon {
        id: trayIcon
        visible: true
        icon.source: trayIconUrl || "icon.png"
        tooltip: qsTr("Eyespoly")

        menu: Menu {
            MenuItem {
                text: mainWindow.visible ? qsTr("Сховати") : qsTr("Показати")
                onTriggered: mainWindow.visible = !mainWindow.visible;
            }
            MenuSeparator {}
            MenuItem {
                text: qsTr("Вийти")
                onTriggered: mainWindow.quitApp()
            }
        }
        onActivated: mainWindow.visible = !mainWindow.visible;
    }
}