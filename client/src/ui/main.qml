import QtQuick

Window {
    id: window
    width: 1280
    height: 720
    minimumWidth: 600
    minimumHeight: 600
    visible: true
    title: "Messenger Alyosha"

    Loader {
        id: pageLoader
        anchors.fill: parent
        source: "sign_up.qml"
    }

    function push(page) {
        pageLoader.source = page
    }
}
