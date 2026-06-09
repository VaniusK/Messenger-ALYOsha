import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Messenger 1.0

Rectangle {
    id: pickerRoot

    property int itemHeight: 44
    property int itemCount: 2

    width: 200
    height: itemHeight * itemCount + 16
    radius: 10
    color: appTheme.bgHeader
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
            color: photoArea.containsMouse ? appTheme.hoverColor : "transparent"
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 12
                IconImage {
                    source: "qrc:/messenger_client_uri/assets/icons/photo.svg"
                    color: appTheme.textHint
                    width: 20; height: 20; sourceSize: Qt.size(20, 20)
                }
                Text {
                    text: "Фото или видео"
                    color: appTheme.textMain
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
            color: docArea.containsMouse ? appTheme.hoverColor : "transparent"
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 12
                IconImage {
                    source: "qrc:/messenger_client_uri/assets/icons/document.svg"
                    color: appTheme.textHint
                    width: 20; height: 20; sourceSize: Qt.size(20, 20)
                }
                Text {
                    text: "Документ"
                    color: appTheme.textMain
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
