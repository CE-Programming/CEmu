import QtQuick 2.0

Rectangle {
    id: rectangle3
    property string text1: "a"
    property string text2: "b"
    property int id1: 1
    property int id2: 2

    width: 50
    height: 20
    color: "#00000000"

    NButton {
        id: nbutton1
        keymap_id: id1
        width: 25
        height: 20
        text: text1
    }

    NButton {
        id: nbutton2
        keymap_id: id2
        x: 25
        y: 0
        width: 25
        height: 20
        text: text2
    }

    Rectangle {
        id: rectangle1
        width: 8
        height: 20
        color: "#222233"
        radius: 0
        z: 0
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        border.width: 1
        border.color: "#888"
    }

    Rectangle {
        id: rectangle2
        width: 8
        height: 18
        color: "#222233"
        radius: 0
        z: 1
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
    }
}

