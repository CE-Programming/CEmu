import QtQuick 2.0
import QtQuick.Layouts 1.0

Rectangle {
    id: rectangle1
    width: 265
    height: 360
    
    color: "#fff"

    NBigButton {
        id: nBigButton1
        x: 187
        y: 52
        width: 29
        height: 20
        color: "#ffffff"
        text: "/\\"
        keymap_id: 59
        font_color: "#000"
    }

    NBigButton {
        id: nBigButton2
        x: 187
        y: 78
        width: 29
        height: 20
        color: "#ffffff"
        radius: 2
        text: "\\/"
        keymap_id: 56
        font_color: "#000"
    }

    NBigButton {
        id: nBigButton49
        x: 222
        y: 52
        width: 17
        height: 46
        color: "#ffffff"
        radius: 0
        text: ">"
        keymap_id: 58
        font_color: "#000"
    }

    NBigButton {
        id: nBigButton50
        x: 164
        y: 52
        width: 17
        height: 46
        color: "#ffffff"
        text: "<"
        keymap_id: 57
        font_color: "#000"
    }

    GridLayout {
        x: 31
        y: 120
        rowSpacing: 15
        columns: 5

        NBigButton {
            id: nBigButton9
            width: 36
            height: 20
            color: "#000000"
            text: "math"
            keymap_id: 22
        }

        NBigButton {
            id: nBigButton10
            width: 36
            height: 20
            color: "#000000"
            text: "apps"
            keymap_id: 30
        }

        NBigButton {
            id: nBigButton11
            width: 36
            height: 20
            color: "#000000"
            text: "prgm"
            keymap_id: 38
        }

        NBigButton {
            id: nBigButton12
            width: 36
            height: 20
            color: "#000000"
            text: "vars"
            keymap_id: 46
        }

        NBigButton {
            id: nBigButton13
            width: 36
            height: 20
            color: "#000000"
            text: "clear"
            keymap_id: 54
        }

        NBigButton {
            id: nBigButton14
            width: 36
            height: 20
            color: "#000000"
            text: "x-1"
            keymap_id: 21
        }

        NBigButton {
            id: nBigButton15
            width: 36
            height: 20
            color: "#000000"
            text: "sin"
            keymap_id: 29
        }

        NBigButton {
            id: nBigButton16
            width: 36
            height: 20
            color: "#000000"
            text: "cos"
            keymap_id: 37
        }

        NBigButton {
            id: nBigButton17
            width: 36
            height: 20
            color: "#000000"
            text: "tan"
            keymap_id: 45
        }

        NBigButton {
            id: nBigButton18
            width: 36
            height: 20
            color: "#000000"
            text: "^"
            keymap_id: 53
        }

        NBigButton {
            id: nBigButton19
            width: 36
            height: 20
            color: "#000000"
            text: "x²"
            keymap_id: 20
        }

        NBigButton {
            id: nBigButton20
            width: 36
            height: 20
            color: "#000000"
            text: ","
            keymap_id: 28
        }

        NBigButton {
            id: nBigButton21
            width: 36
            height: 20
            color: "#000000"
            text: "("
            keymap_id: 36
        }

        NBigButton {
            id: nBigButton22
            width: 36
            height: 20
            color: "#000000"
            text: ")"
            keymap_id: 44
        }

        NBigButton {
            id: nBigButton23
            width: 36
            height: 20
            color: "#ffffff"
            text: "÷"
            keymap_id: 52
            font_color: "#000"
        }

        NBigButton {
            id: nBigButton24
            width: 36
            height: 20
            color: "#000000"
            text: "log"
            keymap_id: 19
        }

        NBigButton {
            id: nBigButton28
            width: 36
            height: 20
            color: "#ffffff"
            text: "7"
            keymap_id: 27
            font_color: "#000"
            active_color: "#555"
        }

        NBigButton {
            id: nBigButton29
            width: 36
            height: 20
            color: "#ffffff"
            text: "8"
            keymap_id: 35
            font_color: "#000"
        }

        NBigButton {
            id: nBigButton30
            width: 36
            height: 20
            color: "#ffffff"
            text: "9"
            keymap_id: 43
            font_color: "#000"
        }

        NBigButton {
            id: nBigButton37
            width: 36
            height: 20
            color: "#ffffff"
            text: "x"
            keymap_id: 51
            font_color: "#000"
        }

        NBigButton {
            id: nBigButton25
            width: 36
            height: 20
            color: "#000000"
            text: "ln"
            keymap_id: 18
        }

        NBigButton {
            id: nBigButton31
            width: 36
            height: 20
            color: "#ffffff"
            text: "4"
            keymap_id: 26
            font_color: "#000"
        }

        NBigButton {
            id: nBigButton32
            width: 36
            height: 20
            color: "#ffffff"
            text: "5"
            keymap_id: 34
            font_color: "#000"
        }

        NBigButton {
            id: nBigButton35
            width: 36
            height: 20
            color: "#ffffff"
            text: "6"
            keymap_id: 42
            font_color: "#000"
        }

        NBigButton {
            id: nBigButton38
            width: 36
            height: 20
            color: "#ffffff"
            text: "-"
            keymap_id: 50
            font_color: "#000"
        }

        NBigButton {
            id: nBigButton26
            width: 36
            height: 20
            color: "#000000"
            text: "sto"
            keymap_id: 17
        }

        NBigButton {
            id: nBigButton33
            width: 36
            height: 20
            color: "#ffffff"
            text: "1"
            keymap_id: 25
            font_color: "#000"
        }

        NBigButton {
            id: nBigButton34
            width: 36
            height: 20
            color: "#ffffff"
            text: "2"
            keymap_id: 33
            font_color: "#000"
        }

        NBigButton {
            id: nBigButton36
            width: 36
            height: 20
            color: "#ffffff"
            text: "3"
            keymap_id: 41
            font_color: "#000"
        }

        NBigButton {
            id: nBigButton39
            width: 36
            height: 20
            color: "#ffffff"
            text: "+"
            keymap_id: 49
            font_color: "#000"
        }

        NBigButton {
            id: nBigButton27
            width: 36
            height: 20
            color: "#000000"
            text: "on"
            keymap_id: 16
        }

        NBigButton {
            id: nBigButton40
            width: 36
            height: 20
            color: "#ffffff"
            text: "0"
            keymap_id: 24
            font_color: "#000"
        }

        NBigButton {
            id: nBigButton41
            width: 36
            height: 20
            color: "#ffffff"
            text: "."
            keymap_id: 32
            font_color: "#000"
        }

        NBigButton {
            id: nBigButton42
            width: 36
            height: 20
            color: "#ffffff"
            text: "(-)"
            keymap_id: 40
            font_color: "#000"
        }

        NBigButton {
            id: nBigButton43
            width: 36
            height: 20
            color: "#ffffff"
            text: "enter"
            keymap_id: 48
            font_color: "#000"
        }
    }

    GridLayout {
        x: 31
        y: 47
        rowSpacing: 15
        columns: 3

        NBigButton {
            id: nBigButton4
            width: 36
            height: 20
            color: "#7b7bdb"
            text: "2nd"
            keymap_id: 13
        }

        NBigButton {
            id: nBigButton3
            width: 36
            height: 20
            color: "#000000"
            text: "mode"
            keymap_id: 14
        }

        NBigButton {
            id: nBigButton5
            width: 36
            height: 20
            color: "#000000"
            text: "del"
            keymap_id: 15
        }

        NBigButton {
            id: nBigButton6
            width: 36
            height: 20
            color: "#62dd6d"
            text: "alpha"
            keymap_id: 23
        }

        NBigButton {
            id: nBigButton7
            width: 36
            height: 20
            color: "#000000"
            text: "X,T,O,n"
            keymap_id: 31
        }

        NBigButton {
            id: nBigButton8
            width: 36
            height: 20
            color: "#000000"
            text: "stat"
            keymap_id: 39
        }
    }

    RowLayout {
        x: 31
        y: 8
        spacing: 5

        NBigButton {
            id: nBigButton44
            width: 36
            height: 20
            color: "#ffffff"
            text: "y="
            keymap_id: 12
            font_color: "#000"
        }

        NBigButton {
            id: nBigButton45
            width: 36
            height: 20
            color: "#ffffff"
            text: "window"
            keymap_id: 11
            font_color: "#000"
        }

        NBigButton {
            id: nBigButton46
            width: 36
            height: 20
            color: "#ffffff"
            text: "zoom"
            keymap_id: 10
            font_color: "#000"
        }

        NBigButton {
            id: nBigButton47
            width: 36
            height: 20
            color: "#ffffff"
            text: "trace"
            keymap_id: 9
            font_color: "#000"
        }

        NBigButton {
            id: nBigButton48
            width: 36
            height: 20
            color: "#ffffff"
            text: "graph"
            keymap_id: 8
            font_color: "#000"
        }
    }

    Text {
        id: text1
        x: 78
        y: 34
        color: "#7b7bdb"
        text: qsTr("quit")
        font.pixelSize: 12
    }

    Text {
        id: text2
        x: 124
        y: 34
        color: "#7b7bdb"
        text: qsTr("ins")
        font.pixelSize: 12
    }

    Text {
        id: text3
        x: 81
        y: 68
        color: "#7b7bdb"
        text: qsTr("link")
        font.pixelSize: 12
    }

    Text {
        id: text4
        x: 122
        y: 68
        color: "#7b7bdb"
        text: qsTr("list")
        font.pixelSize: 12
    }












}

