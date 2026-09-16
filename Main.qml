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
    property bool renameMode: false
    property bool addTagHighlight: false

    property bool tagSelectMode: false
    property bool settings_mode: false
    property bool help_mode: false
    property list<string> tagTargetPaths: []
    property var selectedTagIds: []

    property var candidates: [] 
    property string currentDirPath: ""
    property string currentPrefix: ""

    function detectSep(input) 
    {
        return (input.indexOf("\\") !== -1) ? "\\" : "/";
    }

    function normalizeForLogic(s) 
    {
        return (s || "").replace(/\\/g, "/");
    }

    function splitDirAndPrefix(normInput) {
        if (normInput.length === 0)
            return { dirPath: "", prefix: "" }

        let lastSlash = normInput.lastIndexOf("/")
        if (lastSlash === -1)
            return { dirPath: "", prefix: normInput }

        return {
            dirPath: normInput.slice(0, lastSlash + 1),
            prefix: normInput.slice(lastSlash + 1)
        }
    }

    function commonPrefixFromCandidates(cands, prefix) 
    {
        if (!cands || cands.length === 0) return ""

        let cp = cands[0].name
        for (let i = 1; i < cands.length; i++) 
        {
            const s = cands[i].name
            while (cp.length > 0 && !s.startsWith(cp))
                cp = cp.slice(0, cp.length - 1)
            if (cp.length === 0) break
        }
        if (cp.length < prefix.length)
            return prefix

        return cp
    }

    function refreshCandidates() {
        const temp_input = input.text
        const sep = detectSep(temp_input)
        const norm = normalizeForLogic(temp_input)

        const sp = splitDirAndPrefix(norm)
        currentDirPath = sp.dirPath
        currentPrefix = sp.prefix

        if (currentDirPath === "") {
            candidates = []
            return
        }

        candidates = fileModel.getCandidates(currentDirPath, currentPrefix)
        console.log("current dir path: " + currentDirPath)
        console.log("current prefix: " + currentPrefix)

        if (candidates.length > 0) 
        {
            fileModel.showCandidates(candidates);
        }
    }

    function startTagging(paths, name) {
        tagTargetPaths = paths
        selectedTagIds = ThingModel.listOfThingies.tagIdsForFile(paths)
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
        if (idx >= 0) {
            const arr = selectedTagIds.slice()
            arr.splice(idx, 1)
            selectedTagIds = arr
        }
    }

    function confirmTagging() 
    {
        if (tagTargetPaths.length != 0) 
        {
            const ok = ThingModel.listOfThingies.assignTagsToFile(tagTargetPaths, selectedTagIds)
            if (!ok) console.warn("ERROR: file tags are not saved for: ", tagTargetPaths)
            else fileModel.setFolder(fileModel.currentFolder) 
        }
        cancelTagging()
    }

    function cancelTagging() 
    {
        tagSelectMode = false
        tagTargetPaths = []
        selectedTagIds = []
    }

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

    Example.SignalHub { id: hub }
    Connections 
    {
        target: hub
        function onSelected(id, name) 
        {
            if (win.tagSelectMode) 
            {
                console.log("selected tag id: ", id)
                win.toggleTagSelection(id)
            }
            else
                if (tag_panel.tagInput.text.indexOf(name) === -1) 
                {
                    const sep = tag_panel.tagInput.text.length > 0 && !tag_panel.tagInput.text.endsWith(" ") ? " " : ""
                    tag_panel.tagInput.text += sep + name
                    fileModel.setFolder(tag_panel.tagInput.text)
                }
        }

        function onTagDeleteRequested(id, name) 
        {
            console.log("Delete tag:", id, name)
            const ok = ThingModel.listOfThingies.removeThing(id)
            if (!ok) console.warn("ERROR: removing tag:", name)
            else win.removeTagFromSelection(id)
        }

        function onTagFilesRequested(id, name) 
        {
            console.log("Show files by tag:", id, name)
            const query = name
            input.text = query 
            fileModel.setFolder(query) 
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

                    TagPanel 
                    {
                        id: tag_panel

                        fileModel: fileModel
                        thingModel: ThingModel.listOfThingies
                        hub: hub
                        dragOverlay: dragOverlay
                        tpparentWin: win

                        addTagHighlight: win.addTagHighlight
                        tagSelectMode: win.tagSelectMode
                        selectedTagIds: win.selectedTagIds

                        onSettingsClicked: win.settings_mode = true
                        onHelpClicked: win.help_mode = true
                        onTaggingConfirmed: win.confirmTagging()
                        onTaggingCancelled: win.cancelTagging()
                    }

                    FileListModel { id: fileModel }

                    Rectangle 
                    {
                        id: fileListPanel

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumWidth: 120
                        Layout.topMargin: 12
                        Layout.bottomMargin: 12
                        Layout.rightMargin: 6
                        Layout.leftMargin: 6     

                        radius: 10
                        color: Theme.fieldBackground
                        border.color: Theme.borderColor
                        border.width: 1
                        //clip: true

                        layer.enabled: true
                        layer.effect: MultiEffect 
                        {
                            shadowEnabled: true
                            shadowColor: "#40000000"
                            shadowBlur: 0.6
                            shadowHorizontalOffset: 0
                            shadowVerticalOffset: 3
                        }

                        ColumnLayout 
                        {
                            anchors.fill: parent
                            anchors.margins: 8

                            ListView 
                            {
                                id: fileListView
                                
                                //property alias tagInput: input
                                property alias tagInput: tag_panel.tagInput
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                spacing: 6
                                model: fileModel
                                property var selected_files: []

                                delegate: Rectangle 
                                {
                                    id: card

                                    property bool pressed: false
                                    property var tagColors: []

                                    width: ListView.view.width
                                    height: 36
                                    radius: 8
                                    color: fileListView.selected_files.indexOf(path) !== -1 ? Theme.selectionColor : Theme.fieldBackground
                                    border.color: fileListView.selected_files.indexOf(path) !== -1 ? Theme.accentColor : Theme.borderColor
                                    border.width: fileListView.selected_files.indexOf(path) !== -1 ? 2 : 1

                                    scale: pressed ? 0.98 : 1.0

                                    Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }
                                    Behavior on color { ColorAnimation { duration: 100 } }

                                    function refreshTagColors() 
                                    {
                                        const ids = ThingModel.listOfThingies.tagIdsForFile(path)
                                        const colors = []
                                        for (var i = 0; i < ids.length; i++)
                                            colors.push(ThingModel.listOfThingies.colorForId(ids[i]))
                                        tagColors = colors
                                    }

                                    Component.onCompleted: refreshTagColors()

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

                                            visible: win.renameMode && win.editingPath === path
                                            text: win.editingName
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
                                                text = win.editingName
                                                forceActiveFocus()
                                                selectAll()
                                            }

                                            onAccepted: win.commitRename(text)
                                            onEditingFinished: if (visible) win.commitRename(text)
                                            Keys.onEscapePressed: win.cancelRename()
                                        }

                                        Text 
                                        {
                                            visible: !renameInput.visible
                                            text: isDir ? (name + "/") : name
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
                                                console.log("Tag added for: ", fileListView.selected_files)
                                                win.startTagging(fileListView.selected_files, single_selection_menu.currentName)
                                                fileListView.selected_files = []
                                            }
                                        }
                                        MenuItem 
                                        {
                                            text: qsTr("Открыть")
                                            onTriggered: 
                                            {
                                                if (fileListView.selected_files.length <= 1)
                                                {
                                                    if (single_selection_menu.currentIsDir) 
                                                    {
                                                        fileModel.setFolder(single_selection_menu.currentPath)
                                                        fileListView.tagInput.text = single_selection_menu.currentPath
                                                    } 
                                                    else fileModel.openFile(single_selection_menu.currentPath, win)
                                                }
                                            }
                                        }
                                        MenuItem 
                                        {
                                            text: qsTr("Открыть с помощью ...")
                                            onTriggered: 
                                            {
                                                if (fileListView.selected_files.length <= 1)
                                                {
                                                    if (single_selection_menu.currentIsDir) 
                                                    {
                                                        fileModel.setFolder(single_selection_menu.currentPath)
                                                        fileListView.tagInput.text = single_selection_menu.currentPath
                                                    } 
                                                    else fileModel.openWith(single_selection_menu.currentPath, win)
                                                }
                                            }
                                        }
                                        MenuItem 
                                        {
                                            text: qsTr("Переименовать")
                                            onTriggered: 
                                            {
                                                if (fileListView.selected_files.length <= 1)
                                                {
                                                    win.startRename(single_selection_menu.currentPath, single_selection_menu.currentName)
                                                }
                                            }
                                        }
                                        MenuItem 
                                        {
                                            text: qsTr("Удалить все метки")
                                            onTriggered: 
                                            {
                                                if (fileListView.selected_files.length <= 1)
                                                {
                                                    console.log("Delete all tags for: ", single_selection_menu.currentPath)
                                                }
                                            }
                                        }
                                        MenuItem 
                                        {
                                            text: qsTr("Удалить")
                                            onTriggered: 
                                            {
                                                if (fileListView.selected_files.length <= 1)
                                                {
                                                    console.log("Delete file with path: ", single_selection_menu.currentPath)
                                                }
                                            }
                                        }
                                    }

                                    Menu 
                                    {
                                        id: multi_selection_menu

                                        property string currentPath: ""
                                        property string currentName: ""
                                        property bool currentIsDir: false

                                        MenuItem 
                                        {
                                            text: qsTr("Задать метки")
                                            onTriggered: 
                                            {
                                                console.log("Tag added for: ", fileListView.selected_files)
                                                win.startTagging(fileListView.selected_files, multi_selection_menu.currentName)
                                                fileListView.selected_files = []
                                            }
                                        }
                                        MenuItem 
                                        {
                                            text: qsTr("Удалить все метки")
                                            onTriggered: 
                                            {
                                                if (fileListView.selected_files.length <= 1)
                                                    console.log("Delete all tags for: ", multi_selection_menu.currentPath)
                                            }
                                        }
                                        MenuItem 
                                        {
                                            text: qsTr("Удалить")
                                            onTriggered: 
                                            {
                                                if (fileListView.selected_files.length <= 1)
                                                    console.log("Delete file with path: ", multi_selection_menu.currentPath)
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
                                                if (fileListView.selected_files.length > 1) 
                                                {
                                                    multi_selection_menu.currentPath = path
                                                    multi_selection_menu.currentName = name
                                                    multi_selection_menu.currentIsDir = isDir
                                                    multi_selection_menu.popup(card, mouse.x, mouse.y + 6)
                                                } 
                                                else 
                                                {
                                                    single_selection_menu.currentPath = path
                                                    single_selection_menu.currentName = name
                                                    single_selection_menu.currentIsDir = isDir
                                                    single_selection_menu.popup(card, mouse.x, mouse.y + 6)
                                                }
                                            } 
                                            else if (mouse.button === Qt.LeftButton) card.pressed = true
                                        }

                                        onReleased: function(mouse)
                                        {
                                            if (mouse.button === Qt.LeftButton) card.pressed = false
                                        }

                                        onClicked: function(mouse)
                                        {
                                            if ((mouse.button == Qt.LeftButton) && (mouse.modifiers & Qt.ControlModifier))
                                            {
                                                var ind = fileListView.selected_files.indexOf(path)
                                                if (ind !== -1) 
                                                {
                                                    fileListView.selected_files.splice(ind, 1)
                                                    console.log("unselected file: " + path)
                                                } 
                                                else 
                                                {
                                                    fileListView.selected_files.push(path)
                                                    console.log("selected files: " + fileListView.selected_files)
                                                }
                                                fileListView.selected_files = fileListView.selected_files.slice()
                                            }
                                            else if (mouse.button === Qt.LeftButton) 
                                            {
                                                fileListView.currentIndex = index
                                            }
                                        }
                                        onDoubleClicked: function(mouse)
                                        {
                                            if (mouse.button === Qt.LeftButton) 
                                            { 
                                                if (isDir) {
                                                    const p = fileModel.current_folder()
                                                    console.log("current folder = " + p);
                                                    if (p.endsWith("/"))
                                                    {
                                                        fileListView.tagInput.text = p + name + "/"
                                                        fileModel.setFolder(p + name + "/")
                                                    }
                                                    else
                                                    {
                                                        fileListView.tagInput.text = p + "/" + name + "/"
                                                        fileModel.setFolder(p + "/" + name + "/")
                                                    }
                                                } 
                                                else fileModel.openFile(path)
                                            }
                                        }
                                        onCanceled: 
                                        {
                                            card.pressed = false
                                        }
                                    }
                                }
                            }

                            Rectangle 
                            {
                                anchors.centerIn: fileListView
                                //anchors.centerIn: fileListPanel
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

                Dialog 
                {
                    id: settings_dialog

                    visible: win.settings_mode
                    modal: false
                    closePolicy: !Popup.CloseOnPressOutside | !Popup.CloseOnEscape
                    anchors.centerIn: parent
                    height: win.height - 8
                    width: win.width - 8

                    background: Rectangle 
                    {
                        anchors.fill: parent
                        radius: 4
                        color: Theme.fieldBackground
                        border.width: 1
                        border.color: Theme.borderColor
                    }

                    onClosed: win.settings_mode = false

                    contentItem: ColumnLayout 
                    {
                        spacing: 10

                        Text 
                        {
                            text: qsTr("Настройки")
                            font.pixelSize: 18
                            color: Theme.textColor
                        }

                        Example.ThemedButton 
                        {
                            text: Theme.isDarkMode ? qsTr("Установить светлую тему") : qsTr("Установить тёмную тему")
                            onClicked: Theme.isDarkMode = !Theme.isDarkMode
                        }
                        Example.ThemedButton 
                        {
                            text: Translator.language === "en" ? "Change language to Russian" : "Изменить язык на английский"
                            onClicked: Translator.language = Translator.language === "en" ? "ru" : "en"
                        }
                        Flow 
                        {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 160
                            spacing: 6

                            Repeater 
                            {
                                model: ThingModel.listOfThingies

                                delegate: Rectangle 
                                {
                                    width: tagLabel.implicitWidth + 24
                                    height: 30
                                    radius: 15
                                    color: Theme.fieldBackground
                                    border.color: Theme.borderColor
                                    border.width: 1

                                    Behavior on color { ColorAnimation { duration: 100 } }

                                    Text 
                                    {
                                        id: tagLabel

                                        anchors.centerIn: parent
                                        text: model.name
                                        color: Theme.textColor
                                    }
                                }
                            }
                        }

                        Example.ThemedButton 
                        {
                            text: qsTr("Применить")
                        }
                        Example.ThemedButton 
                        {
                            text: qsTr("Закрыть")
                            onClicked: win.settings_mode = false
                        }
                    }
                }

                Dialog 
                {
                    id: help_dialog

                    visible: win.help_mode
                    modal: false      
                    closePolicy: !Popup.CloseOnPressOutside | !Popup.CloseOnEscape
                    anchors.centerIn: parent
                    width: win.width - 8
                    height: win.height - 8

                    background: Rectangle 
                    {
                        anchors.fill: parent
                        radius: 4
                        color: Theme.fieldBackground
                        border.width: 1
                        border.color: Theme.borderColor
                    }

                    onClosed: win.help_mode = false

                    contentItem: ColumnLayout 
                    {
                        spacing: 10

                        Text 
                        {
                            text: qsTr("Справочник")
                            font.pixelSize: 18
                            color: Theme.textColor
                        }

                        ListView 
                        {
                            id: help_view
                                
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                spacing: 6
                                model: ListModel 
                                {
                                    ListElement { topic: "Тема 1" }
                                    ListElement { topic: "Тема 2" }
                                    ListElement { topic: "Тема 3" }
                                    ListElement { topic: "Тема 4" }
                                    ListElement { topic: "Тема 5" }
                                }

                                delegate: Rectangle 
                                {
                                    id: topic

                                    property bool pressed: false
                                    width: ListView.view.width
                                    height: 36
                                    radius: 8
                                    border.width: 2

                                    scale: pressed ? 0.98 : 1.0

                                    Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }
                                    Behavior on color { ColorAnimation { duration: 100 } }

                                    Row 
                                    {
                                        anchors.left: parent.left
                                        anchors.rightMargin: 8
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.leftMargin: 12
                                        spacing: 8

                                    Text 
                                    {
                                        text: "test topic"
                                        color: Theme.textColor
                                        elide: Text.ElideRight
                                        verticalAlignment: Text.AlignVCenter
                                        anchors.verticalCenter: topic.verticalCenter
                                        width: topic.width
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
                                                topic.pressed = false
                                            } 
                                            else if (mouse.button === Qt.LeftButton) topic.pressed = true
                                        }

                                        onReleased: function(mouse)
                                        {
                                            if (mouse.button === Qt.LeftButton) topic.pressed = false
                                        }

                                        onCanceled: 
                                        {
                                            topic.pressed = false
                                        }
                                    }
                                }
                        }

                        Example.ThemedButton 
                        {
                            text: qsTr("Закрыть")
                            onClicked: win.help_mode = false
                        }
                    }
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
        id: dragOverlay
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