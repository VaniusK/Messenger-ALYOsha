import QtQuick
import QtQuick.Layouts
import Messenger 1.0

Rectangle {
    id: chatAreaRoot
    color: "#0e1621"

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
            if (data.event_type === "NEW_MESSAGE" && data.data && data.data.message) {
                var msg = data.data.message
                if (String(msg.chat_id) === String(activeChatId)) {
                    msg.is_me = (msg.sender_id === AppState.userId)
                    var newMessages = currentMessages.slice()
                    newMessages.push(msg)
                    currentMessages = newMessages
                    
                    messageList.model = currentMessages
                    messageList.positionViewAtIndex(messageList.count - 1, ListView.End)
                }
            }
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(300, placeholderText.width + 40)
        height: 36
        radius: 18
        color: "#1c242f"
        visible: !isChatActive

        Text {
            id: placeholderText
            text: "Выберите чат, чтобы начать общение"
            color: "white"
            font.pixelSize: 14
            font.family: "Segoe UI"
            anchors.centerIn: parent
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: isChatActive

        Rectangle {
            Layout.fillWidth: true
            height: 60
            color: "#242f3d"
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 15

                Rectangle {
                    width: 40
                    height: 40
                    radius: 20
                    color: activeChatName === "Избранное" ? "#4a90d9" : "#5eb5f7"
                    clip: true
                    
                    Canvas {
                        id: headerBookmarkCanvas
                        visible: activeChatName === "Избранное"
                        width: 18
                        height: 22
                        anchors.centerIn: parent
                        anchors.verticalCenterOffset: 1

                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            var w = width
                            var h = height
                            var notchDepth = h * 0.28
                            ctx.beginPath()
                            ctx.moveTo(0, 0)
                            ctx.lineTo(w, 0)
                            ctx.lineTo(w, h)
                            ctx.lineTo(w / 2, h - notchDepth)
                            ctx.lineTo(0, h)
                            ctx.closePath()
                            ctx.fillStyle = "white"
                            ctx.fill()
                        }
                    }

                    Text {
                        visible: activeChatName !== "Избранное"
                        text: activeChatName ? activeChatName.charAt(0).toUpperCase() : "?"
                        color: "white"
                        font.bold: true
                        font.family: "Segoe UI"
                        font.pixelSize: 20
                        anchors.centerIn: parent
                    }
                }

                Column {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                        
                    Text {
                        text: activeChatName
                        font.bold: true
                        color: "white"
                        font.family: "Segoe UI"
                        font.pixelSize: 16
                    }

                    Text {
                        text: (isChatActive && activeChatName !== "Избранное") ? "в сети" : ""
                        visible: text !== ""
                        color: "#5eb5f7"
                        font.pixelSize: 13
                        font.family: "Segoe UI"
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
            spacing: 8
            topMargin: 15
            bottomMargin: 15
            
            delegate: Item {
                id: msgDelegateItem
                width: messageList.width
                height: messageBubble.height + 6
                
                property var msgData: modelData
                property bool isMe: msgData.is_me
                
                Rectangle {
                    id: messageBubble
                    width: Math.min(Math.max(60, messageText.implicitWidth + timeText.implicitWidth + 30), parent.width * 0.75)
                    height: messageText.implicitHeight + 20
                    radius: 12
                    
                    color: isMe ? "#2b5278" : "#18222d"
                    
                    anchors.right: isMe ? parent.right : undefined
                    anchors.left: isMe ? undefined : parent.left
                    anchors.rightMargin: isMe ? 15 : 0
                    anchors.leftMargin: isMe ? 0 : 15

                    Rectangle {
                        width: 12; height: 12
                        color: parent.color
                        anchors.bottom: parent.bottom
                        anchors.right: isMe ? parent.right : undefined
                        anchors.left: isMe ? undefined : parent.left
                    }

                    Item {
                        anchors.bottom: parent.bottom
                        anchors.right: isMe ? parent.right : undefined
                        anchors.left: isMe ? undefined : parent.left
                        anchors.rightMargin: isMe ? -8 : 0
                        anchors.leftMargin: isMe ? 0 : -8
                        width: 8; height: 16
                        
                        Rectangle {
                            width: 16; height: 16
                            color: messageBubble.color; radius: 4
                            anchors.bottom: parent.bottom
                            anchors.right: isMe ? parent.right : undefined
                            anchors.left: isMe ? undefined : parent.left
                        }
                        Rectangle {
                            width: 16; height: 16
                            color: "#0e1621"; radius: 8
                            anchors.bottom: parent.bottom; anchors.bottomMargin: 6
                            anchors.right: isMe ? parent.right : undefined
                            anchors.left: isMe ? undefined : parent.left
                            anchors.rightMargin: isMe ? -8 : 0
                            anchors.leftMargin: isMe ? 0 : -8
                        }
                    }

                    Text {
                        id: messageText
                        text: msgData.text
                        anchors.fill: parent
                        anchors.margins: 8
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        anchors.bottomMargin: 12
                        wrapMode: Text.Wrap
                        color: "white"
                        font.pixelSize: 15
                        font.family: "Segoe UI"
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignLeft
                    }

                    Text {
                        id: timeText
                        text: {
                            var t = msgData.sent_at;
                            if (!t) return "12:00"; 

                            if (typeof t === "string" && t.indexOf(" ") !== -1) {
                                t = t.replace(" ", "T");
                            }
                            var d = new Date(t);
                            if (isNaN(d.getTime())) return "??:??";

                            var hrs = d.getHours();
                            var mins = d.getMinutes();
                            return (hrs < 10 ? "0" : "") + hrs + ":" + (mins < 10 ? "0" : "") + mins;
                        }
                        color: isMe ? "#78aee3" : "#728392"
                        font.pixelSize: 11
                        font.family: "Segoe UI"
                        
                        anchors.bottom: parent.bottom
                        anchors.right: parent.right
                        anchors.bottomMargin: 4
                        anchors.rightMargin: 12
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 60
            color: "#1c242f"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                Rectangle {
                    Layout.fillWidth: true
                    height: 40
                    radius: 20
                    color: "#242f3d"
                    
                    TextInput {
                        id: messageInput
                        anchors.fill: parent
                        anchors.leftMargin: 20
                        anchors.rightMargin: 20
                        verticalAlignment: Text.AlignVCenter
                        color: "white"
                        font.pixelSize: 15
                        font.family: "Segoe UI"
                        clip: true
                        
                        Text {
                            text: isChatActive ? "Сообщение..." : ""
                            color: "#8a96a3"
                            font.family: "Segoe UI"
                            font.pixelSize: 15
                            visible: !parent.text && !parent.activeFocus
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        
                        Keys.onReturnPressed: {
                            if (messageInput.text.trim() != "" && isChatActive) {
                                ChatLayer.sendMessage(activeChatId, messageInput.text.trim())
                            }
                        }
                    }
                }

                Rectangle {
                    width: 40
                    height: 40
                    radius: 20
                    color: "#5eb5f7"
                    
                    Text {
                        text: "➤"
                        color: "white"
                        font.pixelSize: 18
                        anchors.centerIn: parent
                        anchors.horizontalCenterOffset: 1
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (messageInput.text.trim() != "" && isChatActive) {
                                ChatLayer.sendMessage(activeChatId, messageInput.text.trim())
                            }
                        }
                    }
                }
            }
        }
    }
}
