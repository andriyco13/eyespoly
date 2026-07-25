import QtQuick
import QtQuick.Controls
import QtMultimedia
import Qt.labs.platform
import QtCore

Window {
    id: mainWindow
    width: 640
    height: 480
    visible: true
    title: qsTr("Eyespoly")
    color: "#0f0f0f" // Темний фон під твою концепцію

    onClosing: function(close) {
        // Замість завершення програми — ховаємо вікно в трей
        close.accepted = false;
        mainWindow.visible = false;
    }

    // Спільна функція виходу: спершу ховаємо трей-іконку і даємо
    // системі час коректно від'єднати її від D-Bus (StatusNotifierItem),
    // і лише потім завершуємо застосунок. Без цієї затримки на Linux
    // (GNOME/Wayland) процес може зависати при виході.
    function quitApp() {
        trayIcon.visible = false;
        quitTimer.start();
    }

    Timer {
        id: quitTimer
        interval: 150
        repeat: false
        onTriggered: Qt.quit()
    }

    Shortcut {
        sequence: "Ctrl+Q"
        onActivated: mainWindow.quitApp()
    }

    // Збережені налаштування інтервалів — переживають перезапуск програми
    Settings {
        id: appSettings
        category: "timer"
        property int workMinutes: 20
        property int breakSeconds: 20
        property bool soundEnabled: true
        property bool smartPause: true
    }

    // Панель налаштування тривалості роботи/перерви
    Column {
        id: settingsPanel
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 32
        spacing: 16

        Row {
            spacing: 10
            Text {
                text: qsTr("Робота (хв):")
                color: "white"
                anchors.verticalCenter: parent.verticalCenter
            }
            SpinBox {
                id: workSpinBox
                from: 1
                to: 180
                value: appSettings.workMinutes
                enabled: !startButton.isActive && !startButton.isResting
                onValueModified: appSettings.workMinutes = value
            }
        }

        Row {
            spacing: 10
            Text {
                text: qsTr("Перерва (сек):")
                color: "white"
                anchors.verticalCenter: parent.verticalCenter
            }
            SpinBox {
                id: breakSpinBox
                from: 5
                to: 600
                value: appSettings.breakSeconds
                enabled: !startButton.isActive && !startButton.isResting
                onValueModified: appSettings.breakSeconds = value
            }
        }
    }

    // Звук сповіщення про перерву (старт/кінець)
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

    // Розумна пауза: якщо користувач неактивний понад поріг (5 хв,
    // визначено в IdleMonitor) — ставимо робочий таймер на паузу,
    // щоб не зараховувати час відсутності. При поверненні — продовжуємо.
    Connections {
        target: idleMonitor
        function onIsIdleChanged() {
            if (!appSettings.smartPause) {
                return;
            }
            // Чіпаємо лише активний робочий відлік, не перерву
            if (!startButton.isActive || startButton.isResting) {
                return;
            }

            if (idleMonitor.isIdle) {
                countTimer.stop();
            } else {
                countTimer.start();
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

                    // За 1 хвилину до початку перерви — попереджувальне сповіщення
                    if (!startButton.isResting
                        && !startButton.oneMinuteWarningShown
                        && startButton.remainingTime === 60) {
                        trayIcon.showMessage(
                            qsTr("Скоро перерва"),
                            qsTr("Час дати очам відпочити за хвилину!"),
                            SystemTrayIcon.Information,
                            5000
                        );
                        startButton.oneMinuteWarningShown = true;
                    }
                } else {
                    if (!startButton.isResting) {
                        // Перехід до відпочинку
                        startButton.isResting = true;
                        startButton.oneMinuteWarningShown = false;
                        startButton.remainingTime = appSettings.breakSeconds;

                        mainWindow.playNotificationSound();
                        trayIcon.showMessage(
                            qsTr("Перерва!"),
                            qsTr("Час дати очам відпочити!"),
                            SystemTrayIcon.Information,
                            5000
                        );
                    } else {
                        // Перерва закінчилась — одразу починаємо новий робочий відлік
                        startButton.isResting = false;
                        startButton.oneMinuteWarningShown = false;
                        startButton.remainingTime = appSettings.workMinutes * 60;
                        // isActive лишається true — цикл продовжується без зупинки

                        mainWindow.playNotificationSound();
                        trayIcon.showMessage(
                            qsTr("Перерва завершена"),
                            qsTr("Час повернутися до роботи!"),
                            SystemTrayIcon.Information,
                            5000
                        );
                    }
                }
            }
        }

        anchors.centerIn: parent
        width: 200
        height: 200

        contentItem: Text {
            text: startButton.formatTime(startButton.remainingTime)
            font.pixelSize: 32
            font.bold: true
            color: "white"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            radius: parent.width / 2
            color: {
                if (startButton.isResting) return "#27ae60"; // Зелений під час відпочинку
                if (startButton.isActive) return "#b30000"; // Червоний під час роботи
                return "#4a90e2"; // Синій у режимі очікування
            }
        }

        onClicked: {
            if (isActive || isResting) {
                // Зупинка та скидання
                isActive = false;
                isResting = false;
                oneMinuteWarningShown = false;
                countTimer.stop();
                remainingTime = appSettings.workMinutes * 60;
            } else {
                // Запуск
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
            // Пропустити перерву — одразу почати новий робочий відлік
            startButton.isResting = false;
            startButton.isActive = true;
            startButton.oneMinuteWarningShown = false;
            startButton.remainingTime = appSettings.workMinutes * 60;
        }

        onSnoozeRequested: {
            // Відкласти перерву — відновити робочий таймер ще на 5 хв
            startButton.isResting = false;
            startButton.isActive = true;
            startButton.oneMinuteWarningShown = false;
            startButton.remainingTime = 5 * 60;
        }
    }

    Connections {
        target: startButton
        function onIsRestingChanged() {
            if (startButton.isResting) {
                breakOverlay.show();
            } else {
                breakOverlay.hide();
            }
        }
    }

    SystemTrayIcon {
        id: trayIcon
        visible: true
        icon.source: "icon.png" // без реальної іконки трей може не працювати коректно на деяких ОС
        tooltip: qsTr("Eyespoly")

        menu: Menu {
            MenuItem {
                text: mainWindow.visible ? qsTr("Сховати") : qsTr("Показати")
                onTriggered: {
                    mainWindow.visible = !mainWindow.visible;
                }
            }
            MenuItem {
                text: qsTr("Автозапуск при старті системи")
                checkable: true
                Component.onCompleted: checked = autostartManager.enabled
                onTriggered: {
                    autostartManager.enabled = checked;
                }
            }

            MenuItem {
                text: qsTr("Увімкнути звук")
                checkable: true
                Component.onCompleted: checked = appSettings.soundEnabled
                onTriggered: {
                    appSettings.soundEnabled = checked;
                }
            }

            MenuItem {
                text: qsTr("Розумна пауза (зупиняти таймер, коли я не біля ПК)")
                checkable: true
                Component.onCompleted: checked = appSettings.smartPause
                onTriggered: {
                    appSettings.smartPause = checked;
                }
            }

            MenuSeparator {}

            MenuItem {
                text: qsTr("Вийти")
                onTriggered: {
                    console.log("Вийти натиснуто"); // для перевірки в консолі
                    mainWindow.quitApp();
                }
            }
        }

        onActivated: {
            // Клік по іконці теж перемикає видимість вікна
            mainWindow.visible = !mainWindow.visible;
        }
    }
}
