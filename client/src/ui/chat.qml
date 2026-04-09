import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Messenger 1.0

Rectangle {
    id: root
    color: "#f1f2f5"

    Shortcut {
        sequence: "Escape"
        onActivated: {
            if (VoiceLayer.isRecording) {
                chatArea.showCancelPrompt()
            } else if (sidebar.isSearching || sidebar.hasSearchFocus) {
                sidebar.clearSearch()
            } else if (chatArea.activeChatId !== "") {
                chatArea.activeChatId = ""
                chatArea.activeChatName = "Выберите чат"
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Sidebar {
            id: sidebar
            Layout.preferredWidth: Math.max(250, root.width * 0.34)
            Layout.fillHeight: true
            activeChatId: chatArea.activeChatId

            onChatSelected: function(chatId, chatName) {
                if (VoiceLayer.isRecording) {
                    chatArea.showCancelPrompt()
                    return
                }
                chatArea.activeChatId = chatId
                chatArea.activeChatName = chatName
            }

            onLogoutRequested: {
                logoutDialog.open()
            }
        }

        ChatArea {
            id: chatArea
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }

    Dialog {
        id: logoutDialog
        anchors.centerIn: parent
        width: 320
        height: 140
        modal: true

        padding: 0
        margins: 0

        background: Rectangle {
            color: "#1c242f"
            radius: 8
        }

        contentItem: ColumnLayout {
            spacing: 0
            anchors.fill: parent
            anchors.margins: 20

            Text {
                text: "Вы действительно хотите выйти?"
                color: "white"
                font.pixelSize: 16
                font.family: "Segoe UI"
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 5
                wrapMode: Text.Wrap
            }

            Item {
                Layout.fillHeight: true
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignRight | Qt.AlignBottom
                spacing: 10

                Rectangle {
                    width: 70
                    height: 36
                    radius: 8
                    color: cancelMouseArea.containsMouse ? "#2b3644" : "transparent"

                    Text {
                        text: "Отмена"
                        color: "#5eb5f7"
                        font.pixelSize: 15
                        font.bold: true
                        anchors.centerIn: parent
                    }

                    MouseArea {
                        id: cancelMouseArea
                        anchors.fill: parent
                        hoverEnabled: true 
                        cursorShape: Qt.PointingHandCursor
                        onClicked: logoutDialog.close()
                    }
                }

                Rectangle {
                    width: 70
                    height: 36
                    radius: 8
                    color: logoutMouseArea.containsMouse ? "#3d2a2d" : "transparent"

                    Text {
                        text: "Выйти"
                        color: "#f05b5b" 
                        font.pixelSize: 15
                        font.bold: true
                        anchors.centerIn: parent
                    }

                    MouseArea {
                        id: logoutMouseArea
                        anchors.fill: parent
                        hoverEnabled: true 
                        cursorShape: Qt.PointingHandCursor
                        
                        onPressed: parent.color = "#33181a"
                        onReleased: parent.color = logoutMouseArea.containsMouse ? "#3d2a2d" : "transparent"

                        onClicked: {
                            logoutDialog.close()
                            console.log("[Chat] exit to LogIn window")
                            AppState.clearState()

                            var loader = root.parent
                            if (loader) {
                                loader.source = "sign_in.qml"
                            }
                        }
                    }
                }
            }
        }
    }
}
