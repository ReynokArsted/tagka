import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

import untitled 1.0    
import "./" as Example

Dialog
{
    id: root

    property string message: qsTr("Ошибка поиска файла")

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
            text: root.message
            font.pixelSize: 18
            color: Theme.textColor
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Example.ThemedButton
        {
            text: qsTr("Закрыть")
            onClicked: root.close()
        }
    }
}
