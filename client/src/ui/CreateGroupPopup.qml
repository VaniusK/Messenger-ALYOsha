import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Messenger 1.0

Popup {
    id: root
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: 380
    height: currentStep === 1 ? 200 : 500
    modal: true
    dim: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.5) }
    background: Rectangle { color: appTheme.bgPanel; radius: 10 }
    
    padding: 0 

    property int currentStep: 1
    property string groupName: ""
    property var selectedUsers: [] 
    property var recentChatsData: [] 
    readonly property int maxMembers: 50
    property int searchResultsCount: 0 

    onOpened: updateList(null)

    onClosed: { 
        currentStep = 1; groupName = ""; selectedUsers = []
        groupSearchInput.text = ""; candidatesModel.clear()
    }

    ListModel { id: candidatesModel }

    Connections {
        target: ChatLayer
        function onUsersFound(users) {
            if (root.visible && currentStep === 2 && groupSearchInput.text.trim() !== "") root.updateList(users)
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
        } else if (newArr.length < maxMembers) {
            newArr.push(userObj)
        }

        selectedUsers = newArr
        
        if (groupSearchInput.text !== "") {
            groupSearchInput.text = ""
        } else {
            updateList(null)
        }
    }

    function updateList(apiResults) {
        candidatesModel.clear()
        root.searchResultsCount = 0
        for (var i = 0; i < selectedUsers.length; i++) {
            candidatesModel.append({ 
                "userId": selectedUsers[i].id, 
                "displayName": selectedUsers[i].name,
                "handleStr": selectedUsers[i].handle,
                "isSelected": true 
            })
        }

        var sourceList = apiResults ? apiResults : recentChatsData
        for (var j = 0; j < sourceList.length; j++) {
            var sId = sourceList[j].id || sourceList[j].target_user_id
            if (!sId || String(sId) === "undefined" || String(sId) === String(AppState.userId)) continue
            if (!apiResults && sourceList[j].type && sourceList[j].type !== "direct") continue
            
            var alreadySelected = false
            
            for (var k = 0; k < selectedUsers.length; k++) {
                if (String(selectedUsers[k].id) === String(sId)) { 
                    alreadySelected = true
                    break
                } 
            }

            if (!alreadySelected) {
                candidatesModel.append({
                    "userId": sId, 
                    "displayName": sourceList[j].display_name || sourceList[j].title || "", 
                    "handleStr": sourceList[j].handle || "", 
                    "isSelected": false 
                })
                root.searchResultsCount++
            }
        }
    }

    contentItem: Item {
        anchors.fill: parent

        // НАЗВАНИЕ
        Item {
            anchors.fill: parent; visible: currentStep === 1
            RowLayout {
                anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 25; spacing: 20
                
                Rectangle { 
                    width: 72; height: 72; radius: 36
                    color: "#5eb5f7"; Layout.alignment: Qt.AlignVCenter
                    Image {
                        anchors.centerIn: parent
                        width: 36; height: 36; sourceSize: Qt.size(36, 36)
                        source: "qrc:/messenger_client_uri/assets/icons/camera.svg" 
                    } 
                }
                
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    
                    Text { 
                        text: "Название группы"
                        color: groupNameField.activeFocus ? appTheme.accent : appTheme.textHint
                        font.pixelSize: 16
                        font.family: "Segoe UI"
                        font.bold: true
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }

                    TextInput {
                        id: groupNameField
                        Layout.fillWidth: true
                        color: appTheme.textMain
                        font.pixelSize: 18
                        font.family: "Segoe UI"
                        maximumLength: 29
                        text: root.groupName
                        clip: true
                        onTextChanged: root.groupName = text
                        Keys.onReturnPressed: { if (text.trim() !== "") root.currentStep = 2 }
                    }
                    
                    Rectangle { 
                        Layout.fillWidth: true
                        height: 2
                        color: groupNameField.activeFocus ? appTheme.accent : appTheme.textHint 
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }
                }
            }

            // КНОПКИ
            RowLayout {
                anchors.bottom: parent.bottom; anchors.right: parent.right; anchors.margins: 15; spacing: 15
                
                Rectangle {
                    width: 80; height: 36; radius: 6
                    color: cancel1Hover.pressed ? Qt.alpha(appTheme.textMain, 0.1) : (cancel1Hover.containsMouse ? Qt.alpha(appTheme.textMain, 0.05) : "transparent")
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
                        id: cancel1Hover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.close() 
                    }
                }
                
                Rectangle {
                    width: 80; height: 36; radius: 6
                    property bool isValid: groupNameField.text.trim() !== ""
                    color: (nextHover.pressed && isValid) ? Qt.alpha(appTheme.accent, 0.2) : ((nextHover.containsMouse && isValid) ? Qt.alpha(appTheme.accent, 0.1) : "transparent")
                    opacity: isValid ? 1.0 : 0.5
                    Behavior on color { ColorAnimation { duration: 150 } }
                    Behavior on opacity { NumberAnimation { duration: 150 } }
                    
                    Text {
                        anchors.centerIn: parent
                        text: "Далее"
                        color: "#5eb5f7"
                        font.pixelSize: 15
                        font.bold: true
                        font.family: "Segoe UI" 
                    }

                    MouseArea {
                        id: nextHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: parent.isValid ? Qt.PointingHandCursor : Qt.ArrowCursor
                        onClicked: { if (parent.isValid) root.currentStep = 2 }
                    }
                }
            }
        }

        // УЧАСТНИКИ
        ColumnLayout {
            anchors.fill: parent; spacing: 0; visible: currentStep === 2
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
                    text: root.selectedUsers.length + " / " + root.maxMembers
                    color: root.selectedUsers.length > root.maxMembers ? "#ff4d4d" : "#8a96a3"
                    font.pixelSize: 15
                    font.family: "Segoe UI"
                    Layout.leftMargin: 10 
                }
            }

            Rectangle {
                Layout.fillWidth: true; Layout.margins: 15; Layout.topMargin: 0
                height: 36
                color: appTheme.bgInput
                radius: 18

                TextInput {
                    id: groupSearchInput
                    anchors.fill: parent
                    anchors.leftMargin: 15; anchors.rightMargin: 15
                    verticalAlignment: TextInput.AlignVCenter
                    topPadding: 0; bottomPadding: 0
                    font.pixelSize: 14; font.family: "Segoe UI"
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
                        id: popupSearchTimer
                        interval: 400
                        repeat: false
                        onTriggered: ChatLayer.searchUsers(groupSearchInput.text) 
                    }
                    onTextChanged: { 
                        if (text.trim() === "") {
                            popupSearchTimer.stop()
                            root.updateList(null)
                        } else popupSearchTimer.restart() 
                    }
                }
            }

            ListView {
                id: candidatesList
                Layout.fillWidth: true; Layout.fillHeight: true
                clip: true
                model: candidatesModel
                ScrollBar.vertical: ScrollBar {} boundsBehavior: Flickable.StopAtBounds

                footer: Item {
                    width: candidatesList.width
                    height: candidatesList.count === 0 ? candidatesList.height : 60
                    visible: root.searchResultsCount === 0 && groupSearchInput.text.trim() !== ""
                    
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
                    color: popupUserHover.containsMouse ? appTheme.hoverColor : "transparent"
                    
                    RowLayout {
                        anchors.fill: parent; anchors.margins: 15
                        spacing: 15
                        
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
                                    border.color: "#1c242f"
                                    border.width: 2
                                    
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
                        id: popupUserHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.toggleUser({ 
                            "id": model.userId,
                            "name": model.displayName,
                            "handle": model.handleStr 
                        }) 
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true; height: 60; color: appTheme.bgPanel

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#151b23"
                    anchors.top: parent.top 
                }

                RowLayout {
                    anchors.fill: parent; anchors.rightMargin: 15
                    spacing: 15
                    Item { Layout.fillWidth: true }
                    
                    // КНОПКА НАЗАД
                    Rectangle {
                        width: 80; height: 36; radius: 6
                        color: cancel2Hover.pressed ? Qt.alpha(appTheme.textMain, 0.1) : (cancel2Hover.containsMouse ? Qt.alpha(appTheme.textMain, 0.05) : "transparent")
                        Behavior on color { ColorAnimation { duration: 150 } }
                        
                        Text {
                            anchors.centerIn: parent
                            text: "Назад"
                            color: "#5eb5f7"
                            font.pixelSize: 15
                            font.bold: true
                            font.family: "Segoe UI" 
                        }

                        MouseArea {
                            id: cancel2Hover
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.currentStep = 1 
                        }
                    }
                    
                    // КНОПКА СОЗДАТЬ
                    Rectangle {
                        width: 90; height: 36; radius: 6
                        color: createHover.pressed ? Qt.darker(appTheme.accent, 1.2) : (createHover.containsMouse ? Qt.lighter(appTheme.accent, 1.2) : appTheme.accent)
                        Behavior on color { ColorAnimation { duration: 150 } }
                        
                        Text {
                            anchors.centerIn: parent
                            text: "Создать"
                            color: "white"
                            font.pixelSize: 15
                            font.bold: true
                            font.family: "Segoe UI" 
                        }

                        MouseArea {
                            id: createHover
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor

                            onClicked: {
                                var ids = [AppState.userId]
                                for (var i = 0; i < root.selectedUsers.length; i++) {
                                    ids.push(root.selectedUsers[i].id)
                                }

                                ChatLayer.createGroupChat(root.groupName, "", ids)
                                root.close()
                            }
                        }
                    }
                }
            }
        }
    }
}
