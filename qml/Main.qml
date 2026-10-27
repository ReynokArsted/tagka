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
    minimumHeight: 190
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
        const path = editingPath
        const oldName = editingName
        const trimmed = newName.trim()

        renameMode = false
        editingPath = ""
        editingName = ""

        if (path === "" || trimmed === "" || trimmed === oldName)
            return

        const newPath = system.renameFile(path, trimmed)
        if (newPath === "")
        {
            showError(qsTr("Не удалось переименовать «%1» в «%2»").arg(oldName).arg(trimmed))
            return
        }

        search.files.renamePath(path, newPath)
        file_panel.selectOnly(newPath)
    }

    function showError(message)
    {
        file_find_error_dialog.message = message
        file_find_error_dialog.critical = false
        file_find_error_mode = true
    }

    function fileNameOf(path)
    {
        return path.substring(path.lastIndexOf("/") + 1)
    }

    function confirmDelete(text, action)
    {
        if (settings_dialog.showDeleteWarnings)
            confirm_dialog.ask(text, action)
        else
            action()
    }

    function requestDeleteTags(paths)
    {
        if (!paths || paths.length === 0)
            return

        const text = paths.length === 1
            ? qsTr("Удалить все метки у файла «%1»?").arg(fileNameOf(paths[0]))
            : qsTr("Удалить все метки у выбранных файлов (%1)?").arg(paths.length)

        confirmDelete(text, function()
        {
            if (!ThingieListModel.removeAllTags(paths))
                showError(qsTr("Не удалось удалить метки"))
            file_panel.clearSelection()
        })
    }

    function requestDelete(paths)
    {
        if (!paths || paths.length === 0)
            return

        const text = paths.length === 1
            ? qsTr("Переместить файл «%1» в корзину? Все его метки будут удалены").arg(fileNameOf(paths[0]))
            : qsTr("Переместить выбранные файлы (%1) в корзину? Все их метки будут удалены").arg(paths.length)

        confirmDelete(text, function()
        {
            const removed = system.moveToTrash(paths)

            if (removed.length > 0 && !ThingieListModel.removeAllTags(removed))
                showError(qsTr("Файлы удалены, но их метки удалить не удалось"))

            for (let i = 0; i < removed.length; i++)
                search.files.removePath(removed[i])
            file_panel.clearSelection()

            if (removed.length !== paths.length)
                showError(qsTr("Не удалось удалить %1 из %2 элементов")
                          .arg(paths.length - removed.length).arg(paths.length))
        })
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

                    System { id: system }
                    RequestProcessor
                    {
                        id: search
                        onErrorOccurred: (message, critical) =>
                        {
                            file_find_error_dialog.message = message
                            file_find_error_dialog.critical = critical
                            win.file_find_error_mode = true
                        }
                    }

                    Binding
                    {
                        target: search.paths
                        property: "showHidden"
                        value: settings_dialog.showHiddenFiles
                    }

                    Connections
                    {
                        target: search.paths
                        function onShowHiddenChanged()
                        {
                            const input = tag_panel.tagInput.text
                            if (input.indexOf("#") === -1)
                                search.doRequest(input)
                        }
                    }

                    TagPanel 
                    {
                        id: tag_panel

                        search: search;
                        thingModel: ThingieListModel
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

                        tagModel: ThingieListModel

                        renameMode: win.renameMode
                        editingPath: win.editingPath
                        editingName: win.editingName

                        files: search.files
                        tagQuery: search.tags
                        onOpenFileRequested: (path) => system.openFile(path, win)
                        onOpenWithRequested: (path) => system.openWith(path, win)
                        
                        onFolderOpened: (path) =>
                        {
                            tag_panel.tagInput.text = path
                            search.doRequest(path)
                        }

                        onTagRequested: (paths, name) => tag_panel.startTagging(paths, name)
                        onRenameRequested: (path, name) => win.startRename(path, name)
                        onRenameCommitted: (newName) => win.commitRename(newName)
                        onRenameCancelled: win.cancelRename()
                        onDeleteTagsRequested: (paths) => win.requestDeleteTags(paths)
                        onDeleteRequested: (paths) => win.requestDelete(paths)
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
                ChooseCandidateDialog { id: choose_candidate_dialog }
                ConfirmDialog { id: confirm_dialog }
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

                onSettingsClicked: win.settings_mode = true
                onHelpClicked: win.help_mode = true 
            }
        }
    }

    Item 
    {
        id: drag_overlay
        anchors.fill: parent
        z: 1000
    }

    readonly property int resizeMargin: 6
    MouseArea {
        height: win.resizeMargin
        anchors { left: parent.left; right: parent.right; top: parent.top }
        cursorShape: Qt.SizeVerCursor
        onPressed: win.startSystemResize(Qt.TopEdge)
    }
    MouseArea {
        height: win.resizeMargin
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        cursorShape: Qt.SizeVerCursor
        onPressed: win.startSystemResize(Qt.BottomEdge)
    }
    MouseArea {
        width: win.resizeMargin
        anchors { top: parent.top; bottom: parent.bottom; left: parent.left }
        cursorShape: Qt.SizeHorCursor
        onPressed: win.startSystemResize(Qt.LeftEdge)
    }
    MouseArea {
        width: win.resizeMargin
        anchors { top: parent.top; bottom: parent.bottom; right: parent.right }
        cursorShape: Qt.SizeHorCursor
        onPressed: win.startSystemResize(Qt.RightEdge)
    }

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