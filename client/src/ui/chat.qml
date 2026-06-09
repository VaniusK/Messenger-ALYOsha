import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Messenger 1.0

Rectangle {
    id: root
    color: appTheme.bgMain

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

            onChatSelected: function(chatId, chatName, chatType, chatDescription, chatStatus) {
                if (VoiceLayer.isRecording) {
                    chatArea.showCancelPrompt()
                    return
                }
                chatArea.activeChatType = chatType                
                chatArea.activeChatId = chatId
                chatArea.activeChatName = chatName
                chatArea.activeChatDescription = chatDescription || ""
                chatArea.activeChatStatus = chatStatus || "active"
            }

            onSettingsRequested: {
                settingsPopup.open()
            }
        }

        ChatArea {
            id: chatArea
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }

    SettingsPopup {
        id: settingsPopup
        
        onLogoutConfirmed: {
            console.log("[Chat] exit to LogIn window")
            ChatLayer.clearCache()
            AppState.clearState()

            var loader = root.parent
            if (loader) {
                loader.source = "sign_in.qml"
            }
        }
    }

    Component.onCompleted: {
        console.log("[Chat] Main chat window loaded.")
        if (AppState.userId > 0) {
            SecretChatManager.initSession();
        }
    }

    Connections {
        target: AppState
        function onUserIdChanged() {
            if (AppState.userId > 0) {
                SecretChatManager.initSession();
            }
        }
    }
}
