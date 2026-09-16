import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import untitled 1.0

ColumnLayout 
{
    id: tag_panel

    spacing: 10
    Layout.preferredWidth: 320
    Layout.minimumWidth: 320
    Layout.maximumWidth: 320
    Layout.fillHeight: true
    clip: true
    
    property var fileModel
    property var thingModel
    property var hub
    property var dragOverlay
    property bool addTagHighlight: false
    property bool tagSelectMode: false
    property var selectedTagIds: []
    property Window tpparentWin: null

    property alias tagInput: search_line.tagInput
    property alias pathText: search_line.text

    signal settingsClicked()
    signal helpClicked()
    signal taggingConfirmed(string path)
    signal taggingCancelled()
    signal thingAdded(string path)
    signal folderChanged(string path)

    Label 
    {
        //text: qsTr("Вводи метки:")
        color: Theme.textColor
        font.pixelSize: 14
        anchors.horizontalCenter: parent.horizontalCenter
    }

    SearchLine 
    {
        id: search_line

        Layout.fillWidth: true

        fileModel: tag_panel.fileModel
        parentWin: tpparentWin

        onFolderChanged: fileModel.setFolder(path)
    }

    Label 
    {
        text: qsTr("Текущий ввод: %1").arg(tagInput.text)
        font.pixelSize: 12
        color: "gray"
        anchors.horizontalCenter: parent.horizontalCenter
    }

    ThingGrid 
    {
        id: grid

        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 0
        highlightBorders: tag_panel.addTagHighlight
        isTagSelectionMode: tag_panel.tagSelectMode
        selectedTagIds: tag_panel.selectedTagIds
        hub: tag_panel.hub
        dragOverlay: tag_panel.dragOverlay
    }
                        
    RowLayout 
    {
        Layout.fillWidth: true
        Layout.preferredHeight: 52
                            
        ThemedButton 
        {
            text: "⚙"
            onClicked: tag_panel.settingsClicked()
        }
        ThemedButton 
        {
            id: help_button

            text: "?"
            onClicked: tag_panel.helpClicked()

            ToolTip 
            {
                id: help_tooltip

                popupType: Popup.Window
                visible: help_button.hovered
                delay: 1000
                timeout: 5000
                text: qsTr("tip test")
                //text: qsTr("loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong tip test")

                x: (help_button.width - width) / 2
                y: -height - 8

                contentItem: Text 
                {
                    id: tooltip_text

                    text: help_tooltip.text
                    color: "#142528"

                    width: help_tooltip.width

                    wrapMode: Text.WordWrap

                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 12
                    rightPadding: 12
                    topPadding: 8
                    bottomPadding: 18
                }

                background: Item 
                {
                    implicitWidth: tooltip_text.implicitWidth
                    implicitHeight: tooltip_text.implicitHeight

                    Canvas 
                    {
                        id: canvas

                        anchors.fill: parent

                        onPaint: 
                        {
                            var ctx = getContext("2d")
                            ctx.reset()

                            var w = width
                            var h = height
                            var r = 14
                            var tailWidth = 20
                            var tailHeight = 10
                            var bodyBottom = h - tailHeight

                            ctx.beginPath()
                            ctx.moveTo(r, 0)
                            ctx.lineTo(w - r, 0)
                            ctx.quadraticCurveTo(w, 0, w, r)
                            ctx.lineTo(w, bodyBottom - r)
                            ctx.quadraticCurveTo(w, bodyBottom, w - r, bodyBottom)

                            ctx.lineTo((w + tailWidth) / 2, bodyBottom)
                            ctx.lineTo(w / 2, h)
                            ctx.lineTo((w - tailWidth) / 2, bodyBottom)

                            ctx.lineTo(r, bodyBottom)
                            ctx.quadraticCurveTo(0, bodyBottom, 0, bodyBottom - r)
                            ctx.lineTo(0, r)
                            ctx.quadraticCurveTo(0, 0, r, 0)
                            ctx.closePath()

                            ctx.fillStyle = "#FDFDFD"
                            ctx.fill()

                            ctx.strokeStyle = "#142528"
                            ctx.lineWidth = 2
                            ctx.lineJoin = "round"
                            ctx.stroke()
                        }
                    }
                }
            }
        }
    }
}
