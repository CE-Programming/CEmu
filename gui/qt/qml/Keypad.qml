import QtQuick 2.0
import QtQuick.Layouts 1.0

Rectangle {
    id: wrapperRectangle
    width: 265
    height: 370
    border.width: 0

    Rectangle {

    id: rectangle1
    width: 265
    height: 370

    anchors {
        horizontalCenter: parent.horizontalCenter;
        verticalCenter: parent.verticalCenter;
    }

    scale: Math.min(parent.width / width, parent.height / height);

    color: "#fff"
    border.width: 0

    GridLayout {
        x: 32
        y: 132
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
            text: "x-¹"
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
            text: "×"
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
        x: 32
        y: 62
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
            text: "XTØn"
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
        x: 32
        y: 23
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
            text: "wind"
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
        x: 82
        y: 50
        color: "#7b7bdb"
        text: qsTr("quit")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text2
        x: 126
        y: 50
        color: "#7b7bdb"
        text: qsTr("ins")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text3
        x: 83
        y: 85
        color: "#7b7bdb"
        text: qsTr("link")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text4
        x: 126
        y: 85
        color: "#7b7bdb"
        text: qsTr("list")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    NBigButton {
        id: nBigButton1
        x: 189
        y: 62
        width: 20
        color: "#ffffff"
        text: "^"
        font_color: "#000"
        keymap_id: 59
    }

    NBigButton {
        id: nBigButton2
        x: 189
        y: 97
        width: 20
        color: "#ffffff"
        text: "ᴠ"
        font_color: "#000"
        keymap_id: 56
    }

    NBigButton {
        id: nBigButton49
        x: 163
        y: 80
        width: 20
        color: "#ffffff"
        text: "<"
        font_color: "#000"
        keymap_id: 57
    }

    NBigButton {
        id: nBigButton50
        x: 215
        y: 80
        width: 20
        height: 20
        color: "#ffffff"
        text: ">"
        font_color: "#000"
        keymap_id: 58
    }

    Text {
        id: text5
        x: 36
        y: 85
        color: "#7b7bdb"
        text: qsTr("A-lock")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text6
        x: 32
        y: 120
        color: "#7b7bdb"
        text: qsTr("test")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text7
        x: 72
        y: 120
        color: "#7b7bdb"
        text: qsTr("angle")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text8
        x: 114
        y: 120
        color: "#7b7bdb"
        text: qsTr("draw")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text9
        x: 155
        y: 120
        color: "#7b7bdb"
        text: qsTr("distr")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text10
        x: 32
        y: 156
        color: "#7b7bdb"
        text: qsTr("matrix")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text11
        x: 32
        y: 190
        color: "#7b7bdb"
        text: qsTr("√")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text12
        x: 72
        y: 156
        color: "#7b7bdb"
        text: qsTr("sin-¹")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text13
        x: 114
        y: 156
        color: "#7b7bdb"
        text: qsTr("cos-¹")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text14
        x: 155
        y: 156
        color: "#7b7bdb"
        text: qsTr("tan-¹")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text15
        x: 196
        y: 156
        color: "#7b7bdb"
        text: qsTr("π")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text16
        x: 72
        y: 190
        color: "#7b7bdb"
        text: qsTr("EE")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text17
        x: 114
        y: 190
        color: "#7b7bdb"
        text: qsTr("{")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text18
        x: 155
        y: 190
        color: "#7b7bdb"
        text: qsTr("}")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text19
        x: 196
        y: 190
        color: "#7b7bdb"
        text: qsTr("e")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text20
        x: 196
        y: 225
        color: "#7b7bdb"
        text: qsTr("[")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text21
        x: 196
        y: 260
        color: "#7b7bdb"
        text: qsTr("]")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text22
        x: 196
        y: 296
        color: "#7b7bdb"
        text: qsTr("mem")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text23
        x: 32
        y: 225
        color: "#7b7bdb"
        text: qsTr("10×")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text24
        x: 32
        y: 260
        width: 7
        height: 11
        color: "#7b7bdb"
        text: qsTr("e×")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text25
        x: 32
        y: 296
        color: "#7b7bdb"
        text: qsTr("rcl")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text26
        x: 32
        y: 330
        color: "#7b7bdb"
        text: qsTr("off")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text27
        x: 62
        y: 330
        color: "#7b7bdb"
        text: qsTr("catalog")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text28
        x: 72
        y: 225
        color: "#7b7bdb"
        text: qsTr("u")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text29
        x: 114
        y: 225
        color: "#7b7bdb"
        text: qsTr("v")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text30
        x: 155
        y: 225
        color: "#7b7bdb"
        text: qsTr("w")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text31
        x: 155
        y: 260
        color: "#7b7bdb"
        text: qsTr("L6")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text32
        x: 114
        y: 260
        color: "#7b7bdb"
        text: qsTr("L5")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text33
        x: 72
        y: 260
        color: "#7b7bdb"
        text: qsTr("L4")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text34
        x: 72
        y: 296
        color: "#7b7bdb"
        text: qsTr("L1")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text35
        x: 114
        y: 296
        color: "#7b7bdb"
        text: qsTr("L2")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text36
        x: 155
        y: 296
        color: "#7b7bdb"
        text: qsTr("L3")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text37
        x: 114
        y: 330
        color: "#7b7bdb"
        text: qsTr("i")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text38
        x: 155
        y: 330
        color: "#7b7bdb"
        text: qsTr("ans")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text39
        x: 196
        y: 330
        color: "#7b7bdb"
        text: qsTr("entry")
        textFormat: Text.PlainText
        font.pixelSize: 9
    }

    Text {
        id: text40
        x: 62
        y: 120
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("A")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text41
        x: 103
        y: 120
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("B")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text42
        x: 143
        y: 120
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("C")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text43
        x: 62
        y: 155
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("D")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text44
        x: 103
        y: 155
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("E")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text45
        x: 143
        y: 155
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("F")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text46
        x: 183
        y: 155
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("G")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text47
        x: 223
        y: 155
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("H")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text48
        x: 62
        y: 190
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("I")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text49
        x: 103
        y: 190
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("J")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text50
        x: 143
        y: 190
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("K")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text51
        x: 183
        y: 190
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("L")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text52
        x: 223
        y: 190
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("M")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text53
        x: 62
        y: 225
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("N")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text54
        x: 103
        y: 225
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("O")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text55
        x: 143
        y: 225
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("P")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text56
        x: 183
        y: 225
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("Q")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text57
        x: 223
        y: 225
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("R")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text58
        x: 62
        y: 260
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("S")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text59
        x: 103
        y: 260
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("T")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text60
        x: 143
        y: 260
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("U")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text61
        x: 183
        y: 260
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("V")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text62
        x: 223
        y: 260
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("W")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text63
        x: 62
        y: 295
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("X")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text64
        x: 103
        y: 295
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("Y")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text65
        x: 143
        y: 295
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("Z")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text66
        x: 183
        y: 295
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("Ø")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text67
        x: 223
        y: 295
        width: 13
        height: 12
        color: "#62dd6d"
        text: "\""
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text68
        x: 103
        y: 330
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("_")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text69
        x: 143
        y: 330
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr(":")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text70
        x: 183
        y: 330
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("?")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text71
        x: 219
        y: 330
        width: 24
        height: 12
        color: "#62dd6d"
        text: qsTr("solve")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text72
        x: 20
        y: 8
        color: "#7b7bdb"
        text: qsTr("stat plot")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text73
        x: 74
        y: 8
        color: "#7b7bdb"
        text: qsTr("tblset")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text74
        x: 114
        y: 8
        color: "#7b7bdb"
        text: qsTr("frmt")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text75
        x: 155
        y: 8
        color: "#7b7bdb"
        text: qsTr("calc")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text76
        x: 196
        y: 8
        color: "#7b7bdb"
        text: qsTr("table")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text77
        x: 59
        y: 8
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("f1")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text78
        x: 101
        y: 8
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("f2")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text79
        x: 141
        y: 8
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("f3")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text80
        x: 181
        y: 8
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("f4")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }

    Text {
        id: text81
        x: 222
        y: 8
        width: 13
        height: 12
        color: "#62dd6d"
        text: qsTr("f5")
        textFormat: Text.PlainText
        font.pixelSize: 10
    }










}

}

