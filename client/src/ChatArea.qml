import QtQuick
import QtQuick.Layouts
import Messenger 1.0

Rectangle {
    id:chatAreaRoot
    color: "#e5ddd5"

    property bool isChatActive: activeChatId !== ""
    property string activeChatId: ""
    property string activeChatName: "Выберите чат"
    property var currentMessages: []

    Connections {
        target: ChatLayer

        function onChatsHistoryLoaded(messages) {
            currentMessages = messages
            messageList.model = currentMessages

            if (messageList.count > 0) {
                messageList.positionViewAtIndex(messageList.count - 1, ListView.End)
            }
        }

        function onMessageSentSuccess() {
            messageInput.text = ""
        }

        function onIncomingWebSocketMessage(data) {
            console.log("QML WS Event " + JSON.stringify(data))
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: placeholderText.width + 30
        height: 32
        radius: 16
        color: "#4d000000"
        visible: !isChatActive

        Text {
            id: placeholderText
            text: "Select a chat to start messaging"
            color: "white"
            font.pixelSize: 14
            font.bold: true
            anchors.centerIn: parent
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        visible: isChatActive

        Rectangle {
            Layout.fillWidth: true
            height: 95
            color: "#f5f5f5"
            border.color: "#e0e0e0"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 15

                Rectangle {
                    width: 40
                    height: 40
                    radius: 20
                    color: "#007bff"
                    
                    Text {
                        text: activeChatName.charAt(0).toUpperCase()
                        color: "white"
                        font.bold: true
                        anchors.centerIn: parent
                    }
                }

                Column {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                        
                    Text {
                        text: activeChatName
                        font.bold: true
                        font.pixelSize: 16
                    }

                    Text {
                        text: isChatActive ? "В сети" : ""
                        color: "#007bff"
                        font.pixelSize: 12
                    }
                }
            }
        }

        ListView {
            id: messageList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: currentMessages
            spacing: 10

            topMargin: 15
            bottomMargin: 15

            delegate: Rectangle {
                property var msgData: modelData
                property bool isMe: msgData.is_me

                width: Math.min(300, messageList.width * 0.7)
                height: Math.max(40, messageText.contentHeight + 20)
                radius: 12
                color: isMe ? "#dcf8c6" : "#ffffff"

                anchors.right: isMe ? parent.right : undefined
                anchors.left: isMe ? undefined : parent.left
                anchors.rightMargin: isMe ? 20 : 0
                anchors.leftMargin: isMe ? 0 : 20

                Text {
                    id: messageText
                    text: msgData.text
                    anchors.fill: parent
                    anchors.margins: 10
                    wrapMode: Text.Wrap
                    font.pixelSize: 14
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 60
            color: "#f5f5f5"
            border.color: "#e0e0e0"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                Rectangle {
                    Layout.fillWidth: true
                    height: 40
                    radius: 20
                    color: "white"
                    border.color: "#ccc"

                    TextInput {
                        id: messageInput
                        anchors.fill: parent
                        anchors.leftMargin: 15
                        anchors.rightMargin: 15
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 14
                        
                        Text {
                            text: isChatActive ? "Write a message..." : "Выберите чат"
                            color: "#999"
                            visible: !parent.text && !parent.activeFocus
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }

                Rectangle {
                    width: 40
                    height: 40
                    radius: 20
                    color: "#007bff"
                    
                    Text {
                        text: "➤"
                        color: "white"
                        font.pixelSize: 18
                        anchors.centerIn: parent
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (messageInput.text.trim() != "" && isChatActive) {
                                console.log("[ChatArea] Sending message...")
                                ChatLayer.sendMessage(activeChatId, messageInput.text.trim())
                            }
                        }
                    }
                }
            }
        }
    }
}
