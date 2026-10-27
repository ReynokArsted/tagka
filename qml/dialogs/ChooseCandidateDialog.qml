import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

import untitled 1.0
import "./" as Example

Dialog
{
    id: root

    focus: true
    closePolicy: Popup.NoAutoClose
    modal: true

    anchors.centerIn: Overlay.overlay
    width: Overlay.overlay ? Overlay.overlay.width - 8 : 0
    height: Overlay.overlay ? Overlay.overlay.height - 8 : 0

    Connections
    {
        target: MoveChoice
        function onCurrentChanged()
        {
            if (MoveChoice.active && !root.visible)
                root.open()
            else if (!MoveChoice.active && root.visible)
                root.close()
        }
    }
    Component.onCompleted: if (MoveChoice.active) root.open()

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
            text: qsTr("Файл «%1» был удален. Есть похожие файлы на других носителях. Какой из них тот же файл?")
                .arg(MoveChoice.oldPath)
            font.pixelSize: 18
            color: Theme.textColor
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Text
        {
            visible: MoveChoice.waiting > 0
            text: qsTr("Ещё вопросов в очереди: %1").arg(MoveChoice.waiting)
            font.pixelSize: 13
            color: Theme.textColor
            Layout.fillWidth: true
        }

        ListView
        {
            id: list

            model: MoveChoice.candidates
            spacing: 6
            clip: true
            Layout.fillWidth: true
            Layout.fillHeight: true

            delegate: Example.ThemedButton
            {
                width: list.width
                text: modelData
                onClicked: MoveChoice.choose(index)
            }
        }

        Example.ThemedButton
        {
            text: qsTr("Такого файла нет")
            onClicked: MoveChoice.choose(-1)
        }
    }
}
