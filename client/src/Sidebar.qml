import QtQuick
import QtQuick.Layouts
import Messenger 1.0

Rectangle {
    id: sidebarRoot
    color: "#ffffff"
    border.color: "#e0e0e0"
    border.width: 1

    property var chatDataList: []
    property bool isSearching: false
    signal logoutRequested()
    signal chatSelected(string chatId, string chatName)

    Connections {
        target: ChatLayer

        function onChatsUpdated(chats) {
            if (!isSearching) {
                chatDataList = chats
                chatList.model = chatDataList
            }
        }

        function onUsersFound(users) {
            if (isSearching) {
                chatDataList = users
                chatList.model = chatDataList
            }
        }
    }

    Component.onCompleted: {
        ChatLayer.fetchChats()
    }

    Rectangle {
        id:searchHeader
        width: parent.width
        height: 100
        color: "#f5f5f5"
        border.color: "#e0e0e0"
        border.width: 1
        anchors.top: parent.top 

        RowLayout { 
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            Rectangle {
                width: 40
                height: 40 
                radius: 20
                color: "#e0e0e0"
                Layout.alignment: Qt.AlignVCenter

                Text {
                    text: "⚙"
                    font.pixelSize: 20
                    anchors.centerIn: parent
                }

                MouseArea { 
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onPressed: parent.color = "#d0d0d0"
                    onReleased: parent.color = "#e0e0e0"
                    onClicked:sidebarRoot.logoutRequested()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 40
                color: "white"
                border.color: "#ccc"
                radius: 20
                Layout.alignment: Qt.AlignVCenter
                
                TextInput {
                    anchors.fill: parent
                    anchors.leftMargin: 15
                    anchors.rightMargin: 15
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 14
                    Text {
                        text: "Поиск..."
                        color: "#999"
                        visible: !parent.text && !parent.activeFocus
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    onTextChanged: {
                        if (text.trim() === "") {
                            isSearching = false
                            ChatLayer.fetchChats()
                        } else {
                            isSearching = true
                            ChatLayer.searchUsers(text)
                        }
                    }
                }
            }
        }
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
            height: 100
            color: index % 2 == 0 ? "#ffffff" : "#fafafa"
            border.color: "#f0f0f0"
            border.width: 1
            property var itemData: modelData

            MouseArea {
                id: hoverArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    console.log("[Sidebar] open chat/user " + itemData.id)
                    sidebarRoot.chatSelected(itemData.id, itemData.name ? itemData.name : "Unknown user")
                    ChatLayer.fetchChatHistory(itemData.id)
                }
            }

            Row { 
                anchors.fill: parent
                anchors.margins: 10
                spacing: 15

                Rectangle {
                    width: Math.min(chatItem.height * 0.6, 60)
                    height: width
                    radius: width / 2
                    color: "#007bff"
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        text: itemData.name ? itemData.name.charAt(0).toUpperCase() : "?"
                        color: "white"
                        font.bold: true
                        font.pixelSize: parent.height * 0.5
                        anchors.centerIn: parent
                    }
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        text: itemData.name ? itemData.name : "Unknow User"
                        font.bold: true
                        font.pixelSize: Math.max(12, Math.min(16, chatItem.height * 0.2))
                    }
                    Text { 
                        text: itemData.last_message ? itemData.last_message : (isSearching ? "Начать диалог!" : "")
                        color: "#777"
                        font.pixelSize: Math.max(10, Math.min(14, chatItem.height * 0.15))
                    }
                }
            }
        }
    }
}
