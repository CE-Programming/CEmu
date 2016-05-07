import QtQuick 2.0
import CE.emu 1.0

Rectangle {
    property string active_color: "#555"
    property string back_color: "#223"
    property string font_color: "#fff"
    property alias text: label.text
    property bool active: false
    property int keymap_id: 1

    border.width: active ? 2 : 1
    border.color: active ? "#A22" : "#888"
    radius: 4
    color: active ? active_color : back_color

    Text {
        id: label
        anchors.fill: parent
        anchors.centerIn: parent
        font.pixelSize: height/2
        color: font_color
        font.bold: true
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }

    MouseArea {
        // Pressing the right mouse button "locks" the button in enabled state
        property bool fixable: false
        property bool state: false
        property bool hovering: false

        preventStealing: true

        Component.onCompleted: {
            Emu.registerNButton(keymap_id, this);
        }

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onEntered: {
            hovering = true
        }
        onExited: {
            hovering = false
        }

        onStateChanged: {
            parent.active = state || hovering;

            Emu.keypadStateChanged(parent.keymap_id, state);
        }

        onHoveringChanged: {
            parent.active = state || hovering;
        }

        onPressed: {
            mouse.accepted = true;

            if(mouse.button == Qt.LeftButton)
            {
                if(!fixable)
                    state = true;
            }
            else if(fixable == state) // Right button
            {
                fixable = !fixable;
                state = !state;
            }
        }

        onReleased: {
            mouse.accepted = true;

            if(mouse.button == Qt.LeftButton
                    && !fixable)
                state = false;
        }
    }
}
