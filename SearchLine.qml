import QtQuick
import QtQuick.Layouts

RowLayout {
    id: search_panel

    property var fileModel
    property var candidates: []
    property string currentDirPath: ""
    property string currentPrefix: ""
    property Window parentWin: null

    property alias path: input.text
    property alias text: input.text
    property alias tagInput: input

    signal folderChanged(string path)
    signal candidatesChangedExternally(var candidates)

    spacing: 10

    function detectSep(value) {
        return value.indexOf("\\") !== -1 ? "\\" : "/"
    }

    function normalizeForLogic(value) {
        return (value || "").replace(/\\/g, "/")
    }

    function splitDirAndPrefix(normInput) {
        if (normInput.length === 0)
            return {
                dirPath: "",
                prefix: ""
            }

        const lastSlash = normInput.lastIndexOf("/")

        if (lastSlash === -1) {
            return {
                dirPath: "",
                prefix: normInput
            }
        }

        return {
            dirPath: normInput.slice(0, lastSlash + 1),
            prefix: normInput.slice(lastSlash + 1)
        }
    }

    function commonPrefixFromCandidates(cands, prefix) {
        if (!cands || cands.length === 0)
            return ""

        let commonPrefix = cands[0].name

        for (let i = 1; i < cands.length; ++i) {
            const name = cands[i].name

            while (commonPrefix.length > 0 &&
                   !name.startsWith(commonPrefix)) {
                commonPrefix = commonPrefix.slice(
                    0,
                    commonPrefix.length - 1
                )
            }

            if (commonPrefix.length === 0)
                break
        }

        if (commonPrefix.length < prefix.length)
            return prefix

        return commonPrefix
    }

    function refreshCandidates() {
        if (!search_panel.fileModel)
            return

        const value = input.text
        const normalized = normalizeForLogic(value)
        const parts = splitDirAndPrefix(normalized)

        search_panel.currentDirPath = parts.dirPath
        search_panel.currentPrefix = parts.prefix

        // Для пустого пути completion отключён.
        // При необходимости здесь можно использовать текущую папку.
        if (search_panel.currentDirPath === "") {
            search_panel.candidates = []
            return
        }

        search_panel.candidates = search_panel.fileModel.getCandidates(
            search_panel.currentDirPath,
            search_panel.currentPrefix
        )

        console.log("currentDirPath:", search_panel.currentDirPath)
        console.log("currentPrefix:", search_panel.currentPrefix)

        if (search_panel.candidates.length > 0) {
            search_panel.fileModel.showCandidates(search_panel.candidates)
            search_panel.candidatesChangedExternally(search_panel.candidates)
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 40

        color: Theme.fieldBackground
        border.color: Theme.borderColor
        border.width: 1
        radius: 4

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter

            text: qsTr("Поиск?")
            color: "#888"
            font.pixelSize: 16
            visible: input.text.length === 0
        }

        TextInput {
            id: input

            anchors.fill: parent
            anchors.margins: 8

            horizontalAlignment: Text.AlignLeft

            color: Theme.textColor
            font.pixelSize: 16
            clip: true

            onTextChanged: search_panel.refreshCandidates()

            onActiveFocusChanged: {
                if (activeFocus)
                    search_panel.refreshCandidates()
            }

            //onAccepted: search_panel.folderChanged(text)
            onAccepted:
            {
                if (search_panel.candidates && search_panel.candidates.length > 0) 
                {
                    search_panel.acceptFirstCandidate()
                }
                else 
                {
                    search_panel.folderChanged(text)
                    if (search_panel.fileModel)
                        search_panel.fileModel.setFolder(text)
                }
            }

            Keys.onPressed: function(event) {
                if (event.key === Qt.Key_Tab) {
                    event.accepted = true
                    completeCommonPrefix()
                // } else if (event.key === Qt.Key_Return ||
                //            event.key === Qt.Key_Enter) {
                //     event.accepted = true
                //     acceptFirstCandidate()
                } else if (event.key === Qt.Key_Escape) {
                    event.accepted = true
                    search_panel.candidates = []
                }
            }
        }
    }

    ThemedButton 
    {
        text: qsTr("..")
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40

            onClicked: {
                const p = fileModel.parent_folder()

                if (p !== "" && fileModel.hasFolder) {
                    fileModel.setFolder(p)
                    input.text = p
                }
            }
        }
        ThemedButton {
            text: qsTr("+")
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40

            onClicked: {
                ThingModel.listOfThingies.addThing(input.text)
            }
        }

        ThemedButton {
            visible: parentWin.tagSelectMode
            text: "V"
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40

            onClicked: {
                parentWin.confirmTagging()
                fileModel.setFolder(input.text)
            }
        }

        ThemedButton {
            visible: parentWin.tagSelectMode
            text: qsTr("X")
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40

            onClicked: parentWin.cancelTagging()
        }

    function completeCommonPrefix() {
        if (!search_panel.candidates || search_panel.candidates.length === 0)
            return

        const separator = detectSep(input.text)
        const normalized = normalizeForLogic(input.text)
        const parts = splitDirAndPrefix(normalized)

        const commonPrefix = commonPrefixFromCandidates(
            search_panel.candidates,
            parts.prefix
        )

        if (!commonPrefix ||
            commonPrefix.length <= parts.prefix.length) {
            return
        }

        const dirPath = parts.dirPath.split("/").join(separator)
        input.text = dirPath + commonPrefix

        if (search_panel.candidates.length === 1 &&
            search_panel.candidates[0].isDir &&
            !input.text.endsWith(separator)) {
            input.text += separator
        }

        search_panel.refreshCandidates()
    }

    function acceptFirstCandidate() {
        if (!search_panel.candidates || search_panel.candidates.length === 0)
            return

        const separator = detectSep(input.text)
        const normalized = normalizeForLogic(input.text)
        const parts = splitDirAndPrefix(normalized)
        const candidate = search_panel.candidates[0]

        const dirPath = parts.dirPath.split("/").join(separator)
        input.text = dirPath + candidate.name

        if (candidate.isDir &&
            !input.text.endsWith(separator)) {
            input.text += separator
        }

        search_panel.refreshCandidates()
    }
}
