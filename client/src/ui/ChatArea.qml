import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls
import QtMultimedia
import QtQuick.Window
import Messenger 1.0

Rectangle {
    id: chatAreaRoot
    color: appTheme.bgMain

    property bool isChatActive: activeChatId !== ""
    property string activeChatId: ""
    property string activeChatName: "Выберите чат"
    property bool isLoadingHistory: false
    property bool hasMoreHistory: true
    property bool isUploading: false
    property string activeFullscreenVideoUrl: ""
    property string activeChatType: "direct" // "direct", "group", "saved"
    property string activeChatDescription: ""

    MediaPlayer {
        id: globalAudioPlayer
        audioOutput: AudioOutput {
            volume: 1.0
        }

        readonly property int customPlaybackState: {
            if (playbackState === MediaPlayer.PlayingState) return 1;
            if (playbackState === MediaPlayer.PausedState) return 2;
            return 0;
        }

        onMediaStatusChanged: {
            if (mediaStatus === MediaPlayer.EndOfMedia) {
                globalPlayingMsgId = "" 
                source = "" 
            }
        }

        property int playState: MediaPlayer.StoppedState
        onPlaybackStateChanged: playState = playbackState
    }
    
    property string globalPlayingMsgId: ""
    property string globalActiveVoiceAuthor: ""
    property string globalActiveVoiceDate: ""
    
    function formatGlobalTime(ms) {
        if (isNaN(ms) || ms < 0) return "00:00"
        var totalSec = Math.floor(ms / 1000)
        var m = Math.floor(totalSec / 60)
        var s = totalSec % 60
        return (m < 10 ? "0" : "") + m + ":" + (s < 10 ? "0" : "") + s
    }

    function showCancelPrompt() {
        cancelVoiceDialog.open()
    }

    function loadSecretHistory(chatId, beforeTimestamp) {
        isLoadingHistory = true;
        var historyStr = SecretChatManager.fetchSecretChatHistory(chatId, beforeTimestamp);
        
        try {
            if (historyStr !== "") {
                var messages = JSON.parse(historyStr);
                
                if (beforeTimestamp === 0) {
                    chatModel.clear();
                    for (var i = messages.length - 1; i >= 0; i--) chatModel.append(messages[i]);
                } else {
                    for (var k = 0; k < messages.length; k++) chatModel.append(messages[k]);
                }
                
                if (messages.length < 50) hasMoreHistory = false;
            } else {
                if (beforeTimestamp === 0) chatModel.clear();
                hasMoreHistory = false;
            }
        } catch (e) {
            console.error("[ChatArea] Ошибка парсинга истории секретного чата:", e);
        }
        
        Qt.callLater(function() { isLoadingHistory = false; })
    }

    Connections {
        target: MediaLayer
        function onFileDialogOpened() {
            systemDialogBlocker.open()
        }
        function onFileDialogClosed() {
            systemDialogBlocker.close()
        }
        function onFileSelected(filePath, fileType, fileSize, fileName) {
            mediaPreview.openWith(filePath, fileType, fileSize, fileName, messageInput.text.trim())
        }
        function onUploadFinished() {
            isUploading = false
            ChatLayer.fetchChatHistory(activeChatId, 0)
        }
        function onUploadProgress(percent) {}
        function onUploadFailed(error) {
            isUploading = false
            errorToast.show("Ошибка загрузки: " + error)
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
            for (var i = messages.length - 1; i >= 0; i--) {
                chatModel.append(messages[i])
            }

            if (messages.length < 50) {
                hasMoreHistory = false;
            }

            if (chatModel.count > 0) {
                Qt.callLater(function() {
                    messageList.positionViewAtIndex(0, ListView.Beginning)

                    if (messageList.contentHeight < messageList.height && chatModel.count > 0) {
                        var topmostMsgId = chatModel.get(chatModel.count - 1)._id || chatModel.get(chatModel.count - 1).id;
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

            for (var i = 0; i < messages.length; i++) {
                chatModel.append(messages[i])
            }

            Qt.callLater(function() {
                isLoadingHistory = false
            })
        }

        function onMessageSentSuccess(msg) {
            messageInput.text = ""
            if (!msg) return

            chatModel.insert(0, msg)
            
            Qt.callLater(function() {
                messageList.positionViewAtIndex(0, ListView.Beginning)
            })
        }

        function onIncomingWebSocketMessage(data) {
            if (data.message_type === ApiEnums.COMMON_NEW_MESSAGE && data.data && data.data.message) {
                var msg = data.data.message
                if (String(msg.chat_id) === String(activeChatId)) {
                    msg.is_me = (String(msg.sender_id) === String(AppState.userId))
                    
                    chatModel.insert(0, msg)

                    Qt.callLater(function() {
                        messageList.positionViewAtIndex(0, ListView.Beginning)
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

    Connections {
        target: VoiceLayer
        function onVoiceMessageReady(audioUrl) {
            isUploading = true
            MediaLayer.uploadFile(activeChatId, audioUrl, false, "", "voice")
        }
    }

    Rectangle {
        id: globalPlayerUI
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: globalPlayingMsgId !== "" ? 45 : 0
        visible: height > 0
        color: appTheme.bgPanel
        clip: true
        z: 10 
        
        Behavior on height { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }

        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            anchors.leftMargin: 20
            spacing: 15
            
            Rectangle {
                width: 30; height: 30; radius: 15
                color: "transparent"
                
                Image {
                    anchors.centerIn: parent
                    width: 14; height: 14
                    source: globalAudioPlayer.playbackState === MediaPlayer.PlayingState 
                            ? "qrc:/messenger_client_uri/assets/icons/pause_blue.svg" 
                            : "qrc:/messenger_client_uri/assets/icons/play_blue.svg"
                    sourceSize: Qt.size(14, 14)
                }
                
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (globalAudioPlayer.playbackState === 1) {
                            globalAudioPlayer.pause()
                        } else {
                            globalAudioPlayer.play()
                        }
                    }
                }
            }
            
            Text {
                text: globalActiveVoiceAuthor + "  " + globalActiveVoiceDate
                color: appTheme.textMain
                font.pixelSize: 14
                font.family: "Segoe UI"
                font.bold: true
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            
            Text {
                text: formatGlobalTime(globalAudioPlayer.position) + " / " + formatGlobalTime(globalAudioPlayer.duration)
                color: "#728392"
                font.pixelSize: 14
                font.family: "Segoe UI"
            }

            Rectangle {
                width: 30; height: 30; radius: 15
                color: closeHoverArea.containsMouse ? appTheme.hoverColor : "transparent"
                
                Text {
                    anchors.centerIn: parent
                    text: "✕"
                    color: "#728392"
                    font.pixelSize: 16
                }
                
                MouseArea {
                    id: closeHoverArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        globalAudioPlayer.stop()
                        globalPlayingMsgId = ""
                    }
                }
            }
        }
        
        Rectangle {
            width: parent.width; height: 1
            color: "#18222d"
            anchors.bottom: parent.bottom
        }
    }

    Rectangle {
        anchors.top: globalPlayerUI.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        color: "transparent"
        visible: !isChatActive

        Rectangle {
            anchors.centerIn: parent
            width: Math.min(300, placeholderText.width + 40)
            height: 36
            radius: 18
            color: appTheme.bgPanel

            Text {
                id: placeholderText
                text: "Выберите чат, чтобы начать общение"
                color: appTheme.textMain
                font.pixelSize: 14
                font.family: "Segoe UI"
                anchors.centerIn: parent
            }
        }
    }

    ColumnLayout {
        anchors.top: globalPlayerUI.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 0
        visible: isChatActive

        Rectangle {
            Layout.fillWidth: true
            height: 60
            color: appTheme.bgHeader

            MouseArea {
                anchors.fill: parent
                enabled: activeChatType === "group" || activeChatType === "direct"
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (activeChatType === "group") {
                        ChatLayer.fetchChatMembers(activeChatId)
                        ChatLayer.fetchChatInfo(activeChatId)
                        groupInfoPopup.open()
                    } else if (activeChatType === "direct") {
                        directInfoPopup.open()
                    }
                }
            }
            
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
                        width: parent.width
                        elide: Text.ElideRight
                        font.bold: true
                        color: appTheme.textMain
                        font.family: "Segoe UI"
                        font.pixelSize: 16
                        textFormat: Text.PlainText
                    }

                    Text {
                        text: "в сети"
                        visible: isChatActive && activeChatName !== "Избранное" && activeChatType !== "group"
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
            verticalLayoutDirection: ListView.BottomToTop

            onContentYChanged: {
                if (contentY <= -messageList.contentHeight && chatModel.count > 0) {
                    if (isLoadingHistory || !hasMoreHistory || activeChatId === "") return;
                    
                    var topmostMsgId = chatModel.get(chatModel.count - 1)._id || chatModel.get(chatModel.count - 1).id;
                    var parsedId = parseInt(topmostMsgId);

                    if (!isNaN(parsedId) && parsedId > 0) {
                        isLoadingHistory = true;
                        if (activeChatType === "secret") {
                            var oldestMsg = chatModel.get(chatModel.count - 1);
                            var ts = oldestMsg.sent_at ? new Date(oldestMsg.sent_at).getTime() : 0;
                            chatAreaRoot.loadSecretHistory(activeChatId, ts);
                        } else {
                            ChatLayer.fetchChatHistory(activeChatId, parsedId)
                        }
                    }
                }
            }
            
            delegate: Item {
                id: msgDelegateItem
                width: messageList.width
                
                property bool isMe: model.is_me !== undefined ? model.is_me : false
                property var msgAttachments: model.attachments !== undefined ? model.attachments : null
                property var firstAttachment: {
                    if (!msgAttachments) return null;
                    if (msgAttachments.count !== undefined) return msgAttachments.count > 0 ? msgAttachments.get(0) : null;
                    if (msgAttachments.length !== undefined) return msgAttachments.length > 0 ? msgAttachments[0] : null;
                    return null;
                }
                
                property string fileTypeStr: firstAttachment && typeof firstAttachment.file_type === 'string' 
                                             ? firstAttachment.file_type.toLowerCase() : ""

                property bool isVoice: (model.type === "voice") || 
                                       (fileTypeStr.indexOf("audio/") === 0) ||
                                       (typeof model.text === 'string' && model.text.indexOf("VOICE::") === 0)

                property bool isImage: !isVoice && fileTypeStr.indexOf("image/") === 0 && model.type !== "text"
                property bool isVideo: !isVoice && fileTypeStr.indexOf("video/") === 0 && model.type !== "text"

                property bool hasFileAttachment: firstAttachment !== null && !isVoice && !isImage && !isVideo

                property string fileUrl: {
                    if (typeof model.text === 'string' && model.text.indexOf("VOICE::") === 0) return model.text.substring(7)
                    if (firstAttachment !== null && firstAttachment.download_url) return firstAttachment.download_url
                    return ""
                }

                property int img_width: {
                    if (typeof model.text === 'string' && model.text.indexOf("VOICE::") === 0) return 200
                    if (firstAttachment !== null && firstAttachment.img_width) return firstAttachment.img_width
                    return 0
                }

                property int img_height: {
                    if (typeof model.text === 'string' && model.text.indexOf("VOICE::") === 0) return 100
                    if (firstAttachment !== null && firstAttachment.img_height) return firstAttachment.img_height
                    return 0
                }
                
                property string uniqueId: model._id || model.id || index.toString()
                property bool isPlayingThis: globalPlayingMsgId === uniqueId && globalAudioPlayer.playbackState === 1
                
                height: messageBubble.height + 6

                property var senderInfo: model.sender_info ? model.sender_info : null
                property string senderNameStr: senderInfo 
                    ? (senderInfo.display_name || senderInfo.handle || "User " + (model.sender_id || "")) 
                    : ("User " + (model.sender_id || ""))

                Item {
                    visible: model.type !== "system"
                    width: parent.width
                    height: messageBubble.height

                    // Аватарка собеседника слева (только в группах)
                    Rectangle {
                        visible: chatAreaRoot.activeChatType === "group" && !isMe
                        width: 36; height: 36; radius: 18
                        color: "#5eb5f7"
                        anchors.left: parent.left; anchors.leftMargin: 10
                        anchors.bottom: messageBubble.bottom

                        Text {
                            anchors.centerIn: parent
                            text: senderNameStr ? senderNameStr.charAt(0).toUpperCase() : "?"
                            color: "white"; font.bold: true; font.pixelSize: 16
                        }
                    }

                    Rectangle {
                        id: messageBubble
                        anchors.left: isMe ? undefined : (chatAreaRoot.activeChatType === "group" ? parent.left : parent.left)
                        anchors.leftMargin: isMe ? 0 : (chatAreaRoot.activeChatType === "group" ? 56 : 15) 
                        anchors.right: isMe ? parent.right : undefined
                        anchors.rightMargin: isMe ? 15 : 0

                        property int nameOffset: (chatAreaRoot.activeChatType === "group" && !isMe) ? 22 : 0

                        property real nameWidth: senderNameTxt.visible ? senderNameTxt.implicitWidth + 24 : 0

                        width: (isVoice || isImage || isVideo || hasFileAttachment)
                            ? Math.min(280, parent.width * 0.75) 
                            : Math.min(Math.max(60, messageText.implicitWidth + timeText.implicitWidth + 30, nameWidth), parent.width * 0.75)
                        
                        height: {
                            if (isVoice) return 60 + nameOffset
                            if (hasFileAttachment) return fileAttachCol.implicitHeight + 32 + nameOffset

                            let mediaHeight = 0;
                            if (isImage) { 
                                mediaHeight = imageContainer.height + 24 
                            } else if (isVideo) { 
                                mediaHeight = videoContainer.height + 24 
                            }

                            let textHeight = (model.text && model.text.trim() !== "") ? messageText.contentHeight + 20 : 0
                            return mediaHeight + textHeight + (mediaHeight === 0 && textHeight !== 0 ? 8 : 0) + nameOffset
                        }
                        
                        radius: 12
                        color: isMe ? appTheme.myBubble : appTheme.otherBubble

                        Rectangle {
                            width: 12; height: 12; color: parent.color
                            anchors.bottom: parent.bottom
                            anchors.right: isMe ? parent.right : undefined
                            anchors.left: isMe ? undefined : parent.left
                        }

                        Item {
                            anchors.bottom: parent.bottom
                            anchors.right: isMe ? parent.right : undefined
                            anchors.left: isMe ? undefined : parent.left
                            anchors.rightMargin: isMe ? -8 : 0; anchors.leftMargin: isMe ? 0 : -8
                            width: 8; height: 16
                            clip: true

                            Rectangle { 
                                width: 16; height: 16; radius: 4; color: messageBubble.color
                                anchors.bottom: parent.bottom
                                anchors.right: isMe ? parent.right : undefined
                                anchors.left: isMe ? undefined : parent.left 
                            }
                            Rectangle { 
                                width: 16; height: 16; radius: 8; color: appTheme.bgMain
                                anchors.bottom: parent.bottom; anchors.bottomMargin: 6
                                anchors.right: isMe ? parent.right : undefined
                                anchors.left: isMe ? undefined : parent.left
                                anchors.rightMargin: isMe ? -8 : 0
                                anchors.leftMargin: isMe ? 0 : -8 
                            }
                        }

                        // ИМЯ ОТПРАВИТЕЛЯ
                        Text {
                            id: senderNameTxt
                            visible: chatAreaRoot.activeChatType === "group" && !isMe

                            text: {
                                var sInfo = model.sender_info;
                                if (sInfo) {
                                    return sInfo.display_name || sInfo.handle || ("User " + model.sender_id)
                                }
                                return model.sender_name || ("User " + (model.sender_id || ""))
                            }
                            
                            color: "#5eb5f7"
                            font.bold: true
                            font.pixelSize: 14
                            font.family: "Segoe UI"

                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            anchors.topMargin: 8

                            elide: Text.ElideRight
                        }
                    
                        Item {
                            id: imageContainer
                            visible: isImage
                            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 4
                            anchors.topMargin: 4 + parent.nameOffset
                            
                            property real calculatedHeight: (img_width > 0) 
                                ? Math.min(300, Math.max(100, width * (img_height / img_width))) 
                                : 150
                            height: isImage ? calculatedHeight : 0
                            
                            Rectangle {
                                anchors.fill: parent; radius: 10; clip: true; color: "#131b24"

                                Image {
                                    id: attachedImage
                                    anchors.fill: parent
                                    source: isImage ? fileUrl : ""
                                    fillMode: Image.PreserveAspectFit 
                                    asynchronous: true
                                    
                                    function reload() {
                                        var oldSource = source
                                        source = ""
                                        Qt.callLater(function() { source = oldSource })
                                    }

                                    Connections {
                                        target: MediaCacheLayer
                                        function onImageLoaded(downloadedPath) {
                                            if (fileUrl === downloadedPath) {
                                                attachedImage.reload()
                                            }
                                        }
                                    }

                                }
                                MouseArea {
                                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                    onClicked: fullScreenImagePreview.openWith(fileUrl)
                                }
                            }
                        }

                        Item {
                            id: videoContainer
                            visible: isVideo
                            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 4
                            anchors.topMargin: 4 + parent.nameOffset
                            height: isVideo ? (width * 0.75) : 0
                            
                            Rectangle {
                                anchors.fill: parent; radius: 10; clip: true; color: "black"
                                MediaPlayer {
                                    id: videoPlayer
                                    source: (isVideo && chatAreaRoot.activeFullscreenVideoUrl !== fileUrl) ? fileUrl : ""
                                    videoOutput: videoOut
                                }

                                VideoOutput {
                                    id: videoOut
                                    anchors.fill: parent
                                    fillMode: VideoOutput.PreserveAspectCrop
                                }
                                
                                Image {
                                    anchors.centerIn: parent
                                    source: "qrc:/messenger_client_uri/assets/icons/play.svg" 
                                    width: 48; height: 48; sourceSize: Qt.size(48, 48); z: 2
                                    visible: videoPlayer.playbackState !== MediaPlayer.PlayingState
                                }
                                
                                MouseArea {
                                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                    onClicked: fullScreenVideoPreview.openWith(fileUrl)
                                }
                            }
                        }

                        TextEdit {
                            id: messageText
                            visible: !isVoice && !hasFileAttachment && text.trim() !== ""
                            text: (model.text !== undefined && model.text !== null) ? model.text.trim() : ""
                            
                            anchors.top: isImage ? imageContainer.bottom : (isVideo ? videoContainer.bottom : parent.top)
                            anchors.left: parent.left; width: parent.width - 24
                            anchors.margins: 8
                            anchors.leftMargin: 12
                            anchors.topMargin: (isImage || isVideo) ? 8 : (12 + parent.nameOffset)

                            wrapMode: TextEdit.Wrap
                            textFormat: TextEdit.PlainText
                            color: isMe ? "white" : appTheme.textMain
                            font.pixelSize: 15
                            font.family: "Segoe UI"
                            readOnly: true 
                            selectByMouse: true
                            cursorVisible: false
                            selectedTextColor: "white"
                            selectionColor: "#4a90d9"
                        }

                        Column {
                            id: fileAttachCol
                            visible: hasFileAttachment
                            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                            anchors.margins: 12
                            anchors.topMargin: 12 + parent.nameOffset
                            spacing: 6

                            Item {
                                width: parent.width; height: 48

                                RowLayout {
                                    anchors.fill: parent; spacing: 12

                                    Rectangle {
                                        width: 44; height: 44; radius: 22
                                        color: isMe ? "#1e3d5e" : "#0d1825"
                                        Layout.alignment: Qt.AlignVCenter
                                        
                                        Image {
                                            anchors.centerIn: parent; width: 22; height: 22
                                            source: "qrc:/messenger_client_uri/assets/icons/document.svg"
                                            sourceSize: Qt.size(22, 22)
                                        }
                                    }

                                    Column {
                                        Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter; spacing: 2
                                        
                                        Text {
                                            text: firstAttachment ? (firstAttachment.original_filename || firstAttachment.file_name || "Файл") : "Файл"
                                            color: isMe ? "white" : appTheme.textMain
                                            font.pixelSize: 14
                                            font.family: "Segoe UI"
                                            elide: Text.ElideMiddle
                                            width: parent.width
                                        }

                                        Text {
                                            text: {
                                                if (!firstAttachment || !firstAttachment.file_size_bytes) return "0 Б"
                                                var b = firstAttachment.file_size_bytes
                                                if (b < 1024) return b + " Б"
                                                if (b < 1048576) return (b / 1024).toFixed(1) + " КБ"
                                                if (b < 1073741824) return (b / 1048576).toFixed(1) + " МБ"
                                                return (b / 1073741824).toFixed(1) + " ГБ"
                                            }
                                            color: isMe ? Qt.rgba(1, 1, 1, 0.7) : appTheme.textHint
                                            font.pixelSize: 12
                                            font.family: "Segoe UI"
                                        }
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: 
                                        if (fileUrl) 
                                            MediaLayer.downloadFile(
                                                fileUrl, 
                                                firstAttachment 
                                                    ? (firstAttachment.original_filename || firstAttachment.file_name || "file")
                                                    : "file"
                                            )
                                }
                            }

                            Text {
                                visible: model.text && model.text.trim() !== ""
                                text: model.text ? model.text.trim() : ""
                                color: isMe ? "white" : appTheme.textMain
                                font.pixelSize: 14
                                font.family: "Segoe UI"
                                wrapMode: Text.Wrap
                                width: parent.width
                            }
                        }

                        RowLayout {
                            visible: isVoice
                            anchors.fill: parent; anchors.margins: 4; anchors.leftMargin: 8; anchors.rightMargin: 48; spacing: 12
                            anchors.topMargin: 4 + parent.nameOffset

                            Rectangle {
                                width: 44; height: 44; radius: 22
                                color: playMouseArea.pressed 
                                    ? (isMe ? "#e6e6e6" : Qt.lighter(appTheme.accent, 1.1)) 
                                    : (isMe ? "white" : appTheme.accent)
                                
                                IconImage {
                                    anchors.centerIn: parent
                                    width: 16; height: 16
                                    source: isPlayingThis 
                                        ? "qrc:/messenger_client_uri/assets/icons/pause.svg"
                                        : "qrc:/messenger_client_uri/assets/icons/play.svg"
                                    color: isMe ? appTheme.accent : "white"
                                    sourceSize: Qt.size(16, 16)
                                }
                                
                                MouseArea {
                                    id: playMouseArea
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (globalPlayingMsgId === uniqueId) {
                                            if (globalAudioPlayer.playbackState === MediaPlayer.PlayingState) {
                                                globalAudioPlayer.pause()
                                            } else {
                                                globalAudioPlayer.play()
                                            }
                                        } else {
                                            globalAudioPlayer.stop() 
                                            
                                            globalPlayingMsgId = uniqueId
                                            globalActiveVoiceAuthor = isMe ? AppState.currentUserHandle : activeChatName
                                            globalActiveVoiceDate = timeText.text
                                            globalAudioPlayer.source = ""
                                            globalAudioPlayer.source = fileUrl
                                            globalAudioPlayer.play()
                                        }
                                    }
                                }
                            }
                            
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                
                                Item {
                                    id: waveRow
                                    Layout.fillWidth: true
                                    Layout.topMargin: 4
                                    height: 26 
                                    
                                    property real progress: globalPlayingMsgId === uniqueId && globalAudioPlayer.duration > 0
                                        ? globalAudioPlayer.position / globalAudioPlayer.duration : 0.0

                                    Rectangle {
                                        width: parent.width
                                        height: 3
                                        anchors.verticalCenter: parent.verticalCenter
                                        radius: 1.5
                                        color: isMe ? Qt.alpha(appTheme.textMain, 0.7) : appTheme.textHint
                                        
                                        Rectangle {
                                            width: parent.width * waveRow.progress
                                            height: parent.height
                                            radius: 1.5
                                            color: isMe ? "white" : appTheme.textMain
                                        }
                                    }
                                    
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        
                                        function seekToPosition(xPos) {
                                            if (globalPlayingMsgId === uniqueId && globalAudioPlayer.duration > 0) {
                                                var safeX = Math.max(0, Math.min(xPos, width))
                                                var newPos = (safeX / width) * globalAudioPlayer.duration
                                                globalAudioPlayer.position = newPos
                                            }
                                        }
                                        
                                        onClicked: seekToPosition(mouseX)
                                        onPositionChanged: { 
                                            if (pressed) seekToPosition(mouseX) 
                                        }
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text {
                                        text: {
                                            var timeStr = "00:00"

                                            if (globalPlayingMsgId === uniqueId && globalAudioPlayer.duration > 0) {
                                                timeStr = formatGlobalTime(globalAudioPlayer.position) + " / " + formatGlobalTime(globalAudioPlayer.duration)
                                            } else if (globalAudioPlayer.duration > 0 && globalAudioPlayer.source.toString() === fileUrl) {
                                                timeStr = formatGlobalTime(globalAudioPlayer.duration)
                                            }

                                            var sizeStr = "";
                                            if (firstAttachment && firstAttachment.file_size_bytes) {
                                                var b = firstAttachment.file_size_bytes;
                                                if (b < 1048576) sizeStr = ", " + (b / 1024).toFixed(1) + " КБ"
                                                else sizeStr = ", " + (b / 1048576).toFixed(1) + " МБ"
                                            }
                                            
                                            return timeStr + sizeStr
                                        }
                                        color: isMe ? Qt.rgba(1, 1, 1, 0.7) : appTheme.textHint
                                        font.pixelSize: 11
                                        font.family: "Segoe UI"
                                    }
                                    Item { Layout.fillWidth: true }
                                }
                                Item { Layout.fillHeight: true }
                            }
                        }

                        RowLayout {
                            anchors.bottom: parent.bottom
                            anchors.right: parent.right
                            anchors.bottomMargin: 4
                            anchors.rightMargin: 12
                            spacing: 4

                            Text {
                                id: timeText
                                text: {
                                    var t = model.created_at || model.timestamp || model.time || model.sent_at
                                    if (!t) return "00:00"
                                    var d
                                    if (typeof t === "number") {
                                        if (t < 10000000000) t *= 1000
                                        d = new Date(t)
                                    } else {
                                        if (typeof t === "string" && t.indexOf("Z") === -1 && t.indexOf("+") === -1) {
                                            t += "Z"
                                        }
                                        d = new Date(t)
                                    }
                                    if (isNaN(d.getTime())) return "00:00"
                                    var h = d.getHours(); var m = d.getMinutes()
                                    return (h < 10 ? "0" + h : h) + ":" + (m < 10 ? "0" + m : m)
                                }
                                color: isMe ? Qt.rgba(1, 1, 1, 0.7) : appTheme.textHint
                                font.pixelSize: 11
                                font.family: "Segoe UI"
                                Layout.alignment: Qt.AlignVCenter
                            }

                            IconImage {
                                visible: isMe && chatAreaRoot.activeChatName !== "Избранное"
                                Layout.alignment: Qt.AlignVCenter
                                width: 14; height: 14
                                sourceSize: Qt.size(14, 14)
                                color: Qt.rgba(1, 1, 1, 0.7)
                                source: (model.is_read) 
                                        ? "qrc:/messenger_client_uri/assets/icons/check_double.svg" 
                                        : "qrc:/messenger_client_uri/assets/icons/check_single.svg"
                            }
                        }
                    }
                }   
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(250, Math.max(60, messageInput.contentHeight + 20))
            color: appTheme.bgPanel

            Rectangle {
                width: parent.width
                height: 1
                color: appTheme.bgMain
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
                    width: VoiceLayer.isRecording ? 0 : 36
                    height: 36
                    color: "transparent"
                    visible: isChatActive && !VoiceLayer.isRecording

                    IconImage {
                        source: clipArea.containsMouse 
                                ? "qrc:/messenger_client_uri/assets/icons/clip_active.svg"
                                : "qrc:/messenger_client_uri/assets/icons/clip.svg"
                        color: clipArea.containsMouse ? appTheme.accent : appTheme.textHint
                        width: 28
                        height: 28
                        anchors.centerIn: parent
                        sourceSize: Qt.size(28, 28)
                    }
                    MouseArea {
                        id: clipArea
                        anchors.fill: parent
                        hoverEnabled: !VoiceLayer.isRecording
                        enabled: !VoiceLayer.isRecording
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
                        visible: !VoiceLayer.isRecording

                        TextEdit {
                            id: messageInput
                            width: parent.width
                            color: appTheme.textMain
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
                                        if (chatAreaRoot.activeChatType === "secret") {
                                            var newMsgId = SecretChatManager.sendSecretMessage(activeChatId, text.trim(), "text", []);
                                            
                                            // Временно запрашиваем перерисовку вручную, пока нет сигналов
                                            if (newMsgId !== "") {
                                                ChatLayer.fetchChats(); 
                                                chatAreaRoot.loadSecretHistory(activeChatId, 0);
                                            }
                                        } else {
                                            ChatLayer.sendMessage(activeChatId, text.trim());
                                        }
                                        messageInput.text = "";
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        anchors.fill: parent
                        color: appTheme.bgPanel
                        visible: VoiceLayer.isRecording
                        z: 10

                        focus: visible
                        Keys.onReturnPressed: VoiceLayer.stopRecordingAndSend()
                        Keys.onEnterPressed: VoiceLayer.stopRecordingAndSend()
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 2
                            anchors.rightMargin: 10
                            spacing: 12

                            Rectangle {
                                width: 10; height: 10; radius: 5
                                color: "#ff4d4d"
                                Layout.leftMargin: 10
                                SequentialAnimation on opacity {
                                    loops: Animation.Infinite
                                    running: VoiceLayer.isRecording
                                    NumberAnimation { to: 0.1; duration: 700 }
                                    NumberAnimation { to: 1.0; duration: 700 }
                                }
                            }
                            
                            Text {
                                text: VoiceLayer.recordingDuration
                                color: appTheme.textMain
                                font.pixelSize: 16
                                font.family: "Segoe UI"
                            }
                            
                            Item { Layout.fillWidth: true }
                            
                            Text {
                                text: "Отмена"
                                color: appTheme.textMain
                                font.pixelSize: 15
                                font.family: "Segoe UI"
                                
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: VoiceLayer.cancelRecording()
                                }
                            }
                        }
                    }

                    Text {
                        text: isChatActive ? "Сообщение..." : ""
                        color: "#8a96a3"
                        font.family: "Segoe UI"
                        font.pixelSize: 16
                        visible: !messageInput.text && !VoiceLayer.isRecording
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
                        source: VoiceLayer.isRecording 
                            ? "qrc:/messenger_client_uri/assets/icons/send_active.svg" 
                            : (messageInput.text.trim() === "" 
                                ? "qrc:/messenger_client_uri/assets/icons/mic.svg"
                                : "qrc:/messenger_client_uri/assets/icons/send_active.svg")
                        
                        width: 24; height: 24; sourceSize: Qt.size(24, 24)
                        anchors.centerIn: parent
                        anchors.horizontalCenterOffset: (messageInput.text.trim() === "" && !VoiceLayer.isRecording) ? 0 : 2

                        Behavior on opacity { NumberAnimation { duration: 150 } }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (messageInput.text.trim() !== "") {
                                if (isChatActive) {
                                    if (chatAreaRoot.activeChatType === "secret") {
                                        ChatLayer.sendSecretMessage(activeChatId, messageInput.text.trim());
                                    } else {
                                        ChatLayer.sendMessage(activeChatId, messageInput.text.trim());
                                    }
                                    messageInput.text = "";
                                }
                            } else {
                                if (VoiceLayer.isRecording) {
                                    VoiceLayer.stopRecordingAndSend();
                                } else {
                                    VoiceLayer.startRecording();
                                }
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

    Popup {
        id: cancelVoiceDialog
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 320
        height: 160
        modal: true
        dim: true
        closePolicy: Popup.NoAutoClose
        Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.5) }
        
        background: Rectangle {
            color: "#1c2733"
            radius: 10
        }
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 20
            
            Text {
                text: "Вы точно хотите прекратить запись и удалить записанное голосовое сообщение?"
                color: "white"
                font.pixelSize: 16
                font.family: "Segoe UI"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            
            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 20
                
                Rectangle {
                    width: 70
                    height: 36
                    radius: 8
                    color: cancelMouseArea.pressed ? Qt.alpha(appTheme.textMain, 0.1) : (cancelMouseArea.containsMouse ? Qt.alpha(appTheme.textMain, 0.05) : "transparent")

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
                        onClicked: cancelVoiceDialog.close()
                    }
                }
                
                Rectangle {
                    width: 80
                    height: 36
                    radius: 8
                    color: deleteMouseArea.containsMouse ? "#3d2a2d" : "transparent"

                    Text {
                        text: "Удалить"
                        color: "#ff4d4d"
                        font.pixelSize: 15
                        font.bold: true
                        anchors.centerIn: parent
                    }
                    
                    MouseArea {
                        id: deleteMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onPressed: parent.color = "#33181a"
                        onReleased: parent.color = deleteMouseArea.containsMouse ? "#3d2a2d" : "transparent"
                        onClicked: {
                            VoiceLayer.cancelRecording()
                            cancelVoiceDialog.close()
                        }
                    }
                }
            }
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
            messageInput.text = ""
            isUploading = true

            var msgType = (asFile || fileType === "document") ? "text" : "media"
            MediaLayer.uploadFile(activeChatId, filePath, asFile, caption, msgType)
        }
        onCancelRequested: {}
    }

    Popup {
        id: fullScreenImagePreview
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: parent.width
        height: parent.height
        modal: true
        dim: true
        focus: true
        closePolicy: Popup.CloseOnEscape
        Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.8) }
        
        property string imageUrl: ""
        
        function openWith(url) {
            imageUrl = url;
            open();
        }

        background: Rectangle {
            color: "transparent"
        }
        
        Image {
            id: previewImage
            anchors.fill: parent
            anchors.margins: 40
            source: fullScreenImagePreview.imageUrl
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            cache: false
            
            MouseArea {
                anchors.centerIn: parent
                width: previewImage.paintedWidth
                height: previewImage.paintedHeight
                onClicked: {}
            }
        }
        
        Rectangle {
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 20
            width: 40
            height: 40
            radius: 20
            color: closeImagePreviewArea.containsMouse ? Qt.rgba(1, 1, 1, 0.2) : Qt.rgba(0, 0, 0, 0.5)
            
            Text {
                anchors.centerIn: parent
                text: "✕"
                color: "white"
                font.pixelSize: 20
                font.family: "Segoe UI"
            }
            
            MouseArea {
                id: closeImagePreviewArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: fullScreenImagePreview.close()
            }
        }
    }

    Window {
        id: fullScreenVideoPreview
        title: "Video Preview"
        visible: false
        color: "black"
        
        property string videoUrl: ""
        property bool showControls: true
        
        function openWith(url) {
            videoUrl = url;
            chatAreaRoot.activeFullscreenVideoUrl = url;
            fullScreenVideoPreview.visibility = Window.FullScreen;
            fullVideoPlayer.play();
            showControls = true;
            controlsTimer.restart();
        }
        
        function closePlayer() {
            fullVideoPlayer.stop();
            videoUrl = "";
            chatAreaRoot.activeFullscreenVideoUrl = "";
            hide();
        }

        Shortcut {
            sequence: "Escape"
            onActivated: fullScreenVideoPreview.closePlayer()
        }

        Shortcut {
            sequence: "Space"
            onActivated: {
                if (fullVideoPlayer.playbackState === MediaPlayer.PlayingState) {
                    fullVideoPlayer.pause();
                } else {
                    fullVideoPlayer.play();
                }
                fullScreenVideoPreview.showControls = true;
                controlsTimer.restart();
            }
        }

        Shortcut {
            sequence: "Left"
            onActivated: {
                fullVideoPlayer.position = Math.max(0, fullVideoPlayer.position - 10000);
                fullScreenVideoPreview.showControls = true;
                controlsTimer.restart();
            }
        }

        Shortcut {
            sequence: "Right"
            onActivated: {
                fullVideoPlayer.position = Math.min(fullVideoPlayer.duration, fullVideoPlayer.position + 10000);
                fullScreenVideoPreview.showControls = true;
                controlsTimer.restart();
            }
        }

        Shortcut {
            sequence: "Up"
            onActivated: {
                videoVolumeSlider.value = Math.min(1.0, videoVolumeSlider.value + 0.1);
                fullScreenVideoPreview.showControls = true;
                controlsTimer.restart();
            }
        }

        Shortcut {
            sequence: "Down"
            onActivated: {
                videoVolumeSlider.value = Math.max(0.0, videoVolumeSlider.value - 0.1);
                fullScreenVideoPreview.showControls = true;
                controlsTimer.restart();
            }
        }

        VideoOutput {
            id: fullVideoOut
            anchors.fill: parent
            fillMode: VideoOutput.PreserveAspectFit
        }

        Timer {
            id: controlsTimer
            interval: 5000
            repeat: false
            onTriggered: {
                if (fullVideoPlayer.playbackState === MediaPlayer.PlayingState) {
                    fullScreenVideoPreview.showControls = false;
                }
            }
        }
        
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onPositionChanged: {
                fullScreenVideoPreview.showControls = true;
                controlsTimer.restart();
            }
            onClicked: {
                if (fullVideoPlayer.playbackState === MediaPlayer.PlayingState) {
                    fullVideoPlayer.pause();
                } else {
                    fullVideoPlayer.play();
                }
                fullScreenVideoPreview.showControls = true;
                controlsTimer.restart();
            }
        }

        Rectangle {
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 20
            width: 44; height: 44; radius: 22
            color: "#80000000"
            z: 200
            opacity: fullScreenVideoPreview.showControls ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 300 } }

            Text { 
                anchors.centerIn: parent; text: "✕"; color: "white"; font.pixelSize: 20
            }
            MouseArea {
                anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                onClicked: fullScreenVideoPreview.closePlayer()
            }
        }

        Rectangle {
            id: controlBar
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottomMargin: 40
            width: Math.min(900, parent.width - 100)
            height: 110 
            radius: 20
            color: "#EE1c2733"
            z: 200
            
            opacity: (fullScreenVideoPreview.showControls || fullVideoPlayer.playbackState !== MediaPlayer.PlayingState) ? 1 : 0
            visible: opacity > 0
            Behavior on opacity { NumberAnimation { duration: 300 } }
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 12
                
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 54
                    
                    RowLayout {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 12
                        
                        IconImage {
                            source: "qrc:/messenger_client_uri/assets/icons/volume.svg"
                            width: 24; height: 24; sourceSize: Qt.size(24, 24)
                            color: "white"
                        }
                        Slider {
                            id: videoVolumeSlider
                            Layout.preferredWidth: 100
                            from: 0; to: 1.0; value: 0.8
                            background: Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                implicitWidth: 100; implicitHeight: 4; radius: 2; color: "#30ffffff"
                                
                                Rectangle { 
                                    width: videoVolumeSlider.visualPosition * parent.width
                                    height: parent.height
                                    color: appTheme.accent
                                    radius: 2 
                                }
                            }
                            handle: Item {}
                        }
                    }
                    
                    Rectangle {
                        anchors.centerIn: parent
                        width: 54; height: 54; radius: 27
                        color: playMouse.containsMouse ? "white" : "transparent"
                        Behavior on color { ColorAnimation { duration: 150 } }
                        
                        IconImage {
                            anchors.centerIn: parent
                            source: fullVideoPlayer.playbackState === MediaPlayer.PlayingState 
                                ? "qrc:/messenger_client_uri/assets/icons/pause.svg" 
                                : "qrc:/messenger_client_uri/assets/icons/play.svg"
                            width: 40; height: 40; sourceSize: Qt.size(40, 40)
                            color: playMouse.containsMouse ? "black" : "white"
                            Behavior on color { ColorAnimation { duration: 150 } }
                        }
                        
                        MouseArea {
                            id: playMouse
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor; hoverEnabled: true
                            onClicked: fullVideoPlayer.playbackState === MediaPlayer.PlayingState ? fullVideoPlayer.pause() : fullVideoPlayer.play()
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true; spacing: 15
                    Text { 
                        text: chatAreaRoot.formatGlobalTime(fullVideoPlayer.position)
                        color: "white"; font.pixelSize: 13; font.family: "Segoe UI"
                    }
                    Slider {
                        id: videoSlider
                        Layout.fillWidth: true
                        from: 0
                        to: Math.max(1, fullVideoPlayer.duration)
                        value: videoSlider.pressed ? videoSlider.value : fullVideoPlayer.position
                        live: true

                        onValueChanged: {
                            if (pressed) {
                                fullVideoPlayer.position = value
                            }
                        }

                        background: Rectangle {
                            anchors.verticalCenter: parent.verticalCenter
                            implicitHeight: 4
                            radius: 2
                            color: "#30ffffff"
                            Rectangle {
                                width: videoSlider.visualPosition * parent.width
                                height: parent.height
                                color: appTheme.accent
                                radius: 2
                            }
                        }
                        handle: Item {}
                    }
                    Text {
                        text: "-" + chatAreaRoot.formatGlobalTime(Math.max(0, fullVideoPlayer.duration - fullVideoPlayer.position))
                        color: "white"; font.pixelSize: 13; font.family: "Segoe UI"
                    }
                }
            }
        }

        MediaPlayer {
            id: fullVideoPlayer
            source: fullScreenVideoPreview.videoUrl
            audioOutput: AudioOutput { id: fullAudioOut; volume: videoVolumeSlider.value }
            videoOutput: fullVideoOut
        }
    }

    Popup {
        id: groupInfoPopup
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 380
        height: Math.min(700, parent.height * 0.9)
        modal: true
        dim: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.5) }
        background: Rectangle { color: appTheme.bgPanel; radius: 10 }

        property var membersModel: []
        property string currentUserRole: "member"

        Connections {
            target: ChatLayer
            function onChatMembersLoaded(members) {
                groupInfoPopup.membersModel = members
                for(var i=0; i<members.length; i++) {
                    if (String(members[i].user_id) === String(AppState.userId)) {
                        groupInfoPopup.currentUserRole = members[i].role
                        break
                    }
                }
            }
            function onChatInfoLoaded(chat) {
                if (chat.description !== undefined) {
                    chatAreaRoot.activeChatDescription = chat.description;
                }
                if (chat.name || chat.title) {
                    chatAreaRoot.activeChatName = chat.name || chat.title;
                }
            }
        }

        function getPluralMembers(count) {
            var m10 = count % 10; var m100 = count % 100
            if (m10 === 1 && m100 !== 11) return count + " УЧАСТНИК"
            if (m10 >= 2 && m10 <= 4 && (m100 < 10 || m100 >= 20)) return count + " УЧАСТНИКА"
            return count + " УЧАСТНИКОВ"
        }

        Item {
            anchors.fill: parent

            RowLayout {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 15
                
                IconImage {
                    visible: groupInfoPopup.currentUserRole === "admin" || groupInfoPopup.currentUserRole === "owner"
                    source: "qrc:/messenger_client_uri/assets/icons/gear.svg"
                    color: appTheme.textHint
                    width: 24
                    height: 24
                    sourceSize: Qt.size(24, 24)
                    
                    MouseArea {
                        anchors.fill: parent
                        anchors.margins: -10
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            groupInfoPopup.close()
                            groupSettingsPopup.open()
                        }
                    }
                }
                
                Item { Layout.fillWidth: true }
                
                Text {
                    text: "✕"
                    color: "#8a96a3"
                    font.pixelSize: 20
                    MouseArea {
                        anchors.fill: parent
                        anchors.margins: -10
                        cursorShape: Qt.PointingHandCursor
                        onClicked: groupInfoPopup.close()
                    }
                }
            }

            ColumnLayout {
                anchors.top: parent.top
                anchors.topMargin: 50
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                spacing: 0

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 10
                    
                    Rectangle {
                        width: 90
                        height: 90
                        radius: 45
                        color: "#5eb5f7"
                        Layout.alignment: Qt.AlignHCenter
                        
                        Text {
                            anchors.centerIn: parent
                            text: activeChatName ? activeChatName.charAt(0).toUpperCase() : "?"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 36
                        }
                    }
                    
                    Text {
                        text: activeChatName
                        color: appTheme.textMain
                        font.pixelSize: 20
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                        font.family: "Segoe UI"
                    }
                    
                    Text {
                        visible: activeChatDescription !== ""
                        text: activeChatDescription
                        color: appTheme.textMain
                        font.pixelSize: 15
                        font.family: "Segoe UI"
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                        Layout.leftMargin: 20
                        Layout.rightMargin: 20
                        Layout.topMargin: 20
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.margins: 15
                    Layout.topMargin: 20
                    
                    Text {
                        text: groupInfoPopup.getPluralMembers(groupInfoPopup.membersModel.length)
                        color: "#8a96a3"
                        font.pixelSize: 13
                        font.bold: true
                    }
                    
                    Item { Layout.fillWidth: true }
                    
                    IconImage {
                        visible: groupInfoPopup.currentUserRole === "admin" || groupInfoPopup.currentUserRole === "owner"
                        source: "qrc:/messenger_client_uri/assets/icons/person_plus.svg"
                        color: appTheme.textHint
                        width: 24
                        height: 24
                        sourceSize: Qt.size(24, 24)
                        
                        MouseArea {
                            anchors.fill: parent
                            anchors.margins: -10
                            cursorShape: Qt.PointingHandCursor
                            onClicked: addMemberPopup.open()
                        }
                    }
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: groupInfoPopup.membersModel
                    ScrollBar.vertical: ScrollBar {}
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: Rectangle {
                        width: parent.width
                        height: 50
                        color: memberHover.containsMouse ? appTheme.hoverColor : "transparent"
                        
                        property string mName: {
                            if (modelData.display_name) return modelData.display_name
                            if (modelData.member_info && modelData.member_info.display_name) return modelData.member_info.display_name
                            if (modelData.handle) return "@" + modelData.handle
                            if (modelData.member_info && modelData.member_info.handle) return "@" + modelData.member_info.handle
                            return "User " + modelData.user_id
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 15
                            anchors.rightMargin: 15
                            spacing: 15
                            
                            Rectangle {
                                width: 36
                                height: 36
                                radius: 18
                                color: "#4a90d9"
                                
                                Text {
                                    anchors.centerIn: parent
                                    text: parent.parent.parent.mName.charAt(0).toUpperCase()
                                    color: "white"
                                    font.bold: true
                                    font.pixelSize: 15
                                }
                            }
                            
                            Text {
                                text: parent.parent.mName
                                color: appTheme.textMain
                                font.pixelSize: 15
                                Layout.fillWidth: true
                                font.family: "Segoe UI"
                                elide: Text.ElideRight
                            }
                            
                            Rectangle {
                                visible: modelData.role === "owner" || modelData.role === "admin"
                                color: "transparent"
                                border.color: modelData.role === "owner" ? "#5eb5f7" : "#8a96a3"
                                radius: 4
                                width: roleText.implicitWidth + 12
                                height: 20
                                
                                Text {
                                    id: roleText
                                    anchors.centerIn: parent
                                    text: modelData.role === "owner" ? "владелец" : "админ"
                                    color: parent.border.color
                                    font.pixelSize: 12
                                    font.family: "Segoe UI"
                                }
                            }
                        }

                        MouseArea { 
                            id: memberHover
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.RightButton
                            onClicked: (mouse) => {
                                if (mouse.button === Qt.RightButton) {
                                    if (String(modelData.user_id) === String(AppState.userId)) return

                                    var isOwner = groupInfoPopup.currentUserRole === "owner"
                                    var isAdminClickingMember = groupInfoPopup.currentUserRole === "admin" && modelData.role === "member"

                                    if (isOwner || isAdminClickingMember) {
                                        contextMenu.popup()
                                    }
                                }
                            }
                        }

                        Menu {
                            id: contextMenu
                            width: 220
                            background: Rectangle { color: appTheme.bgHeader; radius: 6; border.color: appTheme.bgMain; border.width: 1 }

                            // Кнопка "Передать права" (Видит только текущий владелец, и только на обычных мемберах или админах)
                            MenuItem {
                                id: ownerItem
                                visible: groupInfoPopup.currentUserRole === "owner"
                                height: visible ? 40 : 0
                                contentItem: RowLayout {
                                    spacing: 10
                                    
                                    IconImage {
                                        source: "qrc:/messenger_client_uri/assets/icons/owner_shield.svg"
                                        color: appTheme.textHint
                                        width: 20; height: 20; sourceSize: Qt.size(20, 20) 
                                    }

                                    Text {
                                        text: "Передать права"
                                        color: appTheme.textMain
                                        font.pixelSize: 14
                                        font.family: "Segoe UI"
                                        Layout.fillWidth: true 
                                    }
                                }
                                background: Rectangle { color: ownerItem.highlighted ? appTheme.hoverColor : "transparent"; radius: 4 }
                                onTriggered: ChatLayer.changeMemberRole(activeChatId, modelData.user_id, "owner")
                            }

                            // Кнопка "Назначить админом / Разжаловать" (Видит только текущий владелец)
                            MenuItem {
                                id: adminItem
                                visible: groupInfoPopup.currentUserRole === "owner"
                                height: visible ? 40 : 0
                                contentItem: RowLayout {
                                    spacing: 10
                                    
                                    IconImage {
                                        source: "qrc:/messenger_client_uri/assets/icons/admin_shield.svg"
                                        color: appTheme.textHint
                                        width: 20; height: 20; sourceSize: Qt.size(20, 20) 
                                    }

                                    Text {
                                        text: modelData.role === "admin" ? "Разжаловать" : "Назначить админом"
                                        color: appTheme.textMain
                                        font.pixelSize: 14
                                        font.family: "Segoe UI"
                                        Layout.fillWidth: true 
                                    }
                                }
                                background: Rectangle { color: adminItem.highlighted ? appTheme.hoverColor : "transparent"; radius: 4 }
                                onTriggered: ChatLayer.changeMemberRole(activeChatId, modelData.user_id, modelData.role === "admin" ? "member" : "admin")
                            }

                            // Кнопка "Удалить из группы" (Видят владелец и админы)
                            MenuItem {
                                id: deleteItem
                                height: 40
                                contentItem: RowLayout {
                                    spacing: 10
                                    
                                    IconImage {
                                        source: "qrc:/messenger_client_uri/assets/icons/delete_user.svg"
                                        color: "#ff4d4f"
                                        width: 20; height: 20; sourceSize: Qt.size(20, 20) 
                                    }

                                    Text {
                                        text: "Удалить из группы"
                                        color: "#ff4d4f"
                                        font.pixelSize: 14
                                        font.family: "Segoe UI"
                                        Layout.fillWidth: true 
                                    }
                                }
                                background: Rectangle { color: deleteItem.highlighted ? appTheme.hoverColor : "transparent"; radius: 4 }
                                onTriggered: ChatLayer.removeChatMember(activeChatId, modelData.user_id, true) // true = обновить список после удаления
                            }
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    height: 60
                    visible: groupInfoPopup.currentUserRole !== "owner" 
                    
                    Rectangle {
                        anchors.centerIn: parent
                        width: 200
                        height: 40
                        radius: 8
                        color: leaveHover.pressed ? "#a22026" : (leaveHover.containsMouse ? "#d9363e" : "#ff4d4f")
                        Behavior on color { ColorAnimation { duration: 150 } }
                        
                        Text {
                            anchors.centerIn: parent
                            text: "Покинуть группу"
                            color: "white"
                            font.pixelSize: 15
                            font.family: "Segoe UI"
                            font.bold: true
                        }
                        
                        MouseArea {
                            id: leaveHover
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: { ChatLayer.removeChatMember(activeChatId, AppState.userId, false); groupInfoPopup.close(); chatAreaRoot.activeChatId = "" }
                        }
                    }
                }
            }
        }
    }

    Popup {
        id: addMemberPopup
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 380
        height: 450
        modal: true
        dim: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.5) }
        background: Rectangle { color: appTheme.bgPanel; radius: 10 }
        
        padding: 0 

        property var selectedUsers: [] 
        readonly property int maxMembers: 50
        property int searchResultsCount: 0 

        onClosed: {
            addSearchInput.text = ""
            selectedUsers = []
            addMemberModel.clear()
        }

        ListModel { id: addMemberModel }

        Connections {
            target: ChatLayer
            function onUsersFound(users) {
                if (addMemberPopup.visible && addSearchInput.text.trim() !== "") {
                    addMemberPopup.updateList(users)
                }
            }
        }

        function toggleUser(userObj) {
            var idx = -1
            for (var i = 0; i < selectedUsers.length; i++) {
                if (String(selectedUsers[i].id) === String(userObj.id)) {
                    idx = i
                    break
                }
            }

            var newArr = selectedUsers.slice()
            if (idx !== -1) {
                newArr.splice(idx, 1)
            } else {
                var totalAfterAdd = groupInfoPopup.membersModel.length + newArr.length + 1
                if (totalAfterAdd <= maxMembers) {
                    newArr.push(userObj)
                }
            }
            selectedUsers = newArr

            if (addSearchInput.text !== "") {
                addSearchInput.text = ""
            } else {
                updateList(null)
            }
        }

        function updateList(apiResults) {
            addMemberModel.clear()
            addMemberPopup.searchResultsCount = 0
            
            for (var i = 0; i < selectedUsers.length; i++) {
                addMemberModel.append({
                    "userId": selectedUsers[i].id,
                    "displayName": selectedUsers[i].name,
                    "handleStr": selectedUsers[i].handle,
                    "isSelected": true
                })
            }

            if (!apiResults) return
            for (var j = 0; j < apiResults.length; j++) {
                var sId = apiResults[j].id
                if (!sId || String(sId) === String(AppState.userId)) continue
                
                var alreadySelected = false
                for (var k = 0; k < selectedUsers.length; k++) {
                    if (String(selectedUsers[k].id) === String(sId)) {
                        alreadySelected = true
                        break 
                    }
                }
                
                var alreadyInGroup = false
                for (var m = 0; m < groupInfoPopup.membersModel.length; m++) {
                    if (String(groupInfoPopup.membersModel[m].user_id) === String(sId)) {
                        alreadyInGroup = true
                        break
                    }
                }
                
                if (!alreadySelected && !alreadyInGroup) {
                    addMemberModel.append({
                        "userId": sId,
                        "displayName": apiResults[j].display_name || "",
                        "handleStr": apiResults[j].handle || "",
                        "isSelected": false
                    })
                    addMemberPopup.searchResultsCount++
                }
            }
        }

        contentItem: ColumnLayout {
            spacing: 0
            
            // ШАПКА
            RowLayout {
                Layout.fillWidth: true; Layout.margins: 20; Layout.bottomMargin: 10

                Text {
                    text: "Добавить участников"
                    color: appTheme.textMain
                    font.pixelSize: 18
                    font.bold: true
                    font.family: "Segoe UI" 
                }

                Text { 
                    property int totalPreview: groupInfoPopup.membersModel.length + addMemberPopup.selectedUsers.length
                    text: totalPreview + " / " + addMemberPopup.maxMembers
                    color: totalPreview > addMemberPopup.maxMembers ? "#ff4d4d" : "#8a96a3"
                    font.pixelSize: 15
                    font.family: "Segoe UI"
                    Layout.leftMargin: 10
                }
            }

            // ПОИСК
            Rectangle {
                Layout.fillWidth: true; Layout.margins: 15; Layout.topMargin: 0
                height: 36
                color: appTheme.bgInput
                radius: 18

                TextInput {
                    id: addSearchInput
                    anchors.fill: parent; anchors.leftMargin: 15; anchors.rightMargin: 15
                    verticalAlignment: TextInput.AlignVCenter
                    topPadding: 0; bottomPadding: 0
                    font.pixelSize: 14
                    font.family: "Segoe UI"
                    color: appTheme.textMain
                    clip: true
                    
                    Text {
                        text: "Поиск"
                        color: appTheme.textHint
                        font.family: "Segoe UI"
                        visible: !parent.text
                        anchors.verticalCenter: parent.verticalCenter 
                    }

                    Timer {
                        id: amTimer
                        interval: 400
                        repeat: false
                        onTriggered: ChatLayer.searchUsers(addSearchInput.text) 
                    }
                    onTextChanged: { 
                        if (text.trim() === "") { 
                            amTimer.stop()
                            addMemberPopup.updateList(null)
                        } else amTimer.restart() 
                    }
                }
            }

            // СПИСОК
            ListView {
                id: addList
                Layout.fillWidth: true; Layout.fillHeight: true
                clip: true
                model: addMemberModel
                ScrollBar.vertical: ScrollBar {} boundsBehavior: Flickable.StopAtBounds

                footer: Item {
                    width: addList.width
                    height: addList.count === 0 ? addList.height : 60
                    visible: addMemberPopup.searchResultsCount === 0 && addSearchInput.text.trim() !== ""

                    Text {
                        text: "Нет результатов..."
                        color: "#8a96a3"
                        font.pixelSize: 15
                        font.family: "Segoe UI"
                        anchors.centerIn: parent 
                    }
                }

                delegate: Rectangle {
                    width: ListView.view ? ListView.view.width : 0
                    height: 60
                    color: amHover.containsMouse ? appTheme.hoverColor : "transparent"

                    RowLayout {
                        anchors.fill: parent; anchors.margins: 15; spacing: 15

                        Rectangle {
                            width: 40; height: 40; radius: 20
                            color: "#4a90d9"
                            Layout.alignment: Qt.AlignVCenter
                            
                            Text {
                                anchors.centerIn: parent
                                color: "white"
                                font.bold: true
                                font.pixelSize: 16
                                text: model.displayName ? model.displayName.charAt(0).toUpperCase() : "?" 
                            }
                            
                            Rectangle {
                                visible: model.isSelected
                                width: 16; height: 16; radius: 8
                                color: "#5eb5f7"
                                anchors.bottom: parent.bottom; anchors.right: parent.right
                                border.color: "#1c242f"; border.width: 2
                                
                                Text {
                                    anchors.centerIn: parent
                                    text: "✓"
                                    color: "white"
                                    font.pixelSize: 10
                                    font.bold: true 
                                }
                            } 
                        }
                        
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 2
                            Layout.alignment: Qt.AlignVCenter
                            
                            Text {
                                text: model.displayName
                                color: appTheme.textMain
                                font.pixelSize: 15
                                font.family: "Segoe UI"
                                font.bold: true
                                Layout.fillWidth: true
                                elide: Text.ElideRight 
                            } 
                            
                            Text {
                                visible: model.handleStr !== ""
                                text: "@" + model.handleStr
                                color: "#8a96a3"
                                font.pixelSize: 13
                                font.family: "Segoe UI"
                                Layout.fillWidth: true
                                elide: Text.ElideRight 
                            } 
                        }
                    }

                    MouseArea {
                        id: amHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: addMemberPopup.toggleUser({ 
                            "id": model.userId,
                            "name": model.displayName, 
                            "handle": model.handleStr 
                        }) 
                    }
                }
            }

            // НИЖНИЕ КНОПКИ
            Rectangle {
                Layout.fillWidth: true
                height: 60
                color: appTheme.bgPanel
                
                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#151b23"
                    anchors.top: parent.top 
                }

                RowLayout {
                    anchors.fill: parent; anchors.rightMargin: 15; spacing: 15
                    Item { Layout.fillWidth: true } 

                    Rectangle {
                        width: 80; height: 36; radius: 6
                        color: cancelAddHover.pressed ? Qt.alpha(appTheme.textMain, 0.1) : (cancelAddHover.containsMouse ? Qt.alpha(appTheme.textMain, 0.05) : "transparent")
                        Behavior on color { ColorAnimation { duration: 150 } }
                        
                        Text {
                            anchors.centerIn: parent
                            text: "Отмена"
                            color: "#5eb5f7"
                            font.pixelSize: 15
                            font.bold: true
                            font.family: "Segoe UI" 
                        }

                        MouseArea {
                            id: cancelAddHover
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: addMemberPopup.close() 
                        }
                    }

                    Rectangle {
                        width: 100; height: 36; radius: 6
                        property bool hasUsers: addMemberPopup.selectedUsers.length > 0
                        color: (addHover.pressed && hasUsers) ? Qt.alpha(appTheme.accent, 0.2) : ((addHover.containsMouse && hasUsers) ? Qt.alpha(appTheme.accent, 0.1) : "transparent")
                        opacity: hasUsers ? 1.0 : 0.5
                        Behavior on color { ColorAnimation { duration: 150 } }
                        Behavior on opacity { NumberAnimation { duration: 150 } }
                        
                        Text {
                            anchors.centerIn: parent
                            text: "Добавить"
                            color: "#5eb5f7"
                            font.pixelSize: 15
                            font.bold: true
                            font.family: "Segoe UI" 
                        }

                        MouseArea {
                            id: addHover
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: parent.hasUsers ? Qt.PointingHandCursor : Qt.ArrowCursor

                            onClicked: {
                                if (parent.hasUsers) {
                                    for (var i = 0; i < addMemberPopup.selectedUsers.length; i++) {
                                        ChatLayer.addChatMember(activeChatId, addMemberPopup.selectedUsers[i].id, "member")
                                    }
                                    addMemberPopup.close()
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Popup {
        id: groupSettingsPopup
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 380
        height: 360
        modal: true
        dim: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.5) }
        background: Rectangle { color: appTheme.bgPanel; radius: 10 }

        onOpened: {
            editNameField.text = activeChatName
            editDescField.text = activeChatDescription
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0
            
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                Text {
                    text: "Настройки группы"
                    color: appTheme.textMain
                    font.pixelSize: 18
                    font.bold: true
                    font.family: "Segoe UI"
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 20
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 25
                spacing: 20
                
                Rectangle {
                    width: 64
                    height: 64
                    radius: 32
                    color: "#5eb5f7"
                    Layout.alignment: Qt.AlignTop
                    Image {
                        anchors.centerIn: parent
                        width: 32; height: 32; sourceSize: Qt.size(32, 32)
                        source: "qrc:/messenger_client_uri/assets/icons/camera.svg"
                    }
                }
                
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    
                    Item {
                        Layout.fillWidth: true
                        height: 50

                        Text {
                            text: "Название группы"
                            color: editNameField.activeFocus ? appTheme.accent : appTheme.textHint
                            font.pixelSize: (editNameField.length > 0 || editNameField.activeFocus) ? 12 : 16
                            font.family: "Segoe UI"
                            Behavior on font.pixelSize { NumberAnimation { duration: 150 } }
                            Behavior on anchors.bottomMargin { NumberAnimation { duration: 150 } }
                            anchors.left: parent.left
                            anchors.bottom: editNameField.top
                            anchors.bottomMargin: (editNameField.length > 0 || editNameField.activeFocus) ? 2 : -28
                        }

                        TextField {
                            id: editNameField
                            anchors.bottom: parent.bottom
                            anchors.left: parent.left
                            anchors.right: parent.right
                            color: appTheme.textMain
                            font.pixelSize: 16
                            font.family: "Segoe UI"
                            maximumLength: 29
                            clip: true

                            background: Item {
                                Rectangle {
                                    anchors.bottom: parent.bottom
                                    width: parent.width
                                    height: editNameField.activeFocus ? 2 : 1
                                    color: editNameField.activeFocus ? appTheme.accent : appTheme.textHint
                                    Behavior on height { NumberAnimation { duration: 100 } }
                                    Behavior on color { ColorAnimation { duration: 150 } }
                                }
                            }
                        }
                    }
                    
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 100
                        Layout.topMargin: 10

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.IBeamCursor
                            onClicked: editDescField.forceActiveFocus()
                        }

                        Text {
                            text: "Описание (необязательно)"
                            color: editDescField.activeFocus ? appTheme.accent : appTheme.textHint
                            font.pixelSize: (editDescField.text.length > 0 || editDescField.activeFocus) ? 12 : 16
                            font.family: "Segoe UI"
                            Behavior on font.pixelSize { NumberAnimation { duration: 150 } }
                            Behavior on anchors.topMargin { NumberAnimation { duration: 150 } }
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.topMargin: (editDescField.text.length > 0 || editDescField.activeFocus) ? 0 : 20
                            
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.IBeamCursor
                                onClicked: editDescField.forceActiveFocus()
                            }
                        }

                        Flickable {
                            id: descFlickable
                            anchors.top: parent.top
                            anchors.topMargin: 24
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 8
                            anchors.left: parent.left
                            anchors.right: parent.right
                            clip: true
                            contentWidth: width
                            contentHeight: editDescField.contentHeight
                            boundsBehavior: Flickable.StopAtBounds

                            ScrollBar.vertical: ScrollBar {
                                width: 4
                                policy: descFlickable.contentHeight > descFlickable.height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                            }

                            TextEdit {
                                id: editDescField
                                width: parent.width
                                color: appTheme.textMain
                                font.pixelSize: 16
                                font.family: "Segoe UI"
                                wrapMode: TextEdit.Wrap
                                onTextChanged: {
                                    if (length > 400) {
                                        var cursor = cursorPosition;
                                        text = text.substring(0, 400)
                                        cursorPosition = Math.min(cursor, 400)
                                    }
                                }
                                onCursorRectangleChanged: {
                                    var flick = descFlickable
                                    var rect = cursorRectangle
                                    if (rect.y < flick.contentY) {
                                        flick.contentY = rect.y
                                    } else if (rect.y + rect.height > flick.contentY + flick.height) {
                                        flick.contentY = rect.y + rect.height - flick.height
                                    }
                                }
                            }
                        }

                        Rectangle {
                            anchors.bottom: parent.bottom
                            width: parent.width
                            height: editDescField.activeFocus ? 2 : 1
                            color: editDescField.activeFocus ? appTheme.accent : appTheme.textHint
                            Behavior on height { NumberAnimation { duration: 100 } }
                            Behavior on color { ColorAnimation { duration: 150 } }
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 15
                spacing: 15
                
                Item { Layout.fillWidth: true }
                
                Rectangle {
                    width: 80
                    height: 36
                    radius: 6
                    color: (cancelSettingsHover.pressed) ? Qt.alpha(appTheme.accent, 0.2) : ((cancelSettingsHover.containsMouse) ? Qt.alpha(appTheme.accent, 0.1) : "transparent")
                    Behavior on color { ColorAnimation { duration: 150 } }
                    
                    Text {
                        anchors.centerIn: parent
                        text: "Отмена"
                        color: "#5eb5f7"
                        font.pixelSize: 15
                        font.bold: true
                        font.family: "Segoe UI"
                    }
                    
                    MouseArea {
                        id: cancelSettingsHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: groupSettingsPopup.close()
                    }
                }
                
                Rectangle {
                    width: 100
                    height: 36
                    radius: 6
                    property bool isValid: editNameField.text.trim() !== ""
                    color: (saveSettingsHover.pressed && isValid) ? Qt.alpha(appTheme.accent, 0.2) : ((saveSettingsHover.containsMouse && isValid) ? Qt.alpha(appTheme.accent, 0.1) : "transparent")
                    opacity: isValid ? 1.0 : 0.5
                    Behavior on color { ColorAnimation { duration: 150 } }
                    Behavior on opacity { NumberAnimation { duration: 150 } }
                    
                    Text {
                        anchors.centerIn: parent
                        text: "Сохранить"
                        color: "#5eb5f7"
                        font.pixelSize: 15
                        font.bold: true
                        font.family: "Segoe UI"
                    }
                    
                    MouseArea {
                        id: saveSettingsHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: parent.isValid ? Qt.PointingHandCursor : Qt.ArrowCursor

                        onClicked: {
                            if (parent.isValid) {
                                ChatLayer.updateChatInfo(activeChatId, editNameField.text.trim(), editDescField.text.trim())
                                chatAreaRoot.activeChatName = editNameField.text.trim()
                                chatAreaRoot.activeChatDescription = editDescField.text.trim()
                                groupSettingsPopup.close()
                            }
                        }
                    }
                }
            }
        }
    }

    Popup {
        id: directInfoPopup
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 380
        height: 350
        modal: true; dim: true; focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.5) }
        background: Rectangle { color: appTheme.bgPanel; radius: 10 }

        contentItem: Item {
            anchors.fill: parent
            
            RowLayout {
                anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 15
                
                Item { Layout.fillWidth: true }
                
                Text {
                    text: "✕"
                    color: appTheme.textHint
                    font.pixelSize: 20
                    
                    MouseArea {
                        anchors.fill: parent; anchors.margins: -10
                        cursorShape: Qt.PointingHandCursor
                        onClicked: directInfoPopup.close() 
                    }
                }
            }

            ColumnLayout {
                anchors.top: parent.top; anchors.topMargin: 50; anchors.left: parent.left; anchors.right: parent.right; spacing: 10
                
                Rectangle { 
                    width: 90; height: 90; radius: 45
                    color: "#4a90d9"
                    Layout.alignment: Qt.AlignHCenter
                    
                    Text {
                        anchors.centerIn: parent
                        text: activeChatName ? activeChatName.charAt(0).toUpperCase() : "?"
                        color: "white"
                        font.bold: true
                        font.pixelSize: 36 
                    } 
                }
                
                Text {
                    text: activeChatName
                    color: appTheme.textMain
                    font.pixelSize: 20
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                    font.family: "Segoe UI" 
                }
            }

            Rectangle {
                anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 20
                height: 44; radius: 8
                color: secretHover.pressed ? Qt.alpha(appTheme.accent, 0.2) : (secretHover.containsMouse ? Qt.alpha(appTheme.accent, 0.1) : "transparent")
                border.color: appTheme.accent; border.width: 1
                Behavior on color { ColorAnimation { duration: 150 } }
                
                RowLayout {
                    anchors.centerIn: parent; spacing: 10
                    
                    Image {
                        source: "qrc:/messenger_client_uri/assets/icons/person_lock.svg"
                        width: 20; height: 20; sourceSize: Qt.size(20, 20) 
                    }
                    
                    Text {
                        text: "Создать секретный чат"
                        color: appTheme.accent
                        font.pixelSize: 15
                        font.family: "Segoe UI"
                        font.bold: true 
                    }
                }
                
                MouseArea {
                    id: secretHover; anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { directInfoPopup.close(); confirmSecretChatPopup.open() }
                }
            }
        }
    }

    Popup {
        id: confirmSecretChatPopup
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 320
        height: 180
        modal: true; dim: true; focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.5) }
        background: Rectangle { color: appTheme.bgPanel; radius: 10 }

        contentItem: ColumnLayout {
            anchors.fill: parent; anchors.margins: 20; spacing: 20
            
            Text {
                text: "Создать секретный чат с " + activeChatName + "?\n\nСообщения будут зашифрованы и доступны только на этом устройстве."
                color: appTheme.textMain
                font.pixelSize: 15
                font.family: "Segoe UI"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            
            RowLayout {
                Layout.alignment: Qt.AlignRight | Qt.AlignBottom; spacing: 15
                
                Rectangle {
                    width: 80; height: 36; radius: 6
                    color: cancelSecHover.pressed ? Qt.alpha(appTheme.accent, 0.2) : (cancelSecHover.containsMouse ? Qt.alpha(appTheme.accent, 0.1) : "transparent")
                    
                    Text {
                        anchors.centerIn: parent
                        text: "Отмена"
                        color: appTheme.accent
                        font.pixelSize: 15
                        font.bold: true
                        font.family: "Segoe UI" 
                    }
                    
                    MouseArea {
                        id: cancelSecHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: confirmSecretChatPopup.close() 
                    }
                }
                
                Rectangle {
                    width: 90; height: 36; radius: 6
                    color: startSecHover.pressed ? Qt.darker(appTheme.accent, 1.2) : (startSecHover.containsMouse ? Qt.lighter(appTheme.accent, 1.2) : appTheme.accent)
                    
                    Text {
                        anchors.centerIn: parent
                        text: "Начать"
                        color: "white"
                        font.pixelSize: 15
                        font.bold: true
                        font.family: "Segoe UI" 
                    }
                    
                    MouseArea {
                        id: startSecHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        
                        onClicked: {
                            SecretChatManager.createSecretRequest(parseInt(activeChatId))
                            confirmSecretChatPopup.close()

                            var secId = "pendind_" + activeChatId
                            chatAreaRoot.activeChatType = "secret"
                            chatAreaRoot.activeChatId = secId
                        }
                    }
                }
            }
        }
    }
}
