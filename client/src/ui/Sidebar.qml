import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Messenger 1.0

Rectangle {
    id: sidebarRoot
    color: appTheme.bgPanel
    border.color: appTheme.bgMain
    border.width: 1

    property var chatDataList: []
    property bool isSearching: false
    property string pendingSearchQuery: ""
    property bool hasSearchFocus: searchInput.activeFocus
    property string activeChatId: ""

    function clearSearch() {
        searchInput.text = ""
        searchInput.focus = false
    }

    signal settingsRequested()
    signal chatSelected(string chatId, string chatName, string chatType, string chatDescription, string chatStatus)

    function parseDateToMSecs(t) {
        if (!t) return 0
        if (typeof t === "number") return t < 10000000000 ? t * 1000 : t
        var d = new Date(String(t).replace(" ", "T") + (String(t).indexOf("Z") === -1 ? "Z" : ""))
        return isNaN(d.getTime()) ? 0 : d.getTime()
    }

    function updateCombinedChats(commonChats) {
        if (isSearching) return
        var secretChatsStr = SecretChatManager.getSecretChatsPreviews()
        var secretChats = []
        try {
            if (secretChatsStr !== "") secretChats = JSON.parse(secretChatsStr);
        } catch (e) { console.error("[Sidebar] Ошибка парсинга секретных чатов:", e); }

        var filteredCommon = []
        for (var i = 0; i < commonChats.length; i++) {
            if (commonChats[i].type !== "secret") {
                filteredCommon.push(commonChats[i])
            }
        }

        var combinedChats = filteredCommon.concat(secretChats)

        combinedChats.sort(function(a, b) {
            if (a.type === "saved") return -1
            if (b.type === "saved") return 1
            var timeA = a.last_message ? parseDateToMSecs(a.last_message.sent_at) : 0
            var timeB = b.last_message ? parseDateToMSecs(b.last_message.sent_at) : 0
            return timeB - timeA
        })
        
        chatDataList = combinedChats
        chatList.model = chatDataList
    }

    Connections {
        target: ChatLayer

        function onChatsUpdated(chats) {
            updateCombinedChats(chats)
        }

        function onMessageSentSuccess() {
            ChatLayer.fetchChats()
        }

        function onIncomingWebSocketMessage(data) {
            ChatLayer.fetchChats()
        }

        function onUsersFound(users) {
            if (isSearching && !createGroupPopup.visible) {
                chatDataList = users
                chatList.model = chatDataList
            }
        }

        function onDirectChatOpened(chatId, chatTitle) {
            searchInput.text = ""
            isSearching = false
            sidebarRoot.chatSelected(chatId, chatTitle, "direct", "", "active")
        }

        function onGroupChatCreated(chat) {
            createGroupPopup.close()
            ChatLayer.fetchChats()

            var cid = String(chat.id || chat.chat_id)
            var cname = chat.name || chat.title || "Группа"
            var cdesc = chat.description || ""
            sidebarRoot.chatSelected(cid, cname, "group", cdesc)
            ChatLayer.fetchChatHistory(cid, 0)
        }
    }

    Connections {
        target: SecretChatManager
        
        function onSecretChatsUpdated() {
            var currentCommonChats = []
            for (var i = 0; i < chatDataList.length; i++) {
                if (chatDataList[i].type !== "secret") currentCommonChats.push(chatDataList[i])
            }
            updateCombinedChats(currentCommonChats)
        }
        
        function onSecretChatCreated(chatId, title) {
            sidebarRoot.chatSelected(chatId, title, "secret", "", "pending")
        }
    }

    Connections {
        target: AppState

        function onUserIdChanged() {
            if (AppState.userId > 0) {
                ChatLayer.fetchChats()
            }
        }
    }

    Component.onCompleted: {
        console.log("[Sidebar] Component.onCompleted. userId =", AppState.userId)
        if (AppState.userId > 0) {
            updateCombinedChats([])

            ChatLayer.fetchChats()
        }
    }

    Rectangle {
        id: searchHeader
        width: parent.width
        height: 110
        color: appTheme.bgPanel
        anchors.top: parent.top

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 15
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Text {
                    text: "Чаты"
                    color: appTheme.textMain
                    font.pixelSize: 18
                    font.bold: true
                    font.family: "Segoe UI"
                }

                Item { Layout.fillWidth: true }

                // "НОВАЯ ГРУППА"
                Rectangle {
                    Layout.preferredHeight: 32
                    Layout.preferredWidth: newGroupContent.width + 24
                    radius: 16
                    color: createGroupArea.containsMouse ? Qt.lighter(appTheme.accent, 1.1) : appTheme.accent

                    RowLayout {
                        id: newGroupContent
                        anchors.centerIn: parent
                        spacing: 6
                        Image { 
                            source: "qrc:/messenger_client_uri/assets/icons/create_group.svg" 
                            width: 16; height: 16; sourceSize: Qt.size(16, 16) 
                        }
                        Text { 
                            text: "Новая группа"
                            color: appTheme.textMain
                            font.pixelSize: 13
                            font.bold: true
                            font.family: "Segoe UI" 
                        }
                    }

                    MouseArea {
                        id: createGroupArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: createGroupPopup.open()
                    }
                }

                // КНОПКА НАСТРОЕК
                Rectangle {
                    Layout.preferredHeight: 32
                    Layout.preferredWidth: settingsContent.width + 24
                    radius: 16
                    color: settingsBtnArea.containsMouse ? Qt.lighter(appTheme.accent, 1.1) : appTheme.accent

                    RowLayout {
                        id: settingsContent
                        anchors.centerIn: parent
                        spacing: 6

                        Image { 
                            source: "qrc:/messenger_client_uri/assets/icons/gear.svg" 
                            width: 16; height: 16; sourceSize: Qt.size(16, 16) 
                        }

                        Text { 
                            text: "Настройки"
                            color: appTheme.textMain
                            font.pixelSize: 13
                            font.bold: true
                            font.family: "Segoe UI" 
                        }
                    }

                    MouseArea {
                        id: settingsBtnArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: sidebarRoot.settingsRequested()
                    }
                }
            }

            // Поиск
            Rectangle {
                Layout.fillWidth: true
                height: 36
                color: appTheme.bgInput
                radius: 18

                Timer {
                    id: searchDelayTimer
                    interval: 400
                    repeat: false
                    onTriggered: ChatLayer.searchUsers(sidebarRoot.pendingSearchQuery)
                }

                TextInput {
                    id: searchInput
                    anchors.fill: parent
                    anchors.leftMargin: 15
                    anchors.rightMargin: 15
                    verticalAlignment: TextInput.AlignVCenter
                    topPadding: 0
                    bottomPadding: 0
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

                    Keys.onEscapePressed: { text = ""; focus = false }

                    onTextChanged: {
                        if (text.trim() === "") {
                            searchDelayTimer.stop()
                            isSearching = false
                            ChatLayer.fetchChats()
                        } else {
                            sidebarRoot.pendingSearchQuery = text
                            isSearching = true
                            searchDelayTimer.restart()
                        }
                    }
                }
            }
        }
    }

    Text {
        text: "Нет результатов..."
        color: appTheme.textHint
        font.pixelSize: 15
        font.family: "Segoe UI"
        anchors.centerIn: chatList
        visible: isSearching && chatList.count === 0 && searchInput.text.trim() !== ""
        z: 1
    }

    ListView {
        id: chatList
        width: parent.width
        anchors.top: searchHeader.bottom
        anchors.bottom: parent.bottom
        clip: true
        model: []

        delegate: Rectangle {
            id: chatItem
            width: ListView.view ? ListView.view.width : 0
            height: 70 

            property var itemData: modelData ? modelData : {}
            property bool isActive: !sidebarRoot.isSearching && String(itemData.chat_id) === sidebarRoot.activeChatId
            property bool isSelf: isSearching && String(itemData.id) === String(AppState.userId)
            color: isActive ? appTheme.bgActiveItem : (chatMouseArea.containsMouse ? appTheme.hoverColor : "transparent")

            MouseArea {
                id: chatMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    var targetChatId = String(itemData.chat_id ?? itemData.id ?? "")
                    if (isSearching) {
                        ChatLayer.openDirectChat(itemData.id, itemData.display_name ?? itemData.handle ?? "")
                        ChatManager.markChatAsRead(String(itemData.id))
                    } else {
                        var displayName = (itemData.type === "saved") ? "Избранное" : (itemData.title ? itemData.title : "")
                        sidebarRoot.chatSelected(
                            String(itemData.chat_id ?? itemData.id ?? ""),
                            displayName,
                            itemData.type,
                            itemData.description || "",
                            itemData.status || "active"
                        )

                        if (itemData.type === "secret") {
                            SecretChatManager.markChatAsRead(targetChatId);
                        } else {
                            ChatLayer.markChatAsRead(targetChatId, itemData.last_message.id);
                        }
                    }
                }
            }

            Row {
                anchors.fill: parent
                anchors.leftMargin: 15
                anchors.rightMargin: 15
                anchors.topMargin: 10
                anchors.bottomMargin: 10
                spacing: 15

                Rectangle {
                    id: avatarCircle
                    width: 50
                    height: 50
                    radius: 25
                    color: "#4a90d9"
                    anchors.verticalCenter: parent.verticalCenter
                    clip: true

                    Image {
                        id: bookmarkIcon
                        visible: (!isSearching && itemData.type === "saved") || isSelf
                        source: "qrc:/messenger_client_uri/assets/icons/bookmark.svg"
                        width: 24; height: 24; sourceSize: Qt.size(24, 24)
                        anchors.centerIn: parent
                    }

                    Text {
                        visible: !isSelf && (isSearching || itemData.type !== "saved")
                        text: isSearching
                            ? (itemData.display_name ? itemData.display_name.charAt(0).toUpperCase() : "?")
                            : (itemData.title ? itemData.title.charAt(0).toUpperCase() : "?")
                        color: "white"
                        font.bold: true
                        font.family: "Segoe UI"
                        font.pixelSize: 22
                        anchors.centerIn: parent
                        textFormat: Text.PlainText
                    }

                    Text {
                        visible: !isSelf && (isSearching || itemData.type !== "saved")
                        text: isSearching
                            ? (itemData.display_name ? itemData.display_name.charAt(0).toUpperCase() : "?")
                            : (itemData.title ? itemData.title.charAt(0).toUpperCase() : "?")
                        color: "white"
                        font.bold: true
                        font.family: "Segoe UI"
                        font.pixelSize: 22
                        anchors.centerIn: parent
                        textFormat: Text.PlainText
                    }
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.max(0, chatItem.width - 95)

                    Item {
                        width: parent.width
                        height: 20

                        RowLayout {
                            anchors.left: parent.left
                            anchors.right: timeText.left
                            anchors.rightMargin: 10
                            anchors.top: parent.top
                            spacing: 5
                            
                            Text {
                                text: isSelf
                                ? "Избранное"
                                : isSearching
                                    ? (itemData.display_name ?? itemData.handle ?? "")
                                    : (itemData.type === "saved" ? "Избранное" : (itemData.title ?? ""))
                                font.bold: true
                                color: chatItem.isActive ? "white" : appTheme.textMain
                                font.family: "Segoe UI"
                                font.pixelSize: 15
                                elide: Text.ElideRight
                                textFormat: Text.PlainText
                                Layout.maximumWidth: parent.width - (lockIcon.visible ? lockIcon.width + parent.spacing : 0)
                            }

                            Image {
                                id: lockIcon
                                visible: itemData.type === "secret"
                                source: "qrc:/messenger_client_uri/assets/icons/lock.svg"
                                width: 14; height: 14; sourceSize: Qt.size(14, 14)
                                Layout.alignment: Qt.AlignVCenter
                            }

                            Item { 
                                Layout.fillWidth: true 
                            }
                        }

                        Text {
                            id: timeText
                            visible: !isSearching && !!itemData.last_message && !!itemData.last_message.sent_at
                            text: {
                                if (isSearching || !itemData.last_message || !itemData.last_message.sent_at) return "";
                                var t = itemData.last_message.sent_at;
                                if (typeof t === "string" && t.indexOf(" ") !== -1) {
                                    t = t.replace(" ", "T");
                                }
                                if (t.indexOf("Z") === -1) {
                                    t += "Z";
                                }
                                var d = new Date(t);
                                if (isNaN(d.getTime())) return "";
                                var hrs = d.getHours();
                                var mins = d.getMinutes();
                                return (hrs < 10 ? "0" : "") + hrs + ":" + (mins < 10 ? "0" : "") + mins;
                            }
                            color: chatItem.isActive ? "white" : appTheme.textHint
                            font.pixelSize: 12
                            font.family: "Segoe UI"
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.topMargin: 2
                        }
                        
                        Image {
                            property bool isMyLastMsg: !!itemData 
                                                    && !!itemData.last_message 
                                                    && String(itemData.last_message.sender_id) === String(AppState.userId)
                            visible: isMyLastMsg && !isSearching && itemData.type !== "saved"
                            width: 14; height: 14
                            sourceSize: Qt.size(14, 14)
                            anchors.right: timeText.left
                            anchors.rightMargin: 4
                            anchors.verticalCenter: timeText.verticalCenter
                            source: (itemData.last_message && itemData.last_message.is_read)
                                    ? "qrc:/messenger_client_uri/assets/icons/check_double.svg"
                                    : "qrc:/messenger_client_uri/assets/icons/check_single.svg"
                        }
                    }

                    Item { width: 1; height: 4 }
                    
                    Item {
                        width: parent.width
                        height: Math.max(msgContentLayout.implicitHeight, unreadBadge.visible ? unreadBadge.height : 0)

                        RowLayout {
                            id: msgContentLayout
                            anchors.left: parent.left
                            anchors.right: unreadBadge.visible ? unreadBadge.left : parent.right
                            anchors.rightMargin: unreadBadge.visible ? 8 : 0
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 4
                            visible: !isSearching

                            property string senderNameStr: {
                                if (isSearching || !itemData || !itemData.last_message || itemData.type !== "group") return "";
                                
                                var msg = itemData.last_message;
                                if (String(msg.sender_id) === String(AppState.userId)) return "";

                                var sInfo = msg.sender_info;
                                var sName = sInfo ? (sInfo.display_name || sInfo.handle || "User") : (msg.sender_name || "User");
                                
                                return sName + ":";
                            }

                            // ТИП ВЛОЖЕНИЯ
                            property string mediaPrefix: {
                                if (isSearching || !itemData || !itemData.last_message) return ""
                                var msg = itemData.last_message

                                var mType = msg.message_type || msg.type || "text"

                                if (mType === "voice") return "Голосовое сообщение"

                                if (msg.attachments && msg.attachments.length > 0) {
                                    if (mType === "text") return "Файл"

                                    if (mType === "media") {
                                        var fType = msg.attachments[0].file_type || ""
                                        if (fType.indexOf("video/") === 0) return "Видео"
                                        if (fType.indexOf("image/") === 0) return "Фотография"
                                        return "Медиа"
                                    }
                                    
                                    return "Файл"
                                }
                                
                                return ""
                            }

                            // ИМЯ ОТПРАВИТЕЛЯ
                            Text {
                                visible: parent.senderNameStr !== ""
                                text: parent.senderNameStr
                                color: chatItem.isActive ? "white" : appTheme.accent
                                font.pixelSize: 14
                                font.family: "Segoe UI"

                                Layout.maximumWidth: parent.width * 0.4 
                                elide: Text.ElideRight
                            }

                            // МЕДИА-ПРЕФИКС
                            Text {
                                visible: parent.mediaPrefix !== ""
                                text: parent.mediaPrefix
                                color: chatItem.isActive ? "white" : appTheme.accent
                                font.pixelSize: 14
                                font.family: "Segoe UI"
                            }

                            // ТЕКСТ СООБЩЕНИЯ
                            Text {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter

                                color: chatItem.isActive ? "white" : (itemData.status === "pending" ? appTheme.accent : appTheme.textHint)
                                font.pixelSize: 14
                                font.family: "Segoe UI"
                                font.italic: itemData.status === "pending"
                                
                                elide: Text.ElideRight
                                maximumLineCount: 1
                                textFormat: Text.PlainText
                                
                                text: {
                                    if (isSearching || !itemData) return "";
                                    if (!itemData.last_message) {
                                        if (itemData.unread_count && itemData.unread_count > 0) return "Новое сообщение"
                                        return itemData.status === "pending" ? "Ожидание подтверждения..." : "Нет сообщений"
                                    }

                                    return itemData.last_message.text ? itemData.last_message.text.trim() : ""
                                }
                            }
                        }
                        
                        Rectangle {
                            id: unreadBadge
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter

                            visible: !isSearching && itemData && itemData.unread_count !== undefined && itemData.unread_count > 0

                            color: chatItem.isActive ? "white" : appTheme.accent
                            
                            height: 18
                            width: Math.max(height, unreadText.implicitWidth + 10)
                            radius: height / 2

                            Text {
                                id: unreadText
                                text: itemData && itemData.unread_count ? String(itemData.unread_count) : ""
                                color: chatItem.isActive ? appTheme.accent : "white"
                                font.pixelSize: 11
                                font.bold: true
                                font.family: "Segoe UI"
                                anchors.centerIn: parent
                            }
                        }
                    }

                    Text {
                        visible: isSearching
                        text: ""
                    }
                }
            }
        }
    }

    CreateGroupPopup {
        id: createGroupPopup
        recentChatsData: sidebarRoot.chatDataList
    }
}
