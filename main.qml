import QtQuick
import QtQuick.Controls
import Qt.labs.platform

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

    Button {
        id: startButton
        property bool isActive: false
        property bool isResting: false
        property int remainingTime: 1200 // 20 хвилин * 60 секунд

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
                } else {
                    if (!startButton.isResting) {
                        // Перехід до відпочинку (20 секунд)
                        startButton.isResting = true;
                        startButton.remainingTime = 20;
                    } else {
                        // Повернення до початкового стану після відпочинку
                        startButton.isActive = false;
                        startButton.isResting = false;
                        startButton.remainingTime = 1200;
                        countTimer.stop();
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
                countTimer.stop();
                remainingTime = 1200;
            } else {
                // Запуск
                isActive = true;
                countTimer.start();
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