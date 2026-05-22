import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Messenger 1.0

Popup {
    id: dialogRoot
    modal: true
    dim: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    parent: Overlay.overlay
    anchors.centerIn: parent

    Overlay.modal: Rectangle {
        color: Qt.rgba(0, 0, 0, 0.75)
    }

    background: Item {} // Transparent, we draw dialogBox below
    padding: 0

    property string filePath: ""
    property string fileType: ""
    property string fileName: ""
    property int fileSize: 0
    property bool sendAsFile: fileType !== "image"

    function formatFileSize(bytes) {
        if (bytes === 0) return "0 Б"
        var k = 1024
        var sizes = ["Б", "КБ", "МБ", "ГБ", "ТБ"]
        var i = Math.floor(Math.log(bytes) / Math.log(k))
        return parseFloat((bytes / Math.pow(k, i)).toFixed(1)) + " " + sizes[i]
    }



    signal sendRequested(string filePath, string fileType, bool asFile, string caption)
    signal cancelRequested()

    function openWith(path, type, size, name, initialCaption) {
        filePath = path
        fileType = type
        fileSize = size || 0
        fileName = name || ""
        sendAsFile = (type !== "image")
        captionInput.text = initialCaption || ""
        
        if (fileName === "") {
            var parts = path.split('/');
            fileName = parts[parts.length - 1];
        }
        
        dialogRoot.open()
        captionInput.forceActiveFocus()
    }

    onClosed: {
        filePath = ""
    }

    Rectangle {
        id: dialogBox

        width: {
            var refWidth = Overlay.overlay ? Overlay.overlay.width : 1000;
            if (dialogRoot.sendAsFile || dialogRoot.fileType !== "image") return 400;
            if (previewImage.status === Image.Ready) {
                var ratio = previewImage.sourceSize.width / previewImage.sourceSize.height;
                var targetWidth = Math.min(600, Math.max(300, 450 * ratio));
                return Math.min(targetWidth, refWidth * 0.9);
            }
            return 450;
        }
        height: contentCol.implicitHeight + 40
        anchors.centerIn: parent
        radius: 10
        color: "#1c2733"

        MouseArea { anchors.fill: parent }

        ColumnLayout {
            id: contentCol
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
                margins: 20
            }
            spacing: 16

            Text {
                text: dialogRoot.sendAsFile ? "Отправить как файл" : "Отправить изображение"
                color: "white"
                font.pixelSize: 18
                font.bold: true
                font.family: "Segoe UI"
                Layout.bottomMargin: 4
            }


            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: {
                    if (dialogRoot.sendAsFile || dialogRoot.fileType !== "image") return 80;
                    if (previewImage.status === Image.Ready) {
                        var h = (dialogBox.width - 40) * (previewImage.sourceSize.height / previewImage.sourceSize.width);
                        return Math.min(h, 500);
                    }
                    return 300;
                }
                clip: true

                Rectangle {
                    anchors.fill: parent
                    color: "#17212b"
                    radius: 4
                    visible: dialogRoot.sendAsFile || dialogRoot.fileType !== "image"
                }

                Image {
                    id: previewImage
                    anchors.fill: parent
                    source: (dialogRoot.fileType === "image" && !dialogRoot.sendAsFile)
                            ? dialogRoot.filePath : ""
                    fillMode: Image.PreserveAspectFit
                    visible: dialogRoot.fileType === "image" && !dialogRoot.sendAsFile

                    asynchronous: true
                    sourceSize: Qt.size(1280, 1280) 
                }


                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 16
                    visible: dialogRoot.sendAsFile || dialogRoot.fileType !== "image"

                    Rectangle {
                        width: 52
                        height: 52 
                        radius: 26
                        color: "#5eb5f7"

                        Image {
                            anchors.centerIn: parent
                            width: 28
                            height: 28
                            source: "qrc:/messenger_client_uri/assets/icons/document.svg"
                            sourceSize: Qt.size(28, 28)
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        
                        Text {
                            text: dialogRoot.fileName
                            color: "white"
                            font.pixelSize: 16
                            font.family: "Segoe UI"
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }

                        Text {
                            text: dialogRoot.formatFileSize(dialogRoot.fileSize)
                            color: "#8a96a3"
                            font.pixelSize: 14
                            font.family: "Segoe UI"
                        }
                    }
                }
            }


            RowLayout {
                spacing: 12
                visible: dialogRoot.fileType === "image"
                Layout.topMargin: 4

                Rectangle {
                    id: asFileCheckbox
                    width: 22
                    height: 22
                    radius: 3
                    color: dialogRoot.sendAsFile ? "#5eb5f7" : "transparent"
                    border.color: dialogRoot.sendAsFile ? "#5eb5f7" : "#546576"
                    border.width: 2

                    Text {
                        visible: dialogRoot.sendAsFile
                        text: "✓"
                        color: "white"
                        font.pixelSize: 16
                        font.bold: true
                        anchors.centerIn: parent
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: dialogRoot.sendAsFile = !dialogRoot.sendAsFile
                    }
                }

                Text {
                    text: "Отправить как файл"
                    color: "white"
                    font.pixelSize: 15
                    font.family: "Segoe UI"
                }
            }

            Column {
                Layout.fillWidth: true
                spacing: 2
                
                Text {
                    text: "Подпись"
                    color: captionInput.activeFocus ? "#5eb5f7" : "#8a96a3"
                    font.family: "Segoe UI"
                    font.pixelSize: 13
                    font.bold: true
                }

                TextInput {
                    id: captionInput
                    width: parent.width
                    height: 32
                    color: "white"
                    font.pixelSize: 16
                    font.family: "Segoe UI"
                    verticalAlignment: TextInput.AlignVCenter
                    
                    Keys.onEscapePressed: {
                        dialogRoot.close()
                        dialogRoot.cancelRequested()
                    }
                    
                    Keys.onPressed: function(event) {
                        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                            event.accepted = true;
                            dialogRoot.sendRequested(
                                dialogRoot.filePath,
                                dialogRoot.fileType,
                                dialogRoot.sendAsFile,
                                captionInput.text.trim()
                            )
                            dialogRoot.close()
                        }
                    }
                }
                
                Rectangle {
                    width: parent.width
                    height: 1.5
                    color: captionInput.activeFocus ? "#5eb5f7" : "#39434f"
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Layout.topMargin: 10

                Item { Layout.fillWidth: true }

                Rectangle {
                    height: 40
                    width: 90
                    radius: 4
                    color: cancelBtnArea.containsMouse ? "#2b3a4a" : "transparent"
                    
                    Text {
                        text: "Отмена"
                        color: "#5eb5f7"
                        font.pixelSize: 14
                        font.bold: true
                        anchors.centerIn: parent
                    }
                    
                    MouseArea {
                        id: cancelBtnArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            dialogRoot.close()
                            dialogRoot.cancelRequested()
                        }
                    }
                }

                Rectangle {
                    height: 40
                    width: 110
                    radius: 4
                    color: sendBtnArea.containsMouse ? "#2b3a4a" : "transparent"

                    Text {
                        text: "Отправить" 
                        color: "#5eb5f7"
                        font.pixelSize: 14
                        font.bold: true
                        anchors.centerIn: parent
                    }

                    MouseArea {
                        id: sendBtnArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor

                        onClicked: {
                            dialogRoot.sendRequested(
                                dialogRoot.filePath,
                                dialogRoot.fileType,
                                dialogRoot.sendAsFile,
                                captionInput.text.trim()
                            )
                            dialogRoot.close()
                        }
                    }
                }
            }
        }
    }
}
