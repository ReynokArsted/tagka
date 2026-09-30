import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Effects

import untitled 1.0
import untitled.files 1.0
import "./" as Example

ApplicationWindow 
{
    id: win

    width: 800
    height: 450
    minimumWidth: 500
    minimumHeight: 350
    visible: true
    title: "tagka"
    color: "transparent"

    flags: Qt.Window
        | Qt.WindowTitleHint
        | Qt.WindowSystemMenuHint
        | Qt.WindowMinimizeButtonHint
        | Qt.FramelessWindowHint

    property string editingPath: ""
    property string editingName: ""
    property bool addTagHighlight: false

    property bool renameMode: false
    property bool settings_mode: false
    property bool help_mode: false
    property bool file_find_error_mode: false

    onSettings_modeChanged: 
    {
        if (settings_mode) 
        {
            settings_dialog.open()
        } 
        else if (settings_dialog.visible) 
        {
            settings_dialog.close()
        }
    }

    onHelp_modeChanged: 
    {
        if (help_mode) 
        {
            help_dialog.open()
        } 
        else if (help_dialog.visible) 
        {
            help_dialog.close()
        }
    }

    onFile_find_error_modeChanged: 
    {
        if (file_find_error_mode) 
        {
            file_find_error_dialog.open()
        } 
        else if (file_find_error_dialog.visible) 
        {
            file_find_error_dialog.close()
        }
    }
    
    function startRename(path, name) 
    {
        editingPath = path
        editingName = name
        renameMode = true
    }

    function commitRename(newName) 
    {
        const trimmed = newName.trim()
        if (trimmed !== "" && editingPath !== "") 
        {
            console.log("Rename: ", editingPath, " -> ", trimmed)
            // fileModel.rename(editingPath, trimmed)
        }
        renameMode = false
        editingPath = ""
        editingName = ""
    }

    function cancelRename() 
    {
        renameMode = false
        editingPath = ""
        editingName = ""
    }
    
    Item 
    {
        id: content

        property int r: 8
        property int frame: 4
        property string tagText: tag_panel.tagInput.text

        anchors.fill: parent
        clip: true
        layer.enabled: true
        layer.smooth: true

        Rectangle 
        {
            anchors.fill: parent
            radius: content.r
            color: Qt.alpha(Theme.borderColor, 0.4)
        }

        Rectangle 
        {
            anchors.fill: parent
            anchors.margins: content.frame
            radius: content.r - content.frame
            color: Theme.backgroundColor
        }

        Item 
        {
            anchors.fill: parent

            Example.TopBar
            {
                id: top_bar

                width: parent.width
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: content.frame
                radius: 4
                color: Theme.fieldBackground
                border.color: Theme.borderColor
                border.width: 1

                corner_radius: content.r - content.frame
                title_text: win.title
                move_target: win

                onMinimizeClicked: win.showMinimized()
                onCloseClicked: win.close() 
            }

            ColumnLayout 
            {
                anchors.margins: 10
                anchors.fill: parent
                anchors.topMargin: top_bar.height
                anchors.bottomMargin: bottom_bar.height

                RowLayout 
                {
                    spacing: 2
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    FileListModel { id: fileModel }

                    TagPanel 
                    {
                        id: tag_panel

                        fileModel: fileModel
                        thingModel: ThingModel.listOfThingies
                        dragOverlay: drag_overlay

                        addTagHighlight: win.addTagHighlight

                        onSettingsClicked: win.settings_mode = true
                        onHelpClicked: win.help_mode = true 
                        onTaggingConfirmed: file_panel.clearSelection() 
                    }

                    FilePanel
                    {
                        id: file_panel

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumWidth: 120
                        Layout.topMargin: 12
                        Layout.bottomMargin: 12
                        Layout.rightMargin: 6
                        Layout.leftMargin: 6

                        fileModel: fileModel
                        tagModel: ThingModel.listOfThingies

                        renameMode: win.renameMode
                        editingPath: win.editingPath
                        editingName: win.editingName

                        onFolderOpened: (path) => tag_panel.tagInput.text = path
                        onTagRequested: (paths, name) => tag_panel.startTagging(paths, name)
                        onRenameRequested: (path, name) => win.startRename(path, name)
                        onRenameCommitted: (newName) => win.commitRename(newName)
                        onRenameCancelled: win.cancelRename()
                        onDeleteTagsRequested: (paths) => console.log("Delete all tags for:", paths)
                        onDeleteRequested: (paths) => console.log("Delete files:", paths)
                    }
                }

                SettingsDialog 
                { 
                    id: settings_dialog; 
                    onClosed: win.settings_mode = false 
                }
                HelpDialog
                { 
                    id: help_dialog; 
                    onClosed: win.help_mode = false 
                }
                ErrorDialog
                { 
                    id: file_find_error_dialog; 
                    onClosed: win.file_find_error_mode = false 
                }
            }

            Example.BottomBar
            {
                id: bottom_bar

                width: parent.width
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: content.frame
 
                radius: 4
                color: Theme.fieldBackground
                border.color: Theme.borderColor
                border.width: 1

                corner_radius: content.r - content.frame
                move_target: win
            }
        }
    }

    Item {
        id: drag_overlay
        anchors.fill: parent
        z: 1000
    }

    readonly property int resizeMargin: 6
    MouseArea { // top bond
        height: win.resizeMargin
        anchors { left: parent.left; right: parent.right; top: parent.top }
        cursorShape: Qt.SizeVerCursor
        onPressed: win.startSystemResize(Qt.TopEdge)
    }
    MouseArea { // bot bond
        height: win.resizeMargin
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        cursorShape: Qt.SizeVerCursor
        onPressed: win.startSystemResize(Qt.BottomEdge)
    }
    MouseArea { // left bond
        width: win.resizeMargin
        anchors { top: parent.top; bottom: parent.bottom; left: parent.left }
        cursorShape: Qt.SizeHorCursor
        onPressed: win.startSystemResize(Qt.LeftEdge)
    }
    MouseArea { // rifht bond
        width: win.resizeMargin
        anchors { top: parent.top; bottom: parent.bottom; right: parent.right }
        cursorShape: Qt.SizeHorCursor
        onPressed: win.startSystemResize(Qt.RightEdge)
    }

    // corners
    MouseArea {
        width: win.resizeMargin * 2
        height: win.resizeMargin * 2
        anchors { top: parent.top; left: parent.left }
        cursorShape: Qt.SizeFDiagCursor
        onPressed: win.startSystemResize(Qt.TopEdge | Qt.LeftEdge)
    }
    MouseArea {
        width: win.resizeMargin * 2
        height: win.resizeMargin * 2
        anchors { top: parent.top; right: parent.right }
        cursorShape: Qt.SizeBDiagCursor
        onPressed: win.startSystemResize(Qt.TopEdge | Qt.RightEdge)
    }
    MouseArea {
        width: win.resizeMargin * 2
        height: win.resizeMargin * 2
        anchors { bottom: parent.bottom; left: parent.left }
        cursorShape: Qt.SizeBDiagCursor
        onPressed: win.startSystemResize(Qt.BottomEdge | Qt.LeftEdge)
    }
    MouseArea {
        width: win.resizeMargin * 2
        height: win.resizeMargin * 2
        anchors { bottom: parent.bottom; right: parent.right }
        cursorShape: Qt.SizeFDiagCursor
        onPressed: win.startSystemResize(Qt.BottomEdge | Qt.RightEdge)
    }
}