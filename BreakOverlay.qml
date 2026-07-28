import QtQuick
import QtQuick.Controls

// Повноекранне вікно-накладка на час перерви.
// Показується поверх усіх вікон, поки триває перерва.
Window {
    id: overlay

    // Секунди, що лишились до кінця перерви — прив'язуються ззовні
    property int remainingSeconds: 0
    property string currentTip: ""

    property bool isLimitExceeded: false
    property int exceededSeconds: 0

    readonly property var tips: [
        qsTr("Подивіться у вікно на віддалений об'єкт"),
        qsTr("Швидко покліпайте очима протягом 10 секунд"),
        qsTr("Зробіть кругові рухи очима за годинниковою стрілкою і проти неї"),
        qsTr("Заплющіть очі та розслабтесь на кілька секунд"),
        qsTr("Подивіться по черзі вліво, вправо, вгору, вниз"),
        qsTr("Легко помасажуйте повіки кінчиками пальців"),
        qsTr("Сфокусуйтесь на кінчику носа, а потім на далекому предметі"),
        qsTr("Встаньте, потягніться та подивіться вдалину")
    ]

    signal skipRequested()
    signal snoozeRequested()

    // Без рамки, завжди зверху, на весь екран
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"

    Timer {
        id: hideTimer
        interval: 300
        repeat: false
        onTriggered: overlay.visibility = Window.Hidden
    }

    function show() {
        currentTip = tips[Math.floor(Math.random() * tips.length)];
        visibility = Window.FullScreen;
        content.opacity = 1;
    }

    function hide() {
        content.opacity = 0;
        hideTimer.start();
    }

    function formatSeconds(seconds) {
        if (seconds >= 60) {
            const m = Math.floor(seconds / 60);
            const s = seconds % 60;
            return m + qsTr(" хв ") + (s < 10 ? "0" + s : s) + qsTr(" с");
        }
        return seconds + qsTr(" с");
    }

    // Прозорість анімуємо на рівні внутрішнього елемента, а не самого
    // вікна — на GNOME/Wayland прозорість цілого Window не підтримується
    // Qt API і засмічує лог попередженнями "does not support setting
    // window opacity". Прозорість вкладеного Item підтримується завжди.
    Item {
        id: content
        anchors.fill: parent
        opacity: 0

        Behavior on opacity {
            NumberAnimation { duration: 300; easing.type: Easing.InOutQuad }
        }

        Rectangle {
            anchors.fill: parent
            color: "#121212"
            opacity: 0.95
        }

        Column {
            anchors.centerIn: parent
            spacing: 36

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Перерва")
                color: "#a0d9a0"
                font.pixelSize: 28
                font.bold: true
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: overlay.formatSeconds(overlay.remainingSeconds)
                color: "white"
                font.pixelSize: 96
                font.bold: true
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: overlay.currentTip
                color: "#dddddd"
                font.pixelSize: 22
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                width: Math.min(overlay.width * 0.7, 700)
            }

            Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: overlay.isLimitExceeded
            text: qsTr("⚠️ Денний ліміт перевищено на ") + overlay.formatSeconds(overlay.exceededSeconds)
            color: "#ef4444"
            font.pixelSize: 24
            font.bold: true
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 20

                Button {
                    text: qsTr("Пропустити")
                    onClicked: overlay.skipRequested()

                    background: Rectangle {
                        radius: 8
                        color: parent.hovered ? "#ef4444" : "#ef4444"
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    padding: 14
                }

                Button {
                    text: qsTr("Відкласти на 5 хв")
                    onClicked: overlay.snoozeRequested()

                    background: Rectangle {
                        radius: 8
                        color: parent.hovered ? "#3f6fb0" : "#4a90e2"
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    padding: 14
                }
            }
        }
    }
}
