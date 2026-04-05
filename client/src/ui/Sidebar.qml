import QtQuick
import QtQuick.Layouts
import Messenger 1.0

Rectangle {
    id: sidebarRoot
    color: "#1c242f"
    border.color: "#0e1621"
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

    signal logoutRequested()
    signal chatSelected(string chatId, string chatName)

    Connections {
        target: ChatLayer

        function onChatsUpdated(chats) {
            if (!isSearching) {
                chats.sort(function(a, b) {
                    if (a.type === "saved") return -1;
                    if (b.type === "saved") return 1;

                    var strA = a.last_message ? a.last_message.sent_at || "" : "";
                    var timeA = strA ? new Date(strA.replace(" ", "T") + (strA.indexOf("Z") === -1 ? "Z" : "")).getTime() : 0;
                    if (isNaN(timeA)) timeA = 0;

                    var strB = b.last_message ? b.last_message.sent_at || "" : "";
                    var timeB = strB ? new Date(strB.replace(" ", "T") + (strB.indexOf("Z") === -1 ? "Z" : "")).getTime() : 0;
                    if (isNaN(timeB)) timeB = 0;
                    
                    return timeB - timeA;
                })
                chatDataList = chats
                chatList.model = chatDataList
            }
        }

        function onMessageSentSuccess() {
            ChatLayer.fetchChats()
        }

        function onIncomingWebSocketMessage(data) {
            if (data.event_type === "NEW_MESSAGE") {
                ChatLayer.fetchChats()
            }
        }

        function onUsersFound(users) {
            if (isSearching) {
                chatDataList = users
                chatList.model = chatDataList
            }
        }

        function onDirectChatOpened(chatId, chatTitle) {
            searchInput.text = ""
            isSearching = false
            sidebarRoot.chatSelected(chatId, chatTitle)
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
            ChatLayer.fetchChats()
        }
    }

    Rectangle {
        id: searchHeader
        width: parent.width
        height: 100
        color: "#242f3d"
        anchors.top: parent.top

        RowLayout {
            anchors.fill: parent
            anchors.margins: 15
            spacing: 12

            Rectangle {
                width: 90
                height: 36
                radius: 8
                color: logoutBtnMouseArea.containsMouse ? "#d9363e" : "#ff4d4f"
                Layout.alignment: Qt.AlignVCenter

                Text {
                    text: "Выйти"
                    color: "white"
                    font.pixelSize: 14
                    font.bold: true
                    font.family: "Segoe UI"
                    anchors.centerIn: parent
                }

                MouseArea {
                    id: logoutBtnMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onPressed: parent.color = "#a22026"
                    onReleased: parent.color = logoutBtnMouseArea.containsMouse ? "#d9363e" : "#ff4d4f"
                    onClicked: sidebarRoot.logoutRequested()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 36
                color: "#17212b"
                radius: 18
                Layout.alignment: Qt.AlignVCenter

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
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 14
                    font.family: "Segoe UI"
                    color: "white"
                    clip: true

                    Text {
                        text: "Поиск"
                        color: "#8a96a3"
                        font.family: "Segoe UI"
                        visible: !parent.text
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Keys.onEscapePressed: {
                        text = ""
                        focus = false
                    }

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
        color: "#8a96a3"
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
            width: chatList.width
            height: 70 

            property var itemData: modelData
            property bool isActive: !sidebarRoot.isSearching && String(itemData.chat_id) === sidebarRoot.activeChatId
            property bool isSelf: isSearching && String(itemData.id) === String(AppState.userId)
            color: isActive ? "#2b5278" : (chatMouseArea.containsMouse ? "#202b36" : "#1c242f")

            MouseArea {
                id: chatMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (isSearching) {
                        ChatLayer.openDirectChat(itemData.id, itemData.display_name ?? itemData.handle ?? "")
                    } else {
                        var displayName = (itemData.type === "saved") ? "Избранное" : (itemData.title ? itemData.title : "")
                        sidebarRoot.chatSelected(
                            String(itemData.chat_id),
                            displayName
                        )
                        ChatLayer.fetchChatHistory(String(itemData.chat_id))
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

                    Canvas {
                        id: bookmarkCanvas
                        visible: (!isSearching && itemData.type === "saved") || isSelf
                        width: 22
                        height: 28
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
                    width: chatItem.width - 95

                    Item {
                        width: parent.width
                        height: 20

                        Text {
                            text: isSelf
                            ? "Избранное"
                            : isSearching
                                ? (itemData.display_name ?? itemData.handle ?? "")
                                : (itemData.type === "saved" ? "Избранное" : (itemData.title ?? ""))
                            font.bold: true
                            color: "white"
                            font.family: "Segoe UI"
                            font.pixelSize: 15
                            elide: Text.ElideRight
                            anchors.left: parent.left
                            anchors.right: timeText.left
                            anchors.rightMargin: 10
                            anchors.top: parent.top
                            textFormat: Text.PlainText
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
                            color: "#8a96a3"
                            font.pixelSize: 12
                            font.family: "Segoe UI"
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.topMargin: 2
                        }
                    }

                    Text {
                        text: isSearching ? "" : (itemData.last_message ? itemData.last_message.text : (itemData.unread_count > 0 ? "Новое сообщение\n" : "Нет сообщений"))
                        visible: !isSearching
                        color: "#8a96a3"
                        font.pixelSize: 14
                        font.family: "Segoe UI"
                        elide: Text.ElideRight
                        width: parent.width

                        maximumLineCount: 1
                        textFormat: Text.PlainText
                    }
                }
            }
        }
    }
}
