import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import untitled 1.0

ColumnLayout 
{
    id: tag_panel

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
            search.doRequest(tagInput.text)
        }
    }

    function handleTagFilesRequested(id, name)
    {
        tagInput.text = name
        //fileModel.setFolder(name)
        search.doRequest(name)
    }

    function handleTagDeleteRequested(id, name)
    {
        const ok = thingModel.removeThing(id)
        if (!ok) console.warn("ERROR: removing tag:", name)
        else removeTagFromSelection(id)
    }

    SearchLine 
    {
        id: search_line

        Layout.fillWidth: true
        Layout.topMargin: 12

        parentWin: tag_panel
        search: tag_panel.search

        onFolderChanged: function(path) 
        {
            search.doRequest(path)
        }
    }

    TagFlow 
    {
        id: tag_flow

        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 0
        Layout.bottomMargin: 10
        highlightBorders: tag_panel.addTagHighlight
        isTagSelectionMode: tag_panel.tagSelectMode
        selectedTagIds: tag_panel.selectedTagIds
        dragOverlay: tag_panel.dragOverlay

        onTagSelected: (id, name) => tag_panel.handleTagSelected(id, name)
        onTagFilesRequested: (id, name) => tag_panel.handleTagFilesRequested(id, name)
        onTagDeleteRequested: (id, name) => tag_panel.handleTagDeleteRequested(id, name)
    }                        
}
