import QtQuick

Rectangle {
    id: root
    
    required property Item dragParent
    property int visualIndex: 0
    property bool borderHighlight: false
    property color normalBorderColor: Theme.offBorder
    property color highlightBorderColor: Theme.onBorder

    signal pressed
    signal released
    signal clicked
    signal rightClicked

    border.width: 4
    border.color: Qt.alpha(root.color, borderHighlight ? 0.9 : 0.4)

    anchors {
        horizontalCenter: parent.horizontalCenter
        verticalCenter: parent.verticalCenter
    }
    radius: 8

    MouseArea {
        id: dragArea

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        drag.target: root
        onClicked: root.clicked()
        onPressed: root.pressed()
        onReleased: {
            parent.Drag.drop()
            root.released()
        }
    }

    MouseArea {
        id: contextArea

        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: root.rightClicked()
    }

    Drag.active: dragArea.drag.active
    Drag.source: root
    Drag.hotSpot.x: root.width / 2
    Drag.hotSpot.y: root.height / 2

    states: [
        State {
            when: dragArea.drag.active
            ParentChange {
                target: root
                parent: root.dragParent
            }

            AnchorChanges {
                target: root
                anchors.horizontalCenter: undefined
                anchors.verticalCenter: undefined
            }

            PropertyChanges {
                target: root
                z: 9999
            }
        }
    ]
}