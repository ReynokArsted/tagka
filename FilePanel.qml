import QtQuick
import QtQuick.Controls
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Effects

import untitled 1.0   
import untitled.files 1.0
import "./" as Example

Rectangle
{
    id: root

///
    ///property var fileModel: null
    property var files: null        // search.files
    property var tagQuery: null     // search.tags
///      
    property var tagModel: null         

    property bool renameMode: false
    property string editingPath: ""
    property string editingName: ""

    property var selectedFiles: []

    signal folderOpened(string path)
    signal tagRequested(var paths, string name)
    signal renameRequested(string path, string name)
    signal renameCommitted(string newName)
    signal renameCancelled()
    signal deleteTagsRequested(var paths)
    signal deleteRequested(var paths)

///
    signal openFileRequested(string path)
    signal openWithRequested(string path)
///
    //FileListModel { id: files }
    System { id: system }

    Connections
{
    target: UsnJournalMonitor

    function onFilePathChanged(oldPath, newPath)
    {
        if (root.files) root.files.renamePath(oldPath, newPath)
    }

    function onFileDeleted(path, systemId)
    {
        if (root.files) root.files.removePath(path)
    }
}

    function clearSelection()
    {
        selectedFiles = []
    }

    function openFolder(p)
    {
        folderOpened(p)
    }

    function isSelected(p)
    {
        return selectedFiles.indexOf(p) !== -1
    }

    function toggleSelection(p)
    {
        const list = selectedFiles.slice()
        const i = list.indexOf(p)
        if (i !== -1) list.splice(i, 1)
        else list.push(p)
        selectedFiles = list
    }

    function selectOnly(p)
    {
        selectedFiles = [p]
    }

    // function openItem(p, dir)
    // {
    //     if (dir) openFolder(p)
    //     // else fileModel.openFile(p, root.Window.window)
    //     else system.openFile(p, root.Window.window)
    //     openFileRequested(p)
    // }

    function openItem(p, dir)
    {
        if (dir) openFolder(p + "/")
        else system.openFile(p, root.Window.window)
    }

    radius: 10
    color: Theme.fieldBackground
    border.color: Theme.borderColor
    border.width: 1

    layer.enabled: true
    layer.effect: MultiEffect
    {
        shadowEnabled: true
        shadowColor: "#40000000"
        shadowBlur: 0.6
        shadowHorizontalOffset: 0
        shadowVerticalOffset: 3
    }

    Menu
    {
        id: single_selection_menu

        property string currentPath: ""
        property string currentName: ""
        property bool currentIsDir: false

        MenuItem
        {
            text: qsTr("Задать метки")
            onTriggered:
            {
                root.tagRequested(root.selectedFiles, single_selection_menu.currentName)
                root.clearSelection()
            }
        }
        MenuItem
        {
            text: qsTr("Открыть")
            onTriggered: 
            {
                root.openItem
                (
                    single_selection_menu.currentPath, 
                    single_selection_menu.currentIsDir
                )
            }
        }
        MenuItem
        {
            text: qsTr("Открыть с помощью ...")
            // onTriggered:
            // {
            //     if (single_selection_menu.currentIsDir)
            //         root.openFolder(single_selection_menu.currentPath)
            //     else
            //         // root.fileModel.openWith
            //         system.openWith
            //         (
            //             single_selection_menu.currentPath, 
            //             root.Window.window
            //         )
            //         openWithRequested(single_selection_menu.currentPath)
            // }
            onTriggered:
            {
                if (single_selection_menu.currentIsDir)
                    root.openFolder(single_selection_menu.currentPath + "/")
                else
                    system.openWith(single_selection_menu.currentPath, root.Window.window)
            }
        }
        MenuItem
        {
            text: qsTr("Переименовать")
            onTriggered: 
            {
                root.renameRequested
                (
                    single_selection_menu.currentPath,
                    single_selection_menu.currentName
                )
            }
        }
        MenuItem
        {
            text: qsTr("Удалить все метки")
            onTriggered: root.deleteTagsRequested([single_selection_menu.currentPath])
        }
        MenuItem
        {
            text: qsTr("Удалить")
            onTriggered: root.deleteRequested([single_selection_menu.currentPath])
        }
    }
    Menu
    {
        id: multi_selection_menu

        property string currentName: ""

        MenuItem
        {
            text: qsTr("Задать метки")
            onTriggered: root.tagRequested
            (
                root.selectedFiles, 
                multi_selection_menu.currentName
            )
        }
        MenuItem
        {
            text: qsTr("Удалить все метки")
            onTriggered: root.deleteTagsRequested(root.selectedFiles)
        }
        MenuItem
        {
            text: qsTr("Удалить")
            onTriggered: root.deleteRequested(root.selectedFiles)
        }
    }

    ColumnLayout
    {
        anchors.fill: parent
        anchors.margins: 8

        ListView
        {
            id: fileListView

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 6
            //model: root.fileModel ? root.fileModel.get_files : null
            model: root.files

            delegate: Rectangle
            {
                id: card

                property bool pressed: false
                property var tagColors: []
                //property string path: modelData.path
                //property string name: modelData.name
                //property bool isDir: modelData.isDir
                readonly property bool selected: root.selectedFiles.indexOf(path) !== -1
///
                required property int index
                required property string path
                required property string name
                required property bool isDir
///

                width: ListView.view.width
                height: 36
                radius: 8
                border.color: selected ? Theme.accentColor : Theme.borderColor
                border.width: selected ? 2 : 1

                scale: pressed ? 0.98 : 1.0

                Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }
                Behavior on color { ColorAnimation { duration: 100 } }

                function refreshTagColors()
                {
                    //if (!root.fileModel || !root.tagModel)
                    if (!root.tagQuery || !root.tagModel)
                        return
                    //const ids = root.fileModel.tagIdsForFile([path])
                    const ids = root.tagQuery.tagIdsForFile([path])
                    const colors = []
                    for (var i = 0; i < ids.length; i++)
                        colors.push(root.tagModel.colorForId(ids[i]))
                    tagColors = colors
                }

                Component.onCompleted: refreshTagColors()

                Connections
                {
                    target: root.tagModel
                    function onTagsAssigned(paths)
                    {
                        if (paths.indexOf(card.path) !== -1)
                            card.refreshTagColors()
                    }
                }

                Connections
                {
                    target: UsnJournalMonitor
                    function onFilePathChanged(oldPath, newPath)
                    {
                        if (newPath === card.path) card.refreshTagColors()
                    }
                }

                Row
                {
                    anchors.left: parent.left
                    anchors.right: tagDots.left
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 12
                    spacing: 8

                    TextInput
                    {
                        id: renameInput

                        visible: root.renameMode && root.editingPath === card.path
                        text: root.editingName
                        color: Theme.textColor
                        font.pixelSize: 16
                        selectByMouse: true
                        activeFocusOnPress: true
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width

                        onVisibleChanged: if (visible)
                        {
                            text = root.editingName
                            forceActiveFocus()
                            selectAll()
                        }

                        onAccepted: root.renameCommitted(text)
                        onEditingFinished: if (visible) root.renameCommitted(text)
                        Keys.onEscapePressed: root.renameCancelled()
                    }

                    Text
                    {
                        visible: !renameInput.visible
                        text: card.isDir ? (card.name + "/") : card.name
                        color: Theme.textColor
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width
                    }
                }

                Row
                {
                    id: tagDots
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 12
                    spacing: 4

                    Repeater
                    {
                        model: card.tagColors
                        delegate: Rectangle
                        {
                            width: 8
                            height: 8
                            radius: 4
                            color: modelData
                            border.width: 1
                            border.color: Qt.darker(modelData, 1.3)
                        }
                    }
                }

                MouseArea
                {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton

                    onPressed: function(mouse)
                    {
                        if (mouse.button === Qt.RightButton)
                        {
                            card.pressed = false
                            if (root.selectedFiles.length > 1)
                            {
                                multi_selection_menu.currentName = card.name
                                multi_selection_menu.popup(card, mouse.x, mouse.y + 6)
                            }
                            else
                            {
                                single_selection_menu.currentPath = card.path
                                single_selection_menu.currentName = card.name
                                single_selection_menu.currentIsDir = card.isDir
                                single_selection_menu.popup(card, mouse.x, mouse.y + 6)
                            }
                        }
                        else if (mouse.button === Qt.LeftButton)
                            card.pressed = true
                    }

                    onReleased: function(mouse)
                    {
                        if (mouse.button === Qt.LeftButton) card.pressed = false
                    }

                    onClicked: function(mouse)
                    {
                        if (mouse.button !== Qt.LeftButton)
                            return

                        if (mouse.modifiers & Qt.ControlModifier)
                            root.toggleSelection(card.path)
                        else
                        {
                            fileListView.currentIndex = index
                            root.selectOnly(card.path)
                        }
                    }

                    onDoubleClicked: function(mouse)
                    {
                        if (!(mouse.modifiers & Qt.ControlModifier))
                            root.openItem(card.path, card.isDir)
                    }

                    onCanceled: card.pressed = false
                }
            }

            Rectangle
            {
                anchors.centerIn: parent
                visible: fileListView.count === 0
                width: 220
                height: 60
                radius: 8
                color: Theme.fieldBackground
                border.color: Theme.borderColor
                border.width: 1

                Text
                {
                    anchors.centerIn: parent
                    text: qsTr("Нет результатов поиска")
                    color: Theme.textColor
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    width: parent.width - 20
                }
            }
        }
    }
}
