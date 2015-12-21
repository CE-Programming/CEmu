import QtQuick 2.0
import QtQuick.Layouts 1.0

Rectangle {
    id: rectangle1
    width: 265
    height: 340

    color: "#444"

    ColumnLayout {
        id: columnLayout1
        anchors.bottom: rectangle3.bottom
        anchors.bottomMargin: 0
        z: 4
        anchors.left: parent.left
        anchors.leftMargin: 5
        anchors.top: rectangle3.top
        anchors.topMargin: 0

        NBigButton {
            id: nButton1
            anchors.horizontalCenter: parent.horizontalCenter
            text: "esc"
            keymap_id: 73
        }

        NBigButton {
            id: nButton2
            anchors.horizontalCenter: parent.horizontalCenter
            text: "pad"
            keymap_id: 65
        }

        NBigButton {
            id: nButton3
            anchors.horizontalCenter: parent.horizontalCenter
            text: "tab"
            keymap_id: 75
        }
    }

    ColumnLayout {
        id: columnLayout2
        x: -7
        anchors.bottom: rectangle3.bottom
        anchors.bottomMargin: 0
        z: 3
        anchors.top: rectangle3.top
        anchors.topMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 5
        NBigButton {
            id: nButton4
            anchors.horizontalCenter: parent.horizontalCenter
            text: "⌂on"
            keymap_id: 9
        }

        NBigButton {
            id: nButton5
            anchors.horizontalCenter: parent.horizontalCenter
            text: "doc"
            keymap_id: 69
        }

        NBigButton {
            id: nButton6
            width: 30
            anchors.horizontalCenter: parent.horizontalCenter
            text: "menu"
            keymap_id: 71
        }
    }

    Touchpad {
        id: touchpad1
        width: gridLayout1.width
        anchors.bottom: rectangle3.bottom
        anchors.bottomMargin: 6
        z: 2
        anchors.horizontalCenter: gridLayout1.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 6
    }

    Grid {
        id: gridLayout1
        anchors.right: nDualButton7.left
        anchors.rightMargin: 18
        anchors.left: nDualButton1.right
        anchors.leftMargin: 18
        anchors.bottom: nButton8.bottom
        anchors.bottomMargin: 0
        anchors.top: rectangle3.bottom
        anchors.topMargin: 6
        columns: 3
        rowSpacing: 6
        columnSpacing: 14

        NBigButton {
            id: nButton11
            text: "shift"
            keymap_id: 85
        }

        Rectangle {
            id: rectangle2
            width: 30
            height: 20
            color: "#00000000"
        }

        NBigButton {
            id: nButton12
            text: "var"
            keymap_id: 56
        }

        NNumButton {
            id: nNumButton1
            text: "7"
            keymap_id: 40
        }

        NNumButton {
            id: nNumButton2
            text: "8"
            keymap_id: 72
        }

        NNumButton {
            id: nNumButton3
            text: "9"
            keymap_id: 36
        }

        NNumButton {
            id: nNumButton4
            text: "4"
            keymap_id: 29
        }

        NNumButton {
            id: nNumButton5
            text: "5"
            keymap_id: 61
        }

        NNumButton {
            id: nNumButton6
            text: "6"
            keymap_id: 25
        }

        NNumButton {
            id: nNumButton7
            text: "1"
            keymap_id: 18
        }

        NNumButton {
            id: nNumButton8
            text: "2"
            keymap_id: 70
        }

        NNumButton {
            id: nNumButton9
            text: "3"
            keymap_id: 14
        }

        NNumButton {
            id: nNumButton10
            text: "0"
            keymap_id: 7
        }

        NNumButton {
            id: nNumButton11
            text: "."
            keymap_id: 59
        }

        NNumButton {
            id: nNumButton12
            text: "(-)"
            keymap_id: 3
        }
    }

    NBigButton {
        id: nButton7
        y: 96
        text: "ctrl"
        keymap_id: 86
        anchors.left: parent.left
        anchors.leftMargin: 5
        active_color: "#cce"
        back_color: "#bbf"
        active: false
        font_color: "#222"
    }

    NBigButton {
        id: nButton10
        x: 230
        y: 96
        text: "del"
        keymap_id: 64
        anchors.right: parent.right
        anchors.rightMargin: 5
    }

    GridLayout {
        id: gridLayout2
        anchors.top: gridLayout1.bottom
        anchors.topMargin: 10
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 7
        anchors.left: nDualButton4.left
        anchors.leftMargin: 0
        anchors.right: nButton8.right
        anchors.rightMargin: 0
        rowSpacing: 10
        columnSpacing: 10
        columns: 9

        NAlphaButton { text: "EE" ;keymap_id: 30 }
        NAlphaButton { text: "A" ;keymap_id: 50 }
        NAlphaButton { text: "B" ;keymap_id: 49 }
        NAlphaButton { text: "C" ;keymap_id: 48 }
        NAlphaButton { text: "D" ;keymap_id: 46 }
        NAlphaButton { text: "E" ;keymap_id: 45 }
        NAlphaButton { text: "F" ;keymap_id: 44 }
        NAlphaButton { text: "G" ;keymap_id: 39 }
        NAlphaButton { text: "?!" ;keymap_id: 8 }
        NAlphaButton { text: "π" ;keymap_id: 19 }
        NAlphaButton { text: "H" ;keymap_id: 38 }
        NAlphaButton { text: "I" ;keymap_id: 37 }
        NAlphaButton { text: "J" ;keymap_id: 35 }
        NAlphaButton { text: "K" ;keymap_id: 34 }
        NAlphaButton { text: "L" ;keymap_id: 33 }
        NAlphaButton { text: "M" ;keymap_id: 28 }
        NAlphaButton { text: "N" ;keymap_id: 27 }
        NAlphaButton { text: "⚑" ;keymap_id: 66 }
        NAlphaButton { text: "," ;keymap_id: 87 }
        NAlphaButton { text: "O" ;keymap_id: 26 }
        NAlphaButton { text: "P" ;keymap_id: 24 }
        NAlphaButton { text: "Q" ;keymap_id: 23 }
        NAlphaButton { text: "R" ;keymap_id: 22 }
        NAlphaButton { text: "S" ;keymap_id: 17 }
        NAlphaButton { id: nAlphaButtonT; text: "T" ;keymap_id: 16 }
        NAlphaButton { id: nAlphaButtonU; text: "U" ;keymap_id: 15 }
        NAlphaButton { text: "⏎" ;keymap_id: 0 }
        Rectangle {
            color: "#00000000"
            width: 15
            height: 15
        }
        NAlphaButton { text: "V" ;keymap_id: 13 }
        NAlphaButton { text: "W" ;keymap_id: 12 }
        NAlphaButton { text: "X" ;keymap_id: 11 }
        NAlphaButton { text: "Y" ;keymap_id: 6 }
        NAlphaButton { text: "Z" ;keymap_id: 5 }

        NAlphaButton {
            text: "space"
            keymap_id: 4
            anchors.right: nAlphaButtonU.right
            anchors.rightMargin: 0
            anchors.left: nAlphaButtonT.left
            anchors.leftMargin: 0
        }
    }

    NDualButton {
        id: nDualButton1
        x: 5
        y: 122
        id1: 51
        id2: 20
        anchors.left: parent.left
        anchors.leftMargin: 5
        text2: "trig"
        text1: "="
    }

    NDualButton {
        id: nDualButton2
        x: 5
        y: 148
        id1: 53
        id2: 31
        anchors.left: parent.left
        anchors.leftMargin: 5
        text2: "x²"
        text1: "^"
    }

    NDualButton {
        id: nDualButton3
        x: 5
        y: 174
        id1: 42
        id2: 21
        anchors.left: parent.left
        anchors.leftMargin: 5
        text2: "10^x"
        text1: "e^x"
    }

    NDualButton {
        id: nDualButton4
        y: 200
        id1: 60
        id2: 58
        anchors.left: parent.left
        anchors.leftMargin: 5
        text2: ")"
        text1: "("
    }

    NDualButton {
        id: nDualButton5
        x: 210
        y: 122
        id1: 63
        id2: 62
        anchors.right: parent.right
        anchors.rightMargin: 5
        text2: "?"
        text1: "?"
    }

    NDualButton {
        id: nDualButton6
        x: 210
        y: 148
        id1: 52
        id2: 41
        anchors.right: parent.right
        anchors.rightMargin: 5
        text2: "÷"
        text1: "×"
    }

    NDualButton {
        id: nDualButton7
        x: 210
        y: 174
        id1: 68
        id2: 57
        anchors.right: parent.right
        anchors.rightMargin: 5
        text2: "-"
        text1: "+"
    }

    NButton {
        id: nButton8
        x: 210
        y: 200
        anchors.right: parent.right
        anchors.rightMargin: 5
        width: 50
        height: 20
        text: "enter"
    }

    Rectangle {
        id: rectangle3
        height: 90
        color: "#111111"
        anchors.top: parent.top
        anchors.topMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 0
        z: 1
    }
}

