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
    height: appSettings.isFirstRun ? 660 : 720
    Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
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
        trayIcon.visible = false;
        quitTimer.start();
    }

    Timer {
        id: quitTimer
        interval: 150
        repeat: false
        onTriggered: Qt.exit(0);
    }

    Shortcut {
        sequence: "Ctrl+Q"
        onActivated: mainWindow.quitApp()
    }

    Settings {
        id: appSettings
        category: "timer"
        property bool isFirstRun: true
        property int workMinutes: 20
        property int breakSeconds: 20
        property int idleMinutes: 5 
        property bool soundEnabled: true
        property bool smartPause: true
        property bool isDarkTheme: true
        property bool autostartMinimized: false
        property bool autostartStartTimer: false
    }

    Connections {
        target: appSettings
        function onAutostartMinimizedChanged() { autostartManager.startMinimized = appSettings.autostartMinimized; }
        function onAutostartStartTimerChanged() { autostartManager.startTimer = appSettings.autostartStartTimer; }
        function onIdleMinutesChanged() { idleMonitor.idleThresholdMs = appSettings.idleMinutes * 60000; }
    }

    Component.onCompleted: {
        autostartManager.startMinimized = appSettings.autostartMinimized;
        autostartManager.startTimer = appSettings.autostartStartTimer;
        idleMonitor.idleThresholdMs = appSettings.idleMinutes * 60000;
        
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
        if (appSettings.soundEnabled) notificationSound.play();
    }

    Connections {
        target: idleMonitor
        function onIsIdleChanged() {
            if (!appSettings.smartPause) return;
            
            if (idleMonitor.isIdle) {
                if (startButton.isActive && !startButton.isResting) {
                    countTimer.stop();
                    startButton.remainingTime = appSettings.workMinutes * 60; 
                }
            } else {
                if (startButton.isActive && !startButton.isResting) {
                    countTimer.start();
                }
            }
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

            Image {
                id: appLogo
                source: "logo.png" 
                anchors.bottom: startButton.top
                anchors.bottomMargin: 30
                anchors.horizontalCenter: parent.horizontalCenter
                width: 220 
                fillMode: Image.PreserveAspectFit
                
                layer.enabled: true
                layer.effect: MultiEffect {
                    shadowEnabled: true
                    shadowColor: mainWindow.isDarkTheme ? "#80000000" : "#20000000"
                    shadowBlur: 15
                }
            }

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
                anchors.verticalCenterOffset: 30 
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

                            // --- НОВИЙ БЛОК: Вибір мови ---
                            CenteredSetting {
                                text: qsTr("Мова / Language")
                                ComboBox {
                                    model: [
                                        { text: "Українська", value: "uk" },
                                        { text: "English", value: "en" }
                                    ]
                                    textRole: "text"
                                    valueRole: "value"
                                    
                                    // Якщо langManager ще не підключений у C++, ставимо захист, щоб QML не крашився
                                    currentIndex: (typeof langManager !== "undefined" && langManager.currentLanguage === "en") ? 1 : 0
                                    
                                    onActivated: {
                                        if (typeof langManager !== "undefined") {
                                            langManager.currentLanguage = model[index].value
                                        }
                                    }

                                    // Стилізація ComboBox під тему
                                    background: Rectangle {
                                        color: mainWindow.isDarkTheme ? "#0f172a" : "#f8fafc"
                                        radius: 8
                                        border.color: mainWindow.isDarkTheme ? "#334155" : "#e2e8f0"
                                        implicitWidth: 160
                                        implicitHeight: 40
                                    }
                                    contentItem: Text {
                                        text: parent.displayText
                                        color: mainWindow.isDarkTheme ? "white" : "#0f172a"
                                        font.pixelSize: 14
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                }
                            }

                            Rectangle { Layout.alignment: Qt.AlignHCenter; width: 150; height: 1; color: mainWindow.isDarkTheme ? "#334155" : "#e2e8f0" }

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
                                    onToggled: appSettings.isDarkTheme = checked
                                }
                            }

                            Rectangle { Layout.alignment: Qt.AlignHCenter; width: 150; height: 1; color: mainWindow.isDarkTheme ? "#334155" : "#e2e8f0" }

                            CenteredSetting {
                                text: qsTr("Звукові сповіщення")
                                Switch {
                                    checked: appSettings.soundEnabled
                                    onToggled: appSettings.soundEnabled = checked
                                }
                            }

                            Rectangle { Layout.alignment: Qt.AlignHCenter; width: 150; height: 1; color: mainWindow.isDarkTheme ? "#334155" : "#e2e8f0" }

                            CenteredSetting {
                                text: qsTr("Автоскидання таймера, якщо я відійшов")
                                Switch {
                                    checked: appSettings.smartPause
                                    onToggled: appSettings.smartPause = checked
                                }
                            }
                            
                            CenteredSetting {
                                text: qsTr("└ Час відсутності для скидання (хвилин)")
                                visible: appSettings.smartPause 
                                SpinBox {
                                    from: 1; to: 60
                                    value: appSettings.idleMinutes
                                    onValueModified: appSettings.idleMinutes = value
                                }
                            }
        property bool isFirstRun: true
                            Rectangle { Layout.alignment: Qt.AlignHCenter; width: 150; height: 1; color: mainWindow.isDarkTheme ? "#334155" : "#e2e8f0" }

                            CenteredSetting {
                                text: qsTr("Автозапуск з системою")
                                Switch {
                                    checked: autostartManager.enabled
                                    onToggled: autostartManager.enabled = checked
                                }
                            }

                            CenteredSetting {
                                text: qsTr("└ Запускати згорнутим у трей")
                                visible: autostartManager.enabled
                                Switch {
                                    checked: appSettings.autostartMinimized
                                    onToggled: appSettings.autostartMinimized = checked
                                }
                            }

                            CenteredSetting {
                                text: qsTr("└ Одразу запускати таймер")
                                visible: autostartManager.enabled
                                Switch {
                                    checked: appSettings.autostartStartTimer
                                    onToggled: appSettings.autostartStartTimer = checked
                                }
                            }
                        }
                    }

                    // --- ІНФОРМАЦІЯ ПРО ПРОГРАМУ (ФУТЕР) ---
                    ColumnLayout {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.topMargin: 20
                        spacing: 8

                        Text {
                            text: "Eyespoly v1.0.0"
                            color: mainWindow.isDarkTheme ? "#94a3b8" : "#64748b"
                            font.pixelSize: 14
                            font.bold: true
                            Layout.alignment: Qt.AlignHCenter
                        }

                        Text {
                            text: qsTr("Розробка: ") + "<a href='https://github.com/andriyco13'>andriyco13</a>"
                            color: mainWindow.isDarkTheme ? "#64748b" : "#94a3b8"
                            linkColor: "#3b82f6"
                            font.pixelSize: 13
                            Layout.alignment: Qt.AlignHCenter
                            onLinkActivated: function(link) { Qt.openUrlExternally(link) }
                            
                            HoverHandler {
                                cursorShape: Qt.PointingHandCursor
                            }
                        }

                        Text {
                            text: qsTr("Дизайн логотипу: minzuxx")
                            color: mainWindow.isDarkTheme ? "#64748b" : "#94a3b8"
                            font.pixelSize: 13
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }

                    Item { Layout.fillWidth: true; height: 120 }
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

    // --- 4. ВІКНО ПЕРШОГО ЗАПУСКУ (ВІТАЛЬНИЙ ЕКРАН) ---
  Item {
        id: welcomeScreen
        anchors.fill: parent
        visible: appSettings.isFirstRun
        z: 999

        Rectangle {
            anchors.fill: parent
            color: mainWindow.isDarkTheme ? "#0f172a" : "#f1f5f9"
            Behavior on color { ColorAnimation { duration: 300 } }
        }

        ColumnLayout {
            id: welcomeColumn
            width: Math.min(parent.width - 60, 400)
            anchors.centerIn: parent
            spacing: 16

            Image {
                source: "logo.png"
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 140
                fillMode: Image.PreserveAspectFit
                layer.enabled: true
                layer.effect: MultiEffect {
                    shadowEnabled: true
                    shadowColor: mainWindow.isDarkTheme ? "#80000000" : "#20000000"
                    shadowBlur: 15
                }
            }

                ColumnLayout {
                    spacing: 8
                    Layout.fillWidth: true
                    
                    Text {
                        text: qsTr("Вітаємо в Eyespoly!")
                        font.pixelSize: 26
                        font.bold: true
                        color: mainWindow.isDarkTheme ? "white" : "#0f172a"
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: qsTr("Ця програма допоможе зберегти ваш зір, нагадуючи про регулярні перерви під час роботи за комп'ютером.")
                        font.pixelSize: 14
                        color: mainWindow.isDarkTheme ? "#cbd5e1" : "#475569"
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                        lineHeight: 1.2
                        Layout.fillWidth: true
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    
                    Text {
                        text: qsTr("Оберіть мову / Choose language")
                        font.pixelSize: 15
                        font.bold: true
                        color: mainWindow.isDarkTheme ? "white" : "#1e293b"
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                    }
                    
                    ComboBox {
                        Layout.alignment: Qt.AlignHCenter
                        model: [
                            { text: "Українська", value: "uk" },
                            { text: "English", value: "en" }
                        ]
                        textRole: "text"
                        valueRole: "value"
                        currentIndex: (typeof langManager !== "undefined" && langManager.currentLanguage === "en") ? 1 : 0
                        
                        onActivated: {
                            if (typeof langManager !== "undefined") {
                                langManager.currentLanguage = model[index].value
                            }
                        }

                        background: Rectangle {
                            color: mainWindow.isDarkTheme ? "#1e293b" : "#ffffff"
                            radius: 8
                            border.color: mainWindow.isDarkTheme ? "#334155" : "#e2e8f0"
                            implicitWidth: 180
                            implicitHeight: 40
                        }
                        contentItem: Text {
                            text: parent.displayText
                            color: mainWindow.isDarkTheme ? "white" : "#0f172a"
                            font.pixelSize: 14
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    
                    Text {
                        text: qsTr("Темна тема / Dark theme")
                        font.pixelSize: 15
                        font.bold: true
                        color: mainWindow.isDarkTheme ? "white" : "#1e293b"
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                    }
                    
                    Switch {
                        Layout.alignment: Qt.AlignHCenter
                        checked: appSettings.isDarkTheme
                        onToggled: appSettings.isDarkTheme = checked
                    }
                }

                Button {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 6
                    Layout.bottomMargin: 6
                    Layout.preferredWidth: 200
                    Layout.preferredHeight: 45

                    contentItem: Text {
                        text: qsTr("Почати")
                        font.pixelSize: 16
                        font.bold: true
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: 22.5
                        color: "#3b82f6"
                        layer.enabled: true
                        layer.effect: MultiEffect {
                            shadowEnabled: true
                            shadowColor: mainWindow.isDarkTheme ? "#80000000" : "#30000000"
                            shadowBlur: 20
                        }
                    }

                    onClicked: {
                        appSettings.isFirstRun = false;
                    }
                }
        }
  }
}