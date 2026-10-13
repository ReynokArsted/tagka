import QtQuick
import QtQuick.Layouts

RowLayout 
{
    id: search_panel

    property var search
    property var parentWin: null
    property alias path: input.text
    property alias text: input.text
    property alias tagInput: input

    signal folderChanged(string path)
    signal candidatesChangedExternally(var candidates)

    //spacing: 10

    Connections 
    {
        target: search_panel.search.paths
        function onCandidatesChanged() 
        {
            const c = search_panel.search.paths.candidates
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

            text: qsTr("Поле поиска")
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
                if (search_panel.search)
                    search_panel.search.paths.updateCandidates(text)
            }

            onActiveFocusChanged: 
            {
                if (activeFocus && search_panel.search.paths)
                    search_panel.search.paths.updateCandidates(text)
            }

            onAccepted: 
            {
                const m = search_panel.search
                if (m && m.paths.candidates.length > 0) 
                {
                    text = m.paths.acceptFirstCandidate(text)
                } 
                else 
                {
                    search_panel.folderChanged(text)
                    if (m) m.doRequest(text)
                }
            }

            Keys.onPressed: function(event) 
            {
                if (event.key === Qt.Key_Tab) 
                {
                    event.accepted = true
                    if (search_panel.search.paths)
                        text = search_panel.search.paths.completePrefix(text)
                } 
                else if (event.key === Qt.Key_Escape) 
                {
                    event.accepted = true
                    if (search_panel.search.paths)
                        search_panel.search.paths.clearCandidates()
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
            const p = search_panel.search.paths.parentOf(input.text)
            if (p !== "")
            {
                search_panel.search.doRequest(p)
                input.text = p.endsWith("/") ? p : p + "/"
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
            search_panel.search.doRequest(input.text)
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
