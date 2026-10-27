import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

import untitled 1.0
import "./" as Example

Dialog
{
    id: root

    property string message: ""
    property string confirmText: qsTr("Да")
    property var action: null

    function ask(text, callback)
    {
        message = text
        action = callback
        open()
    }

    focus: true
    modal: true
    closePolicy: Popup.CloseOnEscape

    anchors.centerIn: Overlay.overlay
    width: Overlay.overlay ? Math.min(Overlay.overlay.width - 8, 420) : 0

    onClosed: action = null

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
            text: root.message
            font.pixelSize: 16
            color: Theme.textColor
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        RowLayout
        {
            spacing: 8
            Layout.alignment: Qt.AlignRight

            Example.ThemedButton
            {
                text: root.confirmText
                onClicked:
                {
                    const callback = root.action
                    root.close()
                    if (callback) callback()
                }
            }
            Example.ThemedButton
            {
                text: qsTr("Отмена")
                onClicked: root.close()
            }
        }
    }
}
