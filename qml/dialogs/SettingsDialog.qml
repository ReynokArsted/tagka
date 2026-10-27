import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

import untitled 1.0
import untitled.files 1.0
import "./" as Example

Dialog
{
    id: root

    signal applyClicked()

    property bool showDeleteWarnings: true
    property bool showHiddenFiles: false

    readonly property var items:
    [
        { key: "darkMode",           title: qsTr("Тёмная тема") },
        { key: "english",            title: qsTr("Английский язык интерфейса") },
        { key: "showDeleteWarnings", title: qsTr("Показ предупреждений об удалении") },
        { key: "showHiddenFiles",    title: qsTr("Показ скрытых и системных файлов") }
    ]

    property var draft: []

    readonly property bool dirty: JSON.stringify(draft) !== JSON.stringify(appliedValues())

    function appliedValues()
    {
        return [ Theme.isDarkMode, Translator.language === "en", showDeleteWarnings, showHiddenFiles ]
    }

    function valueOf(key)
    {
        const i = items.findIndex(item => item.key === key)
        return draft[i] === true
    }

    function toggle(index)
    {
        const next = draft.slice()
        next[index] = !next[index]
        draft = next
    }

    function setValues(values)
    {
        Theme.isDarkMode = values.darkMode === true
        Translator.language = values.english === true ? "en" : "ru"
        showDeleteWarnings = values.showDeleteWarnings === true
        showHiddenFiles = values.showHiddenFiles === true
    }

    function load()
    {
        setValues(Storage.userSettings())
    }

    function apply()
    {
        const values = {}
        for (let i = 0; i < items.length; i++)
            values[items[i].key] = draft[i] === true

        setValues(values)
        if (!Storage.setUserSettings(values))
            console.warn("ERROR: cannot save user settings")
        applyClicked()
    }

    Component.onCompleted: load()
    onAboutToShow: draft = appliedValues()

    modal: false
    closePolicy: Popup.NoAutoClose

    anchors.centerIn: Overlay.overlay
    width: Overlay.overlay ? Overlay.overlay.width - 8 : 0
    height: Overlay.overlay ? Overlay.overlay.height - 8 : 0

    background: Rectangle
    {
        radius: 4
        color: Theme.fieldBackground
        border.width: 1
        border.color: Theme.borderColor
    }

    component SettingCard: Rectangle
    {
        id: card

        property string title: ""
        property bool checked: false

        signal toggled()

        implicitHeight: 48
        radius: 8
        color: mouse_area.containsMouse ? Theme.fieldBackground : Theme.backgroundColor
        border.width: 1
        border.color: Theme.borderColor

        Behavior on color { ColorAnimation { duration: 100 } }

        Text
        {
            anchors.left: parent.left
            anchors.right: track.left
            anchors.leftMargin: 14
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            text: card.title
            font.pixelSize: 15
            color: Theme.textColor
            elide: Text.ElideRight
        }

        Rectangle
        {
            id: track

            anchors.right: parent.right
            anchors.rightMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            width: 40
            height: 22
            radius: height / 2
            color: card.checked ? Theme.accentColor : Theme.fieldBackground
            border.width: 1
            border.color: card.checked ? Qt.darker(Theme.accentColor, 1.2) : Theme.borderColor

            Behavior on color { ColorAnimation { duration: 120 } }

            Rectangle
            {
                width: 16
                height: 16
                radius: width / 2
                anchors.verticalCenter: parent.verticalCenter
                x: card.checked ? parent.width - width - 3 : 3
                color: "#FDFDFD"
                border.width: 1
                border.color: Theme.borderColor

                Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }
            }
        }

        MouseArea
        {
            id: mouse_area

            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: card.toggled()
        }
    }

    contentItem: ColumnLayout
    {
        spacing: 10

        Text
        {
            text: qsTr("Настройки")
            font.pixelSize: 18
            color: Theme.textColor
        }

        ListView
        {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 6
            boundsBehavior: Flickable.StopAtBounds
            model: root.items

            delegate: SettingCard
            {
                required property int index
                required property var modelData

                width: ListView.view.width
                title: modelData.title
                checked: root.draft[index] === true
                onToggled: root.toggle(index)
            }
        }

        Rectangle
        {
            Layout.fillWidth: true
            implicitHeight: 1
            color: Theme.borderColor
        }

        RowLayout
        {
            Layout.fillWidth: true
            spacing: 8

            Item { Layout.fillWidth: true }

            Example.ThemedButton
            {
                text: qsTr("Закрыть")
                onClicked: root.close()
            }

            Example.ThemedButton
            {
                text: qsTr("Применить")
                enabled: root.dirty
                opacity: enabled ? 1.0 : 0.5
                onClicked: root.apply()
            }
        }
    }
}
