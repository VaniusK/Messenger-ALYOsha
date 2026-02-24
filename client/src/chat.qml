import QtQuick
import QtQuick.Layouts
import Messenger 1.0

Rectangle {
    id: root
    color: "#f1f2f5"

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Sidebar {
            id: sidebar
            Layout.preferredWidth: Math.max(250, root.width * 0.34)
            Layout.fillHeight: true

            onChatSelected: function(chatId, chatName) {
                chatArea.acriveChatId = chatId
                chatArea.acriveChatName = chatName
            }

            onLogoutRequested: {
                console.log("[Chat] exit to LogIn window")
                AppState.clearState()

                var loader = root.parent
                if (loader) {
                    loader.source = "sign_in.qml"
                }
            }
        }

        ChatArea {
            id: chatArea
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
