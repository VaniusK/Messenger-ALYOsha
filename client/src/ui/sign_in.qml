import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Messenger 1.0

Rectangle {
    id: root
    color: "#1c242f"

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(320, parent.width * 0.8)
        spacing: 30

        Connections {
            target: Auth

            function onLoginSuccess(token) {
                console.log("[Login] Authentication successful! Token:", token)
                AppState.token = token
                AppState.currentUserHandle = handleField.text
                ChatLayer.connectWebSocket()
                
                var loader = root.parent
                if (loader) {
                    loader.source = "chat.qml"
                }
            }

            function onLoginFailed(errorMsg) {
                errorText.text = errorMsg
                errorText.visible = true
                errorTimer.restart()
            }
        }

        Text {
            text: "Начните общение в Алёшe!"
            color: "white"
            font.pixelSize: 24
            font.bold: true
            font.family: "Segoe UI"
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 10
        }

        Item {
            Layout.fillWidth: true
            height: 50

            Text {
                id: handlePlaceholder
                text: "Логин"
                color: handleField.activeFocus ? "#5eb5f7" : "#8a96a3"
                font.pixelSize: (handleField.length > 0 || handleField.activeFocus) ? 12 : 16
                font.family: "Segoe UI"
                
                Behavior on font.pixelSize { NumberAnimation { duration: 150 } }
                Behavior on anchors.bottomMargin { NumberAnimation { duration: 150 } }
                
                anchors.left: parent.left
                anchors.bottom: handleField.top
                anchors.bottomMargin: (handleField.length > 0 || handleField.activeFocus) ? 2 : -28
            }

            TextField {
                id: handleField
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                color: "white"
                font.pixelSize: 16
                font.family: "Segoe UI"
                
                background: Item {
                    Rectangle {
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: handleField.activeFocus ? 2 : 1
                        color: handleField.activeFocus ? "#5eb5f7" : "#39434f"
                        
                        Behavior on height { NumberAnimation { duration: 100 } }
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            height: 50

            Text {
                id: passwordPlaceholder
                text: "Пароль"
                color: passwordField.activeFocus ? "#5eb5f7" : "#8a96a3"
                font.pixelSize: (passwordField.length > 0 || passwordField.activeFocus) ? 12 : 16
                font.family: "Segoe UI"
                
                Behavior on font.pixelSize { NumberAnimation { duration: 150 } }
                Behavior on anchors.bottomMargin { NumberAnimation { duration: 150 } }
                
                anchors.left: parent.left
                anchors.bottom: passwordField.top
                anchors.bottomMargin: (passwordField.length > 0 || passwordField.activeFocus) ? 2 : -28
            }

            TextField {
                id: passwordField
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                color: "white"
                font.pixelSize: 16
                font.family: "Segoe UI"
                echoMode: TextInput.Password
                
                background: Item {
                    Rectangle {
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: passwordField.activeFocus ? 2 : 1
                        color: passwordField.activeFocus ? "#5eb5f7" : "#39434f"
                        
                        Behavior on height { NumberAnimation { duration: 100 } }
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }
                }
            }
        }

        Row {
            Layout.fillWidth: true
            spacing: 8

            Rectangle {
                id: rememberCheckbox
                width: 20
                height: 20
                radius: 4
                color: checked ? "#5eb5f7" : "transparent"
                border.color: checked ? "#5eb5f7" : "#39434f"
                border.width: 2
                anchors.verticalCenter: parent.verticalCenter

                property bool checked: true

                Text {
                    visible: parent.checked
                    text: "✓"
                    color: "white"
                    font.pixelSize: 13
                    font.bold: true
                    anchors.centerIn: parent
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: parent.checked = !parent.checked
                }

                Behavior on color { ColorAnimation { duration: 150 } }
                Behavior on border.color { ColorAnimation { duration: 150 } }
            }

            Text {
                text: "Запомнить меня"
                color: "#8a96a3"
                font.pixelSize: 14
                font.family: "Segoe UI"
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        Text {
            id: errorText
            text: ""
            color: "#f05b5b"
            font.pixelSize: 14
            visible: false
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: -10

            Timer {
                id: errorTimer 
                interval: 5000
                repeat: false
                onTriggered: errorText.visible = false
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 44
            color: loginMouseArea.containsMouse ? "#459ce0" : "#5eb5f7"
            radius: 8

            Text {
                text: "ВОЙТИ"
                color: "white"
                font.pixelSize: 14
                font.bold: true
                font.family: "Segoe UI"
                anchors.centerIn: parent
            }

            MouseArea {
                id: loginMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                
                onPressed: parent.color = "#3782be"
                onReleased: parent.color = loginMouseArea.containsMouse ? "#459ce0" : "#5eb5f7"
                
                onClicked: {
                    errorText.visible = false
                    if (handleField.text.length == 0) {
                        errorText.text = "Введите логин!"
                        errorText.visible = true
                        errorTimer.restart()
                    } else if (passwordField.text.length == 0) {
                        errorText.text = "Введите пароль!"
                        errorText.visible = true
                        errorTimer.restart()
                    } else {
                        AppState.rememberMe = rememberCheckbox.checked
                        Auth.loginUser(handleField.text, passwordField.text)
                    }
                }
            }
        }

        Text {
            text: "Регистрация"
            color: registerMouseArea.containsMouse ? "#5eb5f7" : "#8a96a3"
            font.pixelSize: 14
            font.family: "Segoe UI"
            Layout.alignment: Qt.AlignHCenter

            MouseArea {
                id: registerMouseArea
                anchors.fill: parent
                anchors.margins: -5
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    var loader = root.parent
                    if (loader) {
                        loader.source = "sign_up.qml"
                    }
                }
            }
        }
    }
}
