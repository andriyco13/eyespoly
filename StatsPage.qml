import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

Page {
    id: statsPage
    title: qsTr("Статистика")

    background: Rectangle {
        color: "transparent"
    }

    property string selectedDate: "today"
    property string selectedApp: ""
    property string customDate: ""
    property var statsData: []
    property int totalScreenTime: 0

    function setCustomDate(dateString) {
        if (!dateString || dateString.length < 10) return;
        customDate = dateString;
        selectedDate = "custom_day";
        updateStats();
    }

    function formatDuration(seconds) {
        if (seconds < 60) return seconds + qsTr(" с");
        let hours = Math.floor(seconds / 3600);
        let minutes = Math.floor((seconds % 3600) / 60);
        let secs = seconds % 60;
        if (hours > 0) return hours + qsTr(" год ") + minutes + qsTr(" хв");
        return minutes + qsTr(" хв ") + secs + qsTr(" с");
    }

    function formatShortDate(dateString) {
        if (!dateString || dateString.length < 10) return dateString;
        const date = new Date(dateString + "T00:00:00");
        return date.toLocaleDateString(Qt.locale(), "dd MMM");
    }

    function updateStats() {
        statsPage.statsData = [];
        let newStats = [];
        let newTotal = 0;

        let today = new Date();
        let todayStr = today.getFullYear() + "-" + ("0" + (today.getMonth() + 1)).slice(-2) + "-" + ("0" + today.getDate()).slice(-2);

        let yesterday = new Date();
        yesterday.setDate(yesterday.getDate() - 1);
        let yesterdayStr = yesterday.getFullYear() + "-" + ("0" + (yesterday.getMonth() + 1)).slice(-2) + "-" + ("0" + yesterday.getDate()).slice(-2);

        if (selectedApp && selectedApp.length > 0) {
            newStats = appTracker.getStatsForAppAndRange(selectedApp, selectedDate, customDate);
            for (let i = 0; i < newStats.length; i++) newTotal += newStats[i].duration;
        } else if (selectedDate === "today") {
            newStats = appTracker.getTodayStats();
            newTotal = appTracker.getTotalScreenTimeForDate(todayStr);
        } else if (selectedDate === "yesterday") {
            newStats = appTracker.getStatsForDate(yesterdayStr);
            newTotal = appTracker.getTotalScreenTimeForDate(yesterdayStr);
        } else if (selectedDate === "last7days") {
            newStats = appTracker.getStatsForLastDays(7);
            for (let i = 0; i < newStats.length; i++) newTotal += newStats[i].duration;
        } else if (selectedDate === "last30days") {
            newStats = appTracker.getStatsForLastDays(30);
            for (let j = 0; j < newStats.length; j++) newTotal += newStats[j].duration;
        } else if (selectedDate === "custom_day") {
            newStats = appTracker.getStatsForDate(customDate);
            newTotal = appTracker.getTotalScreenTimeForDate(customDate);
        } else {
            newStats = appTracker.getStatsForDate(selectedDate);
            newTotal = appTracker.getTotalScreenTimeForDate(selectedDate);
        }

        statsPage.statsData = newStats;
        statsPage.totalScreenTime = newTotal;
    }

    Component.onCompleted: updateStats()
    onSelectedDateChanged: updateStats()
    onSelectedAppChanged: updateStats()

    Connections {
        target: appTracker
        function onStatsUpdated() { updateStats() }
    }

    Popup {
        id: customDatePickerPopup
        modal: true
        focus: true
        x: (statsPage.width - width) / 2
        y: (statsPage.height - height) / 2
        width: 360
        height: 380
        background: Rectangle {
            color: mainWindow.isDarkTheme ? "#1e293b" : "#ffffff"
            radius: 16
            border.color: mainWindow.isDarkTheme ? "#334155" : "#e2e8f0"
            border.width: 1
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 14

            Text {
                text: qsTr("Виберіть дату")
                color: mainWindow.isDarkTheme ? "white" : "#0f172a"
                font.pixelSize: 24
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                ComboBox {
                    id: customDayCombo
                    Layout.fillWidth: true
                    model: Array.from({ length: 31 }, (_, i) => i + 1)
                    currentIndex: new Date().getDate() - 1
                    background: Rectangle {
                        color: mainWindow.isDarkTheme ? "#0f172a" : "#f8fafc"
                        radius: 8
                        border.color: mainWindow.isDarkTheme ? "#334155" : "#cbd5e1"
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

                ComboBox {
                    id: customMonthCombo
                    Layout.fillWidth: true
                    model: [qsTr("Січень"), qsTr("Лютий"), qsTr("Березень"), qsTr("Квітень"), qsTr("Травень"), qsTr("Червень"), qsTr("Липень"), qsTr("Серпень"), qsTr("Вересень"), qsTr("Жовтень"), qsTr("Листопад"), qsTr("Грудень")]
                    currentIndex: new Date().getMonth()
                    background: Rectangle {
                        color: mainWindow.isDarkTheme ? "#0f172a" : "#f8fafc"
                        radius: 8
                        border.color: mainWindow.isDarkTheme ? "#334155" : "#cbd5e1"
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

                ComboBox {
                    id: customYearCombo
                    Layout.fillWidth: true
                    model: {
                        let years = [];
                        const currentYear = new Date().getFullYear();
                        for (let y = currentYear - 5; y <= currentYear + 2; y++) {
                            years.push(y);
                        }
                        return years;
                    }
                    currentIndex: 5
                    background: Rectangle {
                        color: mainWindow.isDarkTheme ? "#0f172a" : "#f8fafc"
                        radius: 8
                        border.color: mainWindow.isDarkTheme ? "#334155" : "#cbd5e1"
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

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 12

                Button {
                    text: qsTr("Показати")
                    onClicked: {
                        const year = Number(customYearCombo.currentText);
                        const month = Number(customMonthCombo.currentIndex) + 1;
                        const day = Number(customDayCombo.currentText);
                        const dateString = year + "-" + (String(month).padStart(2, "0")) + "-" + (String(day).padStart(2, "0"));
                        setCustomDate(dateString);
                        customDatePickerPopup.close();
                    }
                    implicitWidth: 140
                    implicitHeight: 40
                    background: Rectangle {
                        color: parent.down ? "#1d4ed8" : "#3b82f6"
                        radius: 8
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "white"
                        font.pixelSize: 14
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Button {
                    text: qsTr("Скасувати")
                    onClicked: customDatePickerPopup.close()
                    implicitWidth: 140
                    implicitHeight: 40
                    background: Rectangle {
                        color: parent.down ? (mainWindow.isDarkTheme ? "#334155" : "#e2e8f0") : (mainWindow.isDarkTheme ? "#1e293b" : "#f8fafc")
                        border.color: mainWindow.isDarkTheme ? "#334155" : "#cbd5e1"
                        border.width: 1
                        radius: 8
                    }
                    contentItem: Text {
                        text: parent.text
                        color: mainWindow.isDarkTheme ? "white" : "#0f172a"
                        font.pixelSize: 14
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            spacing: 20
            width: parent.width

            RowLayout {
                spacing: 15
                Layout.fillWidth: true

                Text {
                    text: qsTr("📊 Статистика використання")
                    color: mainWindow.isDarkTheme ? "white" : "#0f172a"
                    font.pixelSize: 24
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                ComboBox {
                    id: periodComboBox
                    Layout.preferredHeight: 40
                    implicitHeight: 40
                    model: [
                        { text: qsTr("Сьогодні"), value: "today" },
                        { text: qsTr("Вчора"), value: "yesterday" },
                        { text: qsTr("Останні 7 днів"), value: "last7days" },
                        { text: qsTr("Останні 30 днів"), value: "last30days" }
                    ]
                    textRole: "text"
                    valueRole: "value"
                    currentIndex: 0
                    onCurrentValueChanged: {
                        if (selectedDate === "custom_day") {
                            customDate = "";
                        }
                        selectedDate = currentValue;
                    }

                    background: Rectangle {
                        color: mainWindow.isDarkTheme ? "#1e293b" : "#ffffff"
                        radius: 8
                        border.color: mainWindow.isDarkTheme ? "#334155" : "#e2e8f0"
                    }
                    contentItem: Text {
                        text: parent.displayText
                        color: mainWindow.isDarkTheme ? "white" : "#0f172a"
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    popup: Popup {
                        y: parent.height - 5
                        width: parent.width
                        implicitHeight: contentItem.implicitHeight
                        padding: 2
                        background: Rectangle {
                            color: mainWindow.isDarkTheme ? "#1e293b" : "#ffffff"
                            radius: 8
                            border.color: mainWindow.isDarkTheme ? "#334155" : "#e2e8f0"

                            layer.enabled: !mainWindow.isDarkTheme
                            layer.effect: MultiEffect { shadowEnabled: true; shadowBlur: 10; shadowColor: "#20000000" }
                        }
                        contentItem: ListView {
                            clip: true
                            implicitHeight: contentHeight
                            model: periodComboBox.model

                            delegate: ItemDelegate {
                                width: parent.width
                                text: modelData.text
                                highlighted: periodComboBox.currentIndex === index
                                onClicked: {
                                    periodComboBox.currentIndex = index;
                                    periodComboBox.popup.close();
                                }
                                contentItem: Text {
                                    text: parent.text
                                    color: parent.highlighted ? (mainWindow.isDarkTheme ? "#4a90e2" : "#2563eb") : (mainWindow.isDarkTheme ? "white" : "#0f172a")
                                    font.pixelSize: 14
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    color: parent.highlighted ? (mainWindow.isDarkTheme ? "#334155" : "#f1f5f9") : "transparent"
                                    radius: 4
                                }
                            }
                        }
                    }
                }

                Button {
                    id: customDateButton
                    text: "📅"
                    implicitHeight: 40
                    implicitWidth: 52
                    Layout.preferredHeight: 40
                    Layout.preferredWidth: 52
                    onClicked: customDatePickerPopup.open()
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Інший день")
                    background: Rectangle {
                        color: mainWindow.isDarkTheme ? "#1e293b" : "#ffffff"
                        radius: 8
                        border.color: mainWindow.isDarkTheme ? "#334155" : "#e2e8f0"
                    }
                    contentItem: Text {
                        text: parent.text
                        color: mainWindow.isDarkTheme ? "white" : "#0f172a"
                        font.pixelSize: 20
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 120
                color: mainWindow.isDarkTheme ? "#1e293b" : "#ffffff"
                radius: 12
                border.color: mainWindow.isDarkTheme ? "#334155" : "#e2e8f0"

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 8

                    Text {
                        text: selectedApp && selectedApp.length > 0 ? qsTr("Статистика для: %1").arg(selectedApp) : qsTr("Загальний екранний час")
                        color: mainWindow.isDarkTheme ? "#94a3b8" : "#64748b"
                        font.pixelSize: 16
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: formatDuration(totalScreenTime)
                        color: {
                            if (appSettings.dailyLimitMinutes > 0 && selectedDate === "today" && totalScreenTime >= (appSettings.dailyLimitMinutes * 60)) {
                                return "#ef4444";
                            }
                            return mainWindow.isDarkTheme ? "white" : "#0f172a";
                        }
                        font.pixelSize: 36
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(400, statsPage.height - 300)
                color: mainWindow.isDarkTheme ? "#1e293b" : "#ffffff"
                radius: 12
                border.color: mainWindow.isDarkTheme ? "#334155" : "#e2e8f0"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 10

                    RowLayout {
                        visible: selectedApp && selectedApp.length > 0
                        Layout.fillWidth: true
                        spacing: 10

                        Button {
                            text: qsTr("← Назад")
                            onClicked: selectedApp = ""
                            implicitHeight: 32
                            background: Rectangle {
                                color: mainWindow.isDarkTheme ? "#334155" : "#e2e8f0"
                                radius: 8
                            }
                            contentItem: Text {
                                text: parent.text
                                color: mainWindow.isDarkTheme ? "white" : "#0f172a"
                                font.bold: true
                            }
                        }

                        Text {
                            text: qsTr("Деталізація по днях")
                            color: mainWindow.isDarkTheme ? "#94a3b8" : "#64748b"
                            font.pixelSize: 18
                            font.bold: true
                        }
                    }

                    Text {
                        visible: !(selectedApp && selectedApp.length > 0)
                        text: qsTr("Топ програм")
                        color: mainWindow.isDarkTheme ? "#94a3b8" : "#64748b"
                        font.pixelSize: 18
                        font.bold: true
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "transparent"

                        ListView {
                            id: appsListView
                            anchors.fill: parent
                            clip: true
                            model: statsData
                            spacing: 5

                            delegate: Rectangle {
                                width: appsListView.width
                                height: selectedApp && selectedApp.length > 0 ? 42 : 45
                                color: mainWindow.isDarkTheme ? (index % 2 === 0 ? "#0f172a" : "#1e293b") : (index % 2 === 0 ? "#f8fafc" : "#ffffff")
                                radius: 6

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 10

                                    Text {
                                        visible: !(selectedApp && selectedApp.length > 0)
                                        text: (index + 1) + "."
                                        color: "#64748b"
                                        font.pixelSize: 14
                                        font.bold: true
                                        Layout.preferredWidth: 30
                                    }

                                    Rectangle {
                                        visible: !(selectedApp && selectedApp.length > 0)
                                        width: 30
                                        height: 30
                                        radius: 6
                                        color: {
                                            var colors = ["#3b82f6", "#ef4444", "#10b981", "#f59e0b", "#8b5cf6"];
                                            return colors[index % colors.length];
                                        }
                                        Layout.preferredWidth: 30

                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData.appName ? modelData.appName.charAt(0).toUpperCase() : "?"
                                            color: "white"
                                            font.pixelSize: 14
                                            font.bold: true
                                        }
                                    }

                                    ColumnLayout {
                                        spacing: 2
                                        Layout.fillWidth: true

                                        Text {
                                            text: selectedApp && selectedApp.length > 0 ? formatShortDate(modelData.date) : modelData.appName
                                            color: mainWindow.isDarkTheme ? "white" : "#0f172a"
                                            font.pixelSize: 14
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }

                                        Rectangle {
                                            visible: !(selectedApp && selectedApp.length > 0)
                                            id: bgBar
                                            height: 4
                                            Layout.fillWidth: true
                                            color: mainWindow.isDarkTheme ? "#334155" : "#e2e8f0"
                                            radius: 2

                                            Rectangle {
                                                width: statsPage.totalScreenTime > 0 ? bgBar.width * (modelData.duration / statsPage.totalScreenTime) : 0
                                                height: bgBar.height
                                                color: "#3b82f6"
                                                radius: 2
                                            }
                                        }
                                    }

                                    Text {
                                        text: selectedApp && selectedApp.length > 0 ? formatDuration(modelData.duration) : formatDuration(modelData.duration)
                                        color: mainWindow.isDarkTheme ? "#94a3b8" : "#64748b"
                                        font.pixelSize: 12
                                        Layout.preferredWidth: 80
                                        horizontalAlignment: Text.AlignRight
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    enabled: !(selectedApp && selectedApp.length > 0)
                                    onClicked: selectedApp = modelData.appName
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}