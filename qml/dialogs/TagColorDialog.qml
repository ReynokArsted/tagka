import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import untitled 1.0
import "./" as Example

Dialog
{
    id: root

    property int tagId: -1
    property string tagName: ""
    property color chosenColor: "#808080"

    signal colorChosen(int tagId, color color)

    readonly property var swatches:
    [
        "#e53935", "#d81b60", "#8e24aa", "#5e35b1",
        "#3949ab", "#1e88e5", "#039be5", "#00acc1",
        "#00897b", "#43a047", "#7cb342", "#c0ca33",
        "#fdd835", "#ffb300", "#fb8c00", "#6d4c41"
    ]

    function openFor(id, name, current)
    {
        tagId = id
        tagName = name
        chosenColor = current
        open()
    }

    function textColorOn(c)
    {
        return (0.299 * c.r + 0.587 * c.g + 0.114 * c.b) > 0.6 ? "#142528" : "white"
    }

    function randomColor()
    {
        return Qt.hsla(Math.random(), 0.55 + Math.random() * 0.35, 0.4 + Math.random() * 0.2, 1.0)
    }

    focus: true
    modal: true
    closePolicy: Popup.CloseOnEscape

    anchors.centerIn: Overlay.overlay
    width: Overlay.overlay ? Math.min(Overlay.overlay.width - 8, 360) : 0

    background: Rectangle
    {
        radius: 4
        color: Theme.fieldBackground
        border.width: 1
        border.color: Theme.borderColor
    }

    contentItem: ColumnLayout
    {
        spacing: 12

        Text
        {
            Layout.fillWidth: true
            text: qsTr("Цвет метки")
            font.pixelSize: 18
            color: Theme.textColor
        }

        Rectangle
        {
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: preview_text.implicitWidth + 24
            implicitHeight: 40
            radius: 8
            color: root.chosenColor
            border.width: 4
            border.color: Qt.alpha(root.chosenColor, 0.4)

            Text
            {
                id: preview_text

                anchors.centerIn: parent
                text: root.tagName
                font.pixelSize: 18
                color: root.textColorOn(root.chosenColor)
            }
        }

        GridLayout
        {
            Layout.alignment: Qt.AlignHCenter
            columns: 8
            rowSpacing: 6
            columnSpacing: 6

            Repeater
            {
                model: root.swatches

                delegate: Rectangle
                {
                    required property string modelData

                    readonly property bool current: Qt.colorEqual(root.chosenColor, modelData)

                    implicitWidth: 30
                    implicitHeight: 30
                    radius: 6
                    color: modelData
                    border.width: current ? 3 : 1
                    border.color: current ? Theme.textColor : Qt.darker(modelData, 1.3)

                    MouseArea
                    {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.chosenColor = parent.modelData
                    }
                }
            }
        }

        RowLayout
        {
            Layout.fillWidth: true
            spacing: 8

            Rectangle
            {
                Layout.fillWidth: true
                implicitHeight: 32
                radius: 4
                color: Theme.backgroundColor
                border.width: 1
                border.color: hex_input.acceptableInput ? Theme.borderColor : "#e53935"

                TextInput
                {
                    id: hex_input

                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 15
                    color: Theme.textColor
                    selectByMouse: true
                    validator: RegularExpressionValidator { regularExpression: /#?[0-9a-fA-F]{0,6}/ }

                    text: root.chosenColor.toString()
                    onTextEdited:
                    {
                        const hex = text.startsWith("#") ? text : "#" + text
                        if (/^#[0-9a-fA-F]{6}$/.test(hex))
                            root.chosenColor = hex
                    }
                    onEditingFinished: text = Qt.binding(() => root.chosenColor.toString())
                }
            }

            Example.ThemedButton
            {
                text: qsTr("Случайный")
                onClicked: root.chosenColor = root.randomColor()
            }
        }

        Rectangle
        {
            Layout.fillWidth: true
            implicitHeight: 1
            color: Theme.borderColor
        }

        RowLayout
        {
            Layout.fillWidth: true
            spacing: 8

            Item { Layout.fillWidth: true }

            Example.ThemedButton
            {
                text: qsTr("Отмена")
                onClicked: root.close()
            }

            Example.ThemedButton
            {
                text: qsTr("Применить")
                onClicked:
                {
                    root.colorChosen(root.tagId, root.chosenColor)
                    root.close()
                }
            }
        }
    }
}
