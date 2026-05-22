import QtQuick
import QtQuick.Layouts
import Messenger 1.0

Rectangle {
    id: pickerRoot

    property int itemHeight: 44
    property int itemCount: 2

    width: 200
    height: itemHeight * itemCount + 16
    radius: 10
    color: "#242f3d"
    visible: false

    signal photoVideoRequested()
    signal documentRequested()

    function show(x, y) {
        pickerRoot.x = x
        pickerRoot.y = y
        pickerRoot.visible = true
    }

    function hide() {
        pickerRoot.visible = false
    }

    Column {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 0

        Rectangle {
            width: parent.width
            height: pickerRoot.itemHeight
            radius: 6
            color: photoArea.containsMouse ? "#2b3a4a" : "transparent"
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 12
                Image {
                    source: "qrc:/messenger_client_uri/assets/icons/photo.svg"
                    width: 20; height: 20; sourceSize: Qt.size(20, 20)
                }
                Text {
                    text: "Фото или видео"
                    color: "white"
                    font.pixelSize: 14
                    font.family: "Segoe UI"
                    Layout.fillWidth: true
                }
            }
            MouseArea {
                id: photoArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    pickerRoot.hide()
                    pickerRoot.photoVideoRequested()
                }
            }
        }

        Rectangle {
            width: parent.width
            height: pickerRoot.itemHeight
            radius: 6
            color: docArea.containsMouse ? "#2b3a4a" : "transparent"
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 12
                Image {
                    source: "qrc:/messenger_client_uri/assets/icons/document.svg"
                    width: 20; height: 20; sourceSize: Qt.size(20, 20)
                }
                Text {
                    text: "Документ"
                    color: "white"
                    font.pixelSize: 14
                    font.family: "Segoe UI"
                    Layout.fillWidth: true
                }
            }
            MouseArea {
                id: docArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    pickerRoot.hide()
                    pickerRoot.documentRequested()
                }
            }
        }
    }    
}
