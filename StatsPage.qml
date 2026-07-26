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
    property var statsData: []
    property int totalScreenTime: 0

    function formatDuration(seconds) {
        if (seconds < 60) return seconds + qsTr(" с");
        let hours = Math.floor(seconds / 3600);
        let minutes = Math.floor((seconds % 3600) / 60);
        let secs = seconds % 60;
        if (hours > 0) return hours + qsTr(" год ") + minutes + qsTr(" хв");
        return minutes + qsTr(" хв ") + secs + qsTr(" с");
    }

    function updateStats() {
        statsPage.statsData = []; 
        let newStats = [];
        let newTotal = 0;

        if (selectedDate === "today") {
            newStats = appTracker.getTodayStats();
            newTotal = appTracker.getTotalScreenTimeForDate(Qt.formatDateTime(new Date(), "yyyy-MM-dd"));
        } else if (selectedDate === "yesterday") {
            let date = new Date();
            date.setDate(date.getDate() - 1);
            let dateStr = Qt.formatDateTime(date, "yyyy-MM-dd");
            newStats = appTracker.getStatsForDate(dateStr);
            newTotal = appTracker.getTotalScreenTimeForDate(dateStr);
        } else if (selectedDate === "last7days") {
            newStats = appTracker.getStatsForLastDays(7);
            for (let i = 0; i < newStats.length; i++) newTotal += newStats[i].duration;
        } else if (selectedDate === "last30days") {
            newStats = appTracker.getStatsForLastDays(30);
            for (let j = 0; j < newStats.length; j++) newTotal += newStats[j].duration;
        } else {
            newStats = appTracker.getStatsForDate(selectedDate);
            newTotal = appTracker.getTotalScreenTimeForDate(selectedDate);
        }

        statsPage.statsData = newStats;
        statsPage.totalScreenTime = newTotal;
    }

    Component.onCompleted: updateStats()
    onSelectedDateChanged: updateStats()
    
    Connections {
        target: appTracker
        function onStatsUpdated() { updateStats() }
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
                    model: [
                        { text: qsTr("Сьогодні"), value: "today" },
                        { text: qsTr("Вчора"), value: "yesterday" },
                        { text: qsTr("Останні 7 днів"), value: "last7days" },
                        { text: qsTr("Останні 30 днів"), value: "last30days" }
                    ]
                    textRole: "text"
                    valueRole: "value"
                    currentIndex: 0
                    onCurrentValueChanged: selectedDate = currentValue;

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
                            model: parent.parent.model
                            delegate: ItemDelegate {
                                width: parent.width
                                text: model.text
                                highlighted: parent.currentIndex === index
                                onClicked: {
                                    parent.currentIndex = index;
                                    parent.parent.popup.close();
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
                        text: qsTr("Загальний екранний час")
                        color: mainWindow.isDarkTheme ? "#94a3b8" : "#64748b"
                        font.pixelSize: 16
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: formatDuration(totalScreenTime)
                        color: mainWindow.isDarkTheme ? "white" : "#0f172a"
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

                    Text {
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
                                height: 45
                                color: mainWindow.isDarkTheme ? (index % 2 === 0 ? "#0f172a" : "#1e293b") : (index % 2 === 0 ? "#f8fafc" : "#ffffff")
                                radius: 6

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 10

                                    Text {
                                        text: (index + 1) + "."
                                        color: "#64748b"
                                        font.pixelSize: 14
                                        font.bold: true
                                        Layout.preferredWidth: 30
                                    }

                                    Rectangle {
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
                                            text: modelData.appName
                                            color: mainWindow.isDarkTheme ? "white" : "#0f172a"
                                            font.pixelSize: 14
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }

                                        Rectangle {
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
                                        text: formatDuration(modelData.duration)
                                        color: mainWindow.isDarkTheme ? "#94a3b8" : "#64748b"
                                        font.pixelSize: 12
                                        Layout.preferredWidth: 80
                                        horizontalAlignment: Text.AlignRight
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}