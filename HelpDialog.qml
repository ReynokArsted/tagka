import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

import untitled 1.0        
import "./" as Example

Dialog
{
    id: root

    property alias model: help_view.model

    signal topicClicked(int index, string topic)

    modal: false
    closePolicy: Popup.NoAutoClose

    anchors.centerIn: Overlay.overlay
    width: Overlay.overlay ? Overlay.overlay.width - 8 : 0
    height: Overlay.overlay ? Overlay.overlay.height - 8 : 0

    background: Rectangle
    {
        radius: 4
        color: Theme.fieldBackground
        border.width: 1
        border.color: Theme.borderColor
    }

    contentItem: ColumnLayout
    {
        spacing: 10

        Text
        {
            text: qsTr("Справочник")
            font.pixelSize: 18
            color: Theme.textColor
        }

        ListView
        {
            id: help_view

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 6

            model: ListModel
            {
                ListElement { topic: "Тема 1" }
                ListElement { topic: "Тема 2" }
                ListElement { topic: "Тема 3" }
                ListElement { topic: "Тема 4" }
                ListElement { topic: "Тема 5" }
            }

            delegate: Rectangle
            {
                id: topicCard

                required property int index
                required property string topic

                property bool pressed: false

                width: ListView.view.width
                height: 36
                radius: 8
                border.width: 2

                scale: pressed ? 0.98 : 1.0

                Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }
                Behavior on color { ColorAnimation { duration: 100 } }

                Row
                {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 12
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8

                    Text
                    {
                        text: topicCard.topic
                        color: Theme.textColor
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width
                    }
                }

                MouseArea
                {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton

                    onPressed: function(mouse)
                    {
                        topicCard.pressed = (mouse.button === Qt.LeftButton)
                    }
                    onReleased: function(mouse)
                    {
                        if (mouse.button === Qt.LeftButton) topicCard.pressed = false
                    }
                    onClicked: function(mouse)
                    {
                        if (mouse.button === Qt.LeftButton)
                            root.topicClicked(topicCard.index, topicCard.topic)
                    }
                    onCanceled: topicCard.pressed = false
                }
            }
        }

        Example.ThemedButton
        {
            text: qsTr("Закрыть")
            onClicked: root.close()
        }
    }
}
