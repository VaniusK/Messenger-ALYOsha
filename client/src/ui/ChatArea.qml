import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls
import QtMultimedia
import QtQuick.Window
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
    property string activeFullscreenVideoUrl: ""

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
                    // messageList.positionViewAtEnd()

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
        color: "#1c242f"
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
                color: "white"
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
                color: closeHoverArea.containsMouse ? "#2b3644" : "transparent"
                
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
            color: "#1c242f"

            Text {
                id: placeholderText
                text: "Выберите чат, чтобы начать общение"
                color: "white"
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
            verticalLayoutDirection: ListView.BottomToTop

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

                property bool isImage: !isVoice && fileTypeStr.indexOf("image/") === 0
                property bool isVideo: !isVoice && fileTypeStr.indexOf("video/") === 0

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
                
                Rectangle {
                    id: messageBubble

                    width: (isVoice || isImage || isVideo || hasFileAttachment)
                        ? Math.min(280, parent.width * 0.75) 
                        : Math.min(Math.max(60, messageText.implicitWidth + timeText.implicitWidth + 30), parent.width * 0.75)
                    
                    height: {
                        if (isVoice) return 60
                        if (hasFileAttachment) return fileAttachCol.implicitHeight + 32;

                        let mediaHeight = 0;
                        if (isImage) {
                            mediaHeight = imageContainer.height + 24
                        } else if (isVideo) {
                            mediaHeight = videoContainer.height + 24
                        }

                        let textHeight = (model.text && model.text.trim() !== "") ? messageText.contentHeight + 20 : 0
                        return mediaHeight + textHeight + (mediaHeight === 0 && textHeight !== 0 ? 8 : 0)
                    }
                    
                    radius: 12
                    color: isMe ? "#2b5278" : "#18222d"
                    
                    anchors.right: isMe ? parent.right : undefined
                    anchors.left: isMe ? undefined : parent.left
                    anchors.rightMargin: isMe ? 15 : 0
                    anchors.leftMargin: isMe ? 0 : 15

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

                        Rectangle {
                            width: 16; height: 16; color: messageBubble.color; radius: 4
                            anchors.bottom: parent.bottom
                            anchors.right: isMe ? parent.right : undefined
                            anchors.left: isMe ? undefined : parent.left
                        }
                        Rectangle {
                            width: 16; height: 16; color: "#0e1621"; radius: 8
                            anchors.bottom: parent.bottom; anchors.bottomMargin: 6
                            anchors.right: isMe ? parent.right : undefined; anchors.left: isMe ? undefined : parent.left
                            anchors.rightMargin: isMe ? -8 : 0; anchors.leftMargin: isMe ? 0 : -8
                        }
                    }

                    Item {
                        id: imageContainer
                        visible: isImage
                        anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 4
                        
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
                                cache: false
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
                        anchors.margins: 8; anchors.leftMargin: 12
                        anchors.topMargin: (isImage || isVideo) ? 8 : 12

                        wrapMode: TextEdit.Wrap; textFormat: TextEdit.PlainText
                        color: "white"; font.pixelSize: 15; font.family: "Segoe UI"
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
                        anchors.margins: 12; spacing: 6

                        RowLayout {
                            width: parent.width; height: 48; spacing: 12

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
                                    color: "white"; font.pixelSize: 14; font.family: "Segoe UI"
                                    elide: Text.ElideMiddle; width: parent.width
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
                                    color: isMe ? "#78aee3" : "#728392"
                                    font.pixelSize: 12; font.family: "Segoe UI"
                                }
                            }

                            MouseArea {
                                anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                onClicked: if (fileUrl) MediaLayer.downloadFile(fileUrl, firstAttachment ? (firstAttachment.original_filename || firstAttachment.file_name || "file") : "file")
                            }
                        }

                        Text {
                            visible: model.text && model.text.trim() !== ""
                            text: model.text ? model.text.trim() : ""
                            color: "white"; font.pixelSize: 14; font.family: "Segoe UI"
                            wrapMode: Text.Wrap; width: parent.width
                        }
                    }

                    RowLayout {
                        visible: isVoice
                        anchors.fill: parent; anchors.margins: 4; anchors.leftMargin: 8; anchors.rightMargin: 48; spacing: 12

                        Rectangle {
                            width: 44; height: 44; radius: 22
                            color: playMouseArea.pressed 
                                   ? (isMe ? "#2b5278" : "#0e1621") 
                                   : (isMe ? "#4a90d9" : "#2b5278")
                            
                            Image {
                                anchors.centerIn: parent
                                width: 16; height: 16
                                source: isPlayingThis 
                                    ? "qrc:/messenger_client_uri/assets/icons/pause.svg"
                                    : "qrc:/messenger_client_uri/assets/icons/play.svg"
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
                                    color: isMe ? "#78aee3" : "#4a5a6a"
                                    
                                    Rectangle {
                                        width: parent.width * waveRow.progress
                                        height: parent.height
                                        radius: 1.5
                                        color: "white"
                                    }
                                }
                                
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    
                                    function seekToPosition(xPos) {
                                        if (globalPlayingMsgId === uniqueId && globalAudioPlayer.duration > 0) {
                                            var safeX = Math.max(0, Math.min(xPos, width));
                                            var newPos = (safeX / width) * globalAudioPlayer.duration;
                                            globalAudioPlayer.position = newPos;
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
                                        var timeStr = "00:00";

                                        if (globalPlayingMsgId === uniqueId && globalAudioPlayer.duration > 0) {
                                            timeStr = formatGlobalTime(globalAudioPlayer.position) + " / " + formatGlobalTime(globalAudioPlayer.duration);
                                        } else if (globalAudioPlayer.duration > 0 && globalAudioPlayer.source.toString() === fileUrl) {
                                            timeStr = formatGlobalTime(globalAudioPlayer.duration);
                                        }

                                        var sizeStr = "";
                                        if (firstAttachment && firstAttachment.file_size_bytes) {
                                            var b = firstAttachment.file_size_bytes;
                                            if (b < 1048576) sizeStr = ", " + (b / 1024).toFixed(1) + " КБ";
                                            else sizeStr = ", " + (b / 1048576).toFixed(1) + " МБ";
                                        }
                                        
                                        return timeStr + sizeStr;
                                    }
                                    color: isMe ? "#78aee3" : "#728392"
                                    font.pixelSize: 11
                                    font.family: "Segoe UI"
                                }
                                Item { Layout.fillWidth: true }
                            }
                            Item { Layout.fillHeight: true }
                        }
                    }

                    Text {
                        id: timeText
                        text: {
                            var t = model.created_at || model.timestamp || model.time || model.sent_at;
                            if (!t) return "00:00"; 
                            
                            var d;
                            if (typeof t === "number") {
                                if (t < 10000000000) t *= 1000; 
                                d = new Date(t);
                            } else {
                                if (typeof t === "string" && t.indexOf("Z") === -1 && t.indexOf("+") === -1) {
                                    t += "Z";
                                }
                                d = new Date(t);
                            }

                            if (isNaN(d.getTime())) return "00:00";

                            var h = d.getHours(); var m = d.getMinutes();
                            return (h < 10 ? "0" + h : h) + ":" + (m < 10 ? "0" + m : m);
                        }
                        color: isMe ? "#78aee3" : "#728392"
                        font.pixelSize: 11; font.family: "Segoe UI"
                        anchors.right: parent.right; anchors.bottom: parent.bottom
                        anchors.rightMargin: 12; anchors.bottomMargin: 4
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
                    width: VoiceLayer.isRecording ? 0 : 36
                    height: 36
                    color: "transparent"
                    visible: isChatActive && !VoiceLayer.isRecording

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

                    Rectangle {
                        anchors.fill: parent
                        color: "#1c242f"
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
                                color: "white"
                                font.pixelSize: 16
                                font.family: "Segoe UI"
                            }
                            
                            Item { Layout.fillWidth: true }
                            
                            Text {
                                text: "Отмена"
                                color: "#5eb5f7"
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
                                    ChatLayer.sendMessage(activeChatId, messageInput.text.trim());
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
            MediaLayer.uploadFile(activeChatId, filePath, asFile, caption, "text")
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
            color: closeImagePreviewArea.containsMouse ? "rgba(255,255,255,0.2)" : "rgba(0,0,0,0.5)"
            
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

        VideoOutput {
            id: fullVideoOut
            anchors.fill: parent
            fillMode: VideoOutput.PreserveAspectFit
        }

        Timer {
            id: controlsTimer
            interval: 3000
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
            
            Image {
                id: videoCloseIcon
                anchors.centerIn: parent
                source: "qrc:/messenger_client_uri/assets/icons/close.svg"
                width: 24; height: 24; sourceSize: Qt.size(24, 24)
                visible: status === Image.Ready
            }
            Text { 
                anchors.centerIn: parent; text: "✕"; color: "white"; font.pixelSize: 20
                visible: videoCloseIcon.status !== Image.Ready 
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
            
            opacity: (showControls || fullVideoPlayer.playbackState !== MediaPlayer.PlayingState) ? 1 : 0
            visible: opacity > 0
            Behavior on opacity { NumberAnimation { duration: 300 } }
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 12
                
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    RowLayout {
                        spacing: 12
                        Image {
                            source: "qrc:/messenger_client_uri/assets/icons/volume.svg"
                            width: 24; height: 24; sourceSize: Qt.size(24, 24)
                        }
                        Slider {
                            id: videoVolumeSlider
                            Layout.preferredWidth: 120
                            from: 0; to: 1.0; value: 0.8
                            background: Rectangle {
                                implicitWidth: 120; implicitHeight: 4; radius: 2; color: "#30ffffff"
                                Rectangle { width: videoVolumeSlider.visualPosition * parent.width; height: parent.height; color: "white"; radius: 2 }
                            }
                            handle: Rectangle {
                                x: videoVolumeSlider.leftPadding + videoVolumeSlider.visualPosition * (videoVolumeSlider.availableWidth - width)
                                y: videoVolumeSlider.topPadding + videoVolumeSlider.availableHeight / 2 - height / 2
                                implicitWidth: 14; implicitHeight: 14; radius: 7; color: "white"
                            }
                        }
                    }
                    
                    Item { Layout.fillWidth: true }

                    Rectangle {
                        Layout.preferredWidth: 54; Layout.preferredHeight: 54
                        radius: 27; color: "transparent"
                        Image {
                            anchors.centerIn: parent
                            source: fullVideoPlayer.playbackState === MediaPlayer.PlayingState 
                                ? "qrc:/messenger_client_uri/assets/icons/pause.svg" 
                                : "qrc:/messenger_client_uri/assets/icons/play.svg"
                            width: 40; height: 40; sourceSize: Qt.size(40, 40)
                        }
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: fullVideoPlayer.playbackState === MediaPlayer.PlayingState ? fullVideoPlayer.pause() : fullVideoPlayer.play()
                        }
                    }
                    
                    Item { Layout.fillWidth: true }

                    Item { Layout.preferredWidth: 44; Layout.preferredHeight: 44 }
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

                        onMoved: {
                            fullVideoPlayer.position = value
                        }

                        background: Rectangle {
                            implicitHeight: 4
                            radius: 2
                            color: "#30ffffff"
                            Rectangle {
                                width: videoSlider.visualPosition * parent.width
                                height: parent.height
                                color: "#5eb5f7"
                                radius: 2
                            }
                        }
                        handle: Rectangle {
                            x: videoSlider.leftPadding + videoSlider.visualPosition * (videoSlider.availableWidth - width)
                            y: videoSlider.topPadding + videoSlider.availableHeight / 2 - height / 2
                            implicitWidth: 16
                            implicitHeight: 16
                            radius: 8
                            color: "white"
                        }
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
}
