import QtQuick
import QtQuick.Layouts

RowLayout {
    id: search_panel

    spacing: 10

    property var fileModel
    property var parentWin: null
    property alias path: input.text
    property alias text: input.text
    property alias tagInput: input

    signal folderChanged(string path)
    signal candidatesChangedExternally(var candidates)

    Connections 
    {
        target: search_panel.fileModel
        function onCandidatesChanged() 
        {
            const c = search_panel.fileModel.candidates
            if (c.length > 0)
                search_panel.candidatesChangedExternally(c)
        }
    }

    Rectangle 
    {
        Layout.fillWidth: true
        Layout.preferredHeight: 40

        color: Theme.fieldBackground
        border.color: Theme.borderColor
        border.width: 1
        radius: 4

        Text 
        {
            visible: input.text.length === 0

            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter

            text: qsTr("Поиск?")
            color: "#888"
            font.pixelSize: 16
        }

        TextInput 
        {
            id: input

            anchors.fill: parent
            anchors.margins: 8
            horizontalAlignment: Text.AlignLeft

            color: Theme.textColor
            font.pixelSize: 16
            clip: true

            onTextChanged: 
            {
                if (search_panel.fileModel)
                    search_panel.fileModel.updateCandidates(text)
            }

            onActiveFocusChanged: 
            {
                if (activeFocus && search_panel.fileModel)
                    search_panel.fileModel.updateCandidates(text)
            }

            onAccepted: 
            {
                const m = search_panel.fileModel
                if (m && m.candidates.length > 0) 
                {
                    text = m.acceptFirstCandidate(text)
                } 
                else 
                {
                    search_panel.folderChanged(text)
                    if (m) m.setFolder(text)
                }
            }

            Keys.onPressed: function(event) 
            {
                if (event.key === Qt.Key_Tab) 
                {
                    event.accepted = true
                    if (search_panel.fileModel)
                        text = search_panel.fileModel.completePrefix(text)
                } 
                else if (event.key === Qt.Key_Escape) 
                {
                    event.accepted = true
                    if (search_panel.fileModel)
                        search_panel.fileModel.clearCandidates()
                }
            }
        }
    }

    ThemedButton 
    {
        text: qsTr("..")
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40

            onClicked: 
            {
                const p = fileModel.parent_folder()
                if (p !== "" && fileModel.hasFolder) 
                {
                    console.log(p);
                    fileModel.setFolder(p)
                    if (fileModel.parent_folder() !== "") input.text = p + "/"
                    else input.text = p
                }
            }
        }
    ThemedButton 
    {
        text: qsTr("+")
        Layout.preferredWidth: 40
        Layout.preferredHeight: 40

        onClicked: ThingModel.listOfThingies.addThing(input.text)
    }

    ThemedButton 
    {
        visible: parentWin.tagSelectMode
        text: "V"
        Layout.preferredWidth: 40
        Layout.preferredHeight: 40

        onClicked: 
        {
            parentWin.confirmTagging()
            fileModel.setFolder(input.text)
        }
    }

    ThemedButton 
    {
        visible: parentWin.tagSelectMode
        text: qsTr("X")
        Layout.preferredWidth: 40
        Layout.preferredHeight: 40

        onClicked: parentWin.cancelTagging()
    }
}
