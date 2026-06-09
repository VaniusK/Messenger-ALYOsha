import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import Messenger 1.0

Popup {
    id: root
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: 380
    height: 600
    modal: true
    dim: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.5) }
    
    background: Rectangle { color: appTheme.bgPanel; radius: 10 }
    
    padding: 0 

    signal logoutConfirmed()

    // Вспомогательная функция для перевода цвета Qt (rgba) в HEX (для сохранения в C++)
    function colorToHex(colorObj) {
        var r = Math.round(colorObj.r * 255).toString(16).padStart(2, '0');
        var g = Math.round(colorObj.g * 255).toString(16).padStart(2, '0');
        var b = Math.round(colorObj.b * 255).toString(16).padStart(2, '0');
        return "#" + r + g + b;
    }

    // Системный диалог выбора цвета
    ColorDialog {
        id: customColorDialog
        title: "Выберите акцентный цвет"
        selectedColor: AppState.accentColor
        
        onAccepted: {
            AppState.accentColor = root.colorToHex(selectedColor)
        }
    }

    contentItem: Item {
        anchors.fill: parent

        // ШАПКА
        RowLayout {
            id: headerRow
            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 15
            
            Text { 
                text: "Настройки"
                color: appTheme.textMain
                font.pixelSize: 18
                font.bold: true
                font.family: "Segoe UI" 
            }

            Item { Layout.fillWidth: true }
            
            Text {
                text: "✕"
                color: appTheme.textHint
                font.pixelSize: 20

                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -10
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.close() 
                }
            }
        }

        // ПРОФИЛЬ
        ColumnLayout {
            id: profileCol
            anchors.top: headerRow.bottom; anchors.topMargin: 20
            anchors.left: parent.left; anchors.right: parent.right
            spacing: 8

            Rectangle {
                width: 90; height: 90; radius: 45; color: appTheme.accent
                Layout.alignment: Qt.AlignHCenter
                Behavior on color { ColorAnimation { duration: 200 } }
                Text { 
                    anchors.centerIn: parent
                    color: "white"
                    font.bold: true
                    font.pixelSize: 36
                    text: AppState.currentUserHandle ? AppState.currentUserHandle.charAt(0).toUpperCase() : "?"
                }
            }

            Text { 
                text: AppState.currentUserHandle
                color: appTheme.textMain
                font.pixelSize: 20; font.bold: true; font.family: "Segoe UI"
                Layout.alignment: Qt.AlignHCenter
            }
        }

        Rectangle { 
            id: divider1
            anchors.top: profileCol.bottom
            anchors.topMargin: 20
            width: parent.width
            height: 8
            color: appTheme.bgPanel 
        }

        // НАСТРОЙКИ ТЕМЫ
        ColumnLayout {
            anchors.top: divider1.bottom; anchors.topMargin: 20
            anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 20
            spacing: 15

            Text { 
                text: "Темы оформления"
                color: appTheme.accent
                font.pixelSize: 14
                font.bold: true
                font.family: "Segoe UI"
                Behavior on color { ColorAnimation { duration: 200 } } 
            }

            // ПРЕСЕТЫ
            RowLayout {
                Layout.fillWidth: true; spacing: 10

                Repeater {
                    model: [
                        { name: "Классика", key: "classic", bg: "#0e1621", bubble: "#18222d" },
                        { name: "Светлая", key: "day", bg: "#f1f2f5", bubble: "#ffffff" },
                        { name: "Тёмная", key: "night", bg: "#000000", bubble: "#1c242f" }
                    ]
                    
                    ColumnLayout {
                        spacing: 8; Layout.fillWidth: true
                        
                        Rectangle {
                            Layout.fillWidth: true; Layout.preferredHeight: 60; radius: 8
                            color: modelData.bg; border.width: 2
                            border.color: AppState.theme === modelData.key ? appTheme.accent : "transparent"
                            Behavior on border.color { ColorAnimation { duration: 200 } }

                            Rectangle { 
                                width: 30; height: 12; radius: 4
                                color: modelData.bubble
                                anchors.left: parent.left; anchors.top: parent.top; anchors.margins: 10 
                            }

                            Rectangle {
                                width: 35; height: 12; radius: 4
                                color: appTheme.accent
                                anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 10
                                Behavior on color { ColorAnimation { duration: 200 } } 
                            }

                            MouseArea { 
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: AppState.theme = modelData.key 
                            }
                        }
                        
                        Text { 
                            text: modelData.name
                            color: appTheme.textMain
                            font.pixelSize: 13
                            font.family: "Segoe UI"
                            Layout.alignment: Qt.AlignHCenter 
                        }
                    }
                }
            }

            // Выбор акцентного цвета
            RowLayout {
                Layout.fillWidth: true; Layout.topMargin: 15
                spacing: Math.floor((parent.width - (8 * 28)) / 7) 
                
                Repeater {
                    // Последний элемент "custom" - открывает диалог
                    model: ["#5eb5f7", "#4fa896", "#f05b5b", "#d87b32", "#a668c9", "#e26b8e", "#6485c2", "custom"]
                    
                    Rectangle {
                        width: 28; height: 28; radius: 14
                        
                        // Если это "custom" и цвет не из стандартного списка, заливаем кружок текущим кастомным цветом
                        property bool isStandardColor: ["#5eb5f7", "#4fa896", "#f05b5b", "#d87b32", "#a668c9", "#e26b8e", "#6485c2"].includes(AppState.accentColor.toLowerCase())
                        color: modelData === "custom" 
                               ? (isStandardColor ? "transparent" : AppState.accentColor) 
                               : modelData

                        border.width: (AppState.accentColor.toLowerCase() === modelData || (modelData === "custom" && !isStandardColor)) ? 3 : 0
                        border.color: AppState.theme === "day" ? "#ffffff" : appTheme.bgPanel

                        // Иконка палитры для кастомного цвета (рисуем только если кастомный цвет еще не выбран)
                        Image {
                            visible: modelData === "custom" && isStandardColor
                            anchors.centerIn: parent
                            width: 28; height: 28; sourceSize: Qt.size(28, 28)
                            source: "qrc:/messenger_client_uri/assets/icons/change_color.svg"
                        }

                        MouseArea { 
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: { 
                                if (modelData !== "custom") {
                                    AppState.accentColor = modelData 
                                } else {
                                    customColorDialog.open()
                                }
                            } 
                        }
                    }
                }
            }
        }

        // КНОПКА ВЫХОДА
        Rectangle {
            anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 20
            height: 44; radius: 8
            color: logoutHover.pressed ? "#a22026" : (logoutHover.containsMouse ? "#d9363e" : "#ff4d4f")
            Behavior on color { ColorAnimation { duration: 150 } }
            
            Text { 
                anchors.centerIn: parent
                text: "Выйти из аккаунта"
                color: "white"
                font.pixelSize: 15; font.family: "Segoe UI"; font.bold: true 
            }
            
            MouseArea {
                id: logoutHover
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: { 
                    root.close()
                    root.logoutConfirmed() 
                }
            }
        }
    }
}
