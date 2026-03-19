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
        spacing: 25

        Connections {
            target: Auth

            function onRegisterSuccess() {
                console.log("[Sign Up] Registration successful!")
                var loader = root.parent
                if (loader) {
                    loader.source = "sign_in.qml"
                }
            }

            function onRegisterFailed(errorMsg) {
                errorText.text = errorMsg
                errorText.visible = true
                errorTimer.restart()
            }
        }

        Text {
            text: "Зарегистрируйтесь в Алёше!"
            color: "white"
            font.pixelSize: 24
            font.bold: true
            font.family: "Segoe UI"
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 5
        }

        Item {
            Layout.fillWidth: true
            height: 50

            Text {
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
                text: "Имя профиля"
                color: displayNameField.activeFocus ? "#5eb5f7" : "#8a96a3"
                font.pixelSize: (displayNameField.length > 0 || displayNameField.activeFocus) ? 12 : 16
                font.family: "Segoe UI"
                
                Behavior on font.pixelSize { NumberAnimation { duration: 150 } }
                Behavior on anchors.bottomMargin { NumberAnimation { duration: 150 } }
                
                anchors.left: parent.left
                anchors.bottom: displayNameField.top
                anchors.bottomMargin: (displayNameField.length > 0 || displayNameField.activeFocus) ? 2 : -28
            }

            TextField {
                id: displayNameField
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
                        height: displayNameField.activeFocus ? 2 : 1
                        color: displayNameField.activeFocus ? "#5eb5f7" : "#39434f"
                        
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

        Item {
            Layout.fillWidth: true
            height: 50

            Text {
                text: "Повторите пароль"
                color: repeatPasswordField.activeFocus ? "#5eb5f7" : "#8a96a3"
                font.pixelSize: (repeatPasswordField.length > 0 || repeatPasswordField.activeFocus) ? 12 : 16
                font.family: "Segoe UI"
                
                Behavior on font.pixelSize { NumberAnimation { duration: 150 } }
                Behavior on anchors.bottomMargin { NumberAnimation { duration: 150 } }
                
                anchors.left: parent.left
                anchors.bottom: repeatPasswordField.top
                anchors.bottomMargin: (repeatPasswordField.length > 0 || repeatPasswordField.activeFocus) ? 2 : -28
            }

            TextField {
                id: repeatPasswordField
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
                        height: repeatPasswordField.activeFocus ? 2 : 1
                        color: repeatPasswordField.activeFocus ? "#5eb5f7" : "#39434f"
                        
                        Behavior on height { NumberAnimation { duration: 100 } }
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }
                }
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
            color: registerButtonMouseArea.containsMouse ? "#459ce0" : "#5eb5f7"
            radius: 8
            Layout.topMargin: 10

            Text {
                text: "СОЗДАТЬ АККАУНТ"
                color: "white"
                font.pixelSize: 14
                font.bold: true
                font.family: "Segoe UI"
                anchors.centerIn: parent
            }

            MouseArea {
                id: registerButtonMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                
                onPressed: parent.color = "#3782be"
                onReleased: parent.color = registerButtonMouseArea.containsMouse ? "#459ce0" : "#5eb5f7"
                
                onClicked: {
                    errorText.visible = false
                    
                    if (handleField.length == 0 || displayNameField.length == 0 || passwordField.length == 0) {
                        errorText.text = "Пожалуйста, заполните все поля"
                        errorText.visible = true
                        errorTimer.restart()
                        return
                    } else if (handleField.length < 3) {
                        errorText.text = "Логин слишком короткий"
                        errorText.visible = true
                        errorTimer.restart()
                        return
                    } else if (handleField.length > 32) {
                        errorText.text = "Логин слишком длинный"
                        errorText.visible = true
                        errorTimer.restart()
                        return
                    } else if (!/^[a-zA-Z0-9_]+$/.test(handleField.text)) {
                        errorText.text = "Логин: только латиница, цифры и _"
                        errorText.visible = true
                        errorTimer.restart()
                        return
                    } else if (passwordField.length < 8) {
                        errorText.text = "Пароль слишком короткий"
                        errorText.visible = true
                        errorTimer.restart()
                        return
                    } else if (passwordField.length > 72) {
                        errorText.text = "Пароль слишком длинный"
                        errorText.visible = true
                        errorTimer.restart()
                        return
                    } else if (passwordField.text !== repeatPasswordField.text) {
                        errorText.text = "Пароли не совпадают!"
                        errorText.visible = true
                        errorTimer.restart()
                        return
                    }

                    Auth.registerUser(handleField.text, displayNameField.text, passwordField.text)
                }
            }
        }

        Text {
            text: "Уже есть аккаунт? Войти"
            color: gotoLoginMouseArea.containsMouse ? "#5eb5f7" : "#8a96a3"
            font.pixelSize: 14
            font.family: "Segoe UI"
            Layout.alignment: Qt.AlignHCenter

            MouseArea {
                id: gotoLoginMouseArea
                anchors.fill: parent
                anchors.margins: -5
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    var loader = root.parent
                    if (loader) {
                        loader.source = "sign_in.qml"
                    }
                }
            }
        }
    }
}
