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

    property var search    
    property var thingModel
    property var dragOverlay
    property bool addTagHighlight: false
    property Window tpparentWin: null

    property alias tagInput: search_line.tagInput
    property alias pathText: search_line.text

    property bool tagSelectMode: false
    property var selectedTagIds: []
    property var tagTargetPaths: []

    signal settingsClicked()
    signal helpClicked()
    signal taggingCancelled()
    signal thingAdded(string path)
    signal folderChanged(string path)

    signal taggingConfirmed()

    function startTagging(paths, name) 
    {
        tagTargetPaths = paths
        selectedTagIds = search.tags.tagIdsForFile(paths)
        tagSelectMode = true
    }

    function toggleTagSelection(tagId) 
    {
        const idx = selectedTagIds.indexOf(tagId)
        const arr = selectedTagIds.slice()
        if (idx >= 0) arr.splice(idx, 1)
        else arr.push(tagId)
        selectedTagIds = arr
    }

    function removeTagFromSelection(tagId) 
    {
        const idx = selectedTagIds.indexOf(tagId)
        if (idx >= 0) 
        {
            const arr = selectedTagIds.slice()
            arr.splice(idx, 1)
            selectedTagIds = arr
        }
    }

    function confirmTagging() 
    {
        if (tagTargetPaths.length != 0) 
        {
            const ok = thingModel.assignTagsToFile(tagTargetPaths, selectedTagIds)
            if (!ok) console.warn("ERROR: file tags are not saved for: ", tagTargetPaths)
            else tag_panel.taggingConfirmed()
        }
        cancelTagging()
    }

    function cancelTagging() 
    {
        tagSelectMode = false
        tagTargetPaths = []
        selectedTagIds = []
        tag_panel.taggingCancelled()
    }

    function handleTagSelected(id, name)
    {
        if (tagSelectMode)
        {
            toggleTagSelection(id)
            return
        }

        if (tagInput.text.indexOf(name) === -1)
        {
            const sep = tagInput.text.length > 0 && !tagInput.text.endsWith(" ") ? " " : ""
            tagInput.text += sep + name
            search.setQuery(tagInput.text)
        }
    }

    function handleTagFilesRequested(id, name)
    {
        tagInput.text = name
        //fileModel.setFolder(name)
        search.setQuery(name)
    }

    function handleTagDeleteRequested(id, name)
    {
        const ok = thingModel.removeThing(id)
        if (!ok) console.warn("ERROR: removing tag:", name)
        else removeTagFromSelection(id)
    }

    Label 
    {
        color: Theme.textColor
        font.pixelSize: 14
        Layout.alignment: Qt.AlignHCenter
    }

    SearchLine 
    {
        id: search_line

        Layout.fillWidth: true

        //fileModel: tag_panel.fileModel
        parentWin: tag_panel

        search: tag_panel.search

        // onFolderChanged: fileModel.setFolder(path)
        onFolderChanged: function(path) 
        {
            //fileModel.setFolder(path)
            search.setQuery(path)
        }
    }

    Label 
    {
        text: qsTr("Текущий ввод: %1").arg(tagInput.text)
        font.pixelSize: 12
        color: "gray"
        Layout.alignment: Qt.AlignHCenter
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
        dragOverlay: tag_panel.dragOverlay

        onTagSelected: (id, name) => tag_panel.handleTagSelected(id, name)
        onTagFilesRequested: (id, name) => tag_panel.handleTagFilesRequested(id, name)
        onTagDeleteRequested: (id, name) => tag_panel.handleTagDeleteRequested(id, name)
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
