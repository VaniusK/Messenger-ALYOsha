import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls
import Messenger 1.0

Rectangle {
    id: chatAreaRoot
    color: "#0e1621"

    property bool isChatActive: activeChatId !== ""
    property string activeChatId: ""
    property string activeChatName: "Выберите чат"
    property bool isLoadingHistory: false
    property bool hasMoreHistory: true
    property bool isUploading: false
    property string pendingMediaCaption: ""

    Connections {
        target: MediaLayer
        function onFileDialogOpened() {
            systemDialogBlocker.open()
        }
        function onFileDialogClosed() {
            systemDialogBlocker.close()
        }
        function onFileSelected(filePath, fileType, fileSize, fileName) {
            if (fileSize === 0) {
                errorToast.show("Невозможно отправить пустой файл:\n" + filePath)
                return
            }
            if (fileSize >= 1073741824) { // 1 GB
                errorToast.show("Невозможно отправить файл больше 1 ГБ:\n" + filePath)
                return
            }
            mediaPreview.openWith(filePath, fileType, fileSize, fileName, messageInput.text.trim())
        }
        function onUploadFinished(fileUrl, fileType, fileSize, fileName) {
            isUploading = false
            var finalMsg = "Отправлено: " + fileName
            if (pendingMediaCaption !== "") {
                finalMsg += "\n" + pendingMediaCaption
            }
            ChatLayer.sendMessage(activeChatId, finalMsg,
                                fileUrl, fileType, fileSize, fileName)
            pendingMediaCaption = ""
        }
        function onUploadProgress(percent) {
            // Здесь будет прогресс-бар
        }
        function onUploadFailed(error) {
            isUploading = false
        }
    }

    ListModel {
        id: chatModel
    }

    onActiveChatIdChanged: {
        hasMoreHistory = true
        isLoadingHistory = false
        if (typeof messageInput !== "undefined") {
            messageInput.text = ""
        }
    }

    Connections {
        target: ChatLayer

        function onChatsHistoryLoaded(messages) {
            isLoadingHistory = true
            chatModel.clear()
            for (var i = 0; i < messages.length; i++) {
                chatModel.append(messages[i])
            }

            if (messages.length < 50) {
                hasMoreHistory = false;
            }

            if (chatModel.count > 0) {
                Qt.callLater(function() {
                    messageList.positionViewAtEnd()

                    if (messageList.contentHeight < messageList.height && chatModel.count > 0) {
                        var topmostMsgId = chatModel.get(0)._id || chatModel.get(0).id;
                        topmostMsgId = parseInt(topmostMsgId);

                        if (!isNaN(topmostMsgId) && topmostMsgId > 0 && hasMoreHistory) {
                            ChatLayer.fetchChatHistory(activeChatId, topmostMsgId);
                        } else {
                            isLoadingHistory = false;
                        }
                    } else {
                        isLoadingHistory = false;
                    }
                })
            } else {
                isLoadingHistory = false;
            }
        }

        function onChatsHistoryPrepended(messages) {
            if (messages.length === 0) {
                hasMoreHistory = false
                isLoadingHistory = false
                return
            }

            for (var i = messages.length - 1; i >= 0; i--) {
                chatModel.insert(0, messages[i])
            }

            Qt.callLater(function() {
                messageList.positionViewAtIndex(messages.length, ListView.Beginning)
                isLoadingHistory = false
            })
        }

        function onMessageSentSuccess(msg) {
            messageInput.text = ""
            if (!msg) return

            chatModel.append(msg)
            
            Qt.callLater(function() {
                messageList.positionViewAtEnd()
            })
        }

        function onIncomingWebSocketMessage(data) {
            if (data.event_type === "NEW_MESSAGE" && data.data && data.data.message) {
                var msg = data.data.message
                if (String(msg.chat_id) === String(activeChatId)) {
                    msg.is_me = (msg.sender_id === AppState.userId)
                    
                    chatModel.append(msg)
                    
                    Qt.callLater(function() {
                        messageList.positionViewAtEnd()
                    })
                }
            }
        }

        function onChatError(msg) {
            isLoadingHistory = false
            placeholderText.text = "Error: " + msg
            placeholderText.color = "red"
            placeholderText.parent.visible = true
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
                    
                    Image {
                        id: headerBookmarkIcon
                        visible: activeChatName === "Избранное"
                        source: "qrc:/messenger_client_uri/assets/icons/bookmark.svg"
                        width: 20; height: 20; sourceSize: Qt.size(20, 20)
                        anchors.centerIn: parent
                    }

                    Text {
                        visible: activeChatName !== "Избранное"
                        text: activeChatName ? activeChatName.charAt(0).toUpperCase() : "?"
                        color: "white"
                        font.bold: true
                        font.family: "Segoe UI"
                        font.pixelSize: 20
                        anchors.centerIn: parent
                        textFormat: Text.PlainText
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
                        textFormat: Text.PlainText
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
            model: chatModel
            spacing: 8
            topMargin: 15
            bottomMargin: 15

            onContentYChanged: {
                if (contentY <= 0 && chatModel.count > 0) {
                    if (isLoadingHistory || !hasMoreHistory || activeChatId === "") return;
                    
                    var topmostMsgId = chatModel.get(0)._id || chatModel.get(0).id;
                    var parsedId = parseInt(topmostMsgId);

                    if (!isNaN(parsedId) && parsedId > 0) {
                        isLoadingHistory = true;
                        ChatLayer.fetchChatHistory(activeChatId, parsedId);
                    }
                }
            }
            
            delegate: Item {
                id: msgDelegateItem
                width: messageList.width
                height: messageBubble.height + 6
                
                property bool isMe: model.is_me !== undefined ? model.is_me : false
                
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

                    TextEdit {
                        id: messageText
                        text: model.text !== undefined ? model.text : ""
                        anchors.fill: parent
                        anchors.margins: 8
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        anchors.bottomMargin: 12

                        wrapMode: TextEdit.Wrap
                        textFormat: TextEdit.PlainText
                        verticalAlignment: TextEdit.AlignVCenter
                        horizontalAlignment: TextEdit.AlignLeft

                        color: "white"
                        font.pixelSize: 15
                        font.family: "Segoe UI"

                        readOnly: true
                        selectByMouse: true
                        cursorVisible: false
                        selectedTextColor: "white"
                        selectionColor: "#4a90d9"
                    }

                    Text {
                        id: timeText
                        text: {
                            var t = model.sent_at;
                            if (!t) return "12:00"; 

                            if (typeof t === "string" && t.indexOf(" ") !== -1) {
                                t = t.replace(" ", "T");
                            }

                            if (t.indexOf("Z") === -1) {
                                t += "Z";
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
            Layout.preferredHeight: Math.min(250, Math.max(60, messageInput.contentHeight + 20))
            color: "#1c242f"

            Rectangle {
                width: parent.width
                height: 1
                color: "#151b23"
                anchors.top: parent.top
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 15
                anchors.topMargin: 10
                anchors.bottomMargin: 10
                spacing: 15

                Rectangle {
                    Layout.alignment: Qt.AlignBottom
                    width: 36
                    height: 36
                    color: "transparent"
                    visible: isChatActive
                    Image {
                        anchors.centerIn: parent
                        width: 22; height: 22
                        sourceSize: Qt.size(24, 24)
                        source: clipArea.containsMouse 
                                ? "qrc:/messenger_client_uri/assets/icons/clip_active.svg"
                                : "qrc:/messenger_client_uri/assets/icons/clip.svg"
                        Behavior on opacity { NumberAnimation { duration: 150 } }
                    }
                    MouseArea {
                        id: clipArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            mediaPicker.visible = !mediaPicker.visible
                        }
                    }
                }
                
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Flickable {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        height: Math.min(parent.height, messageInput.contentHeight)
                        contentWidth: width
                        contentHeight: messageInput.contentHeight
                        clip: true

                        TextEdit {
                            id: messageInput
                            width: parent.width
                            height: contentHeight
                            color: "white"
                            font.pixelSize: 16
                            font.family: "Segoe UI"
                            wrapMode: TextEdit.Wrap

                            onTextChanged: {
                                if (mediaPicker.visible && text.length > 0) {
                                    mediaPicker.visible = false
                                }
                            }

                            Keys.onPressed: function(event) {
                                if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && !(event.modifiers & Qt.ShiftModifier)) {
                                    event.accepted = true;
                                    if (text.trim() !== "" && isChatActive) {
                                        ChatLayer.sendMessage(activeChatId, text.trim());
                                    }
                                }
                            }
                        }
                    }

                    Text {
                        text: isChatActive ? "Сообщение..." : ""
                        color: "#8a96a3"
                        font.family: "Segoe UI"
                        font.pixelSize: 16
                        visible: !messageInput.text
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                Rectangle {
                    Layout.alignment: Qt.AlignBottom
                    width: 40
                    height: 40
                    color: "transparent"

                    Image {
                        source: messageInput.text.trim() === "" 
                            ? "qrc:/messenger_client_uri/assets/icons/send.svg"
                            : "qrc:/messenger_client_uri/assets/icons/send_active.svg"
                        width: 24; height: 24; sourceSize: Qt.size(24, 24)
                        anchors.centerIn: parent
                        anchors.horizontalCenterOffset: 2

                        Behavior on opacity { NumberAnimation { duration: 150 } }
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

    Popup {
        id: systemDialogBlocker
        parent: Overlay.overlay
        modal: true
        dim: false
        closePolicy: Popup.NoAutoClose
        background: Item {}
        contentItem: MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.BusyCursor
        }
    }

    Popup {
        id: errorToast
        parent: Overlay.overlay
        anchors.centerIn: parent
        z: 9999
        modal: false
        dim: false
        closePolicy: Popup.NoAutoClose

        background: Rectangle {
            color: Qt.rgba(0, 0, 0, 0.85)
            radius: 8
        }
        
        padding: 16
        
        contentItem: Text {
            id: errorToastText
            color: "white"
            font.pixelSize: 15
            font.family: "Segoe UI"
            text: ""
            wrapMode: Text.Wrap
        }
        
        Timer {
            id: errorToastTimer
            interval: 4000
            onTriggered: errorToast.close()
        }
        
        function show(msg) {
            errorToastText.text = msg
            errorToast.open()
            errorToastTimer.restart()
        }
    }


    MediaPickerPopup {
        id: mediaPicker
        z: 100
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 70
        anchors.left: parent.left
        anchors.leftMargin: 15
        onPhotoVideoRequested: MediaLayer.openFileDialog("image")
        onDocumentRequested: MediaLayer.openFileDialog("document")
    }

    MediaPreviewDialog {
        id: mediaPreview
        z: 200
        onSendRequested: function(filePath, fileType, asFile, caption) {
            pendingMediaCaption = caption
            messageInput.text = ""
            isUploading = true
            MediaLayer.uploadFile(filePath, fileType)
        }
        onCancelRequested: {}
    }
}
