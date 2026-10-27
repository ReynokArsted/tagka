import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

import untitled 1.0
import "./" as Example

Dialog
{
    id: root

    signal topicClicked(int index, string topic)

    readonly property var topics:
    [
        { question: qsTr("Что такое метка?"),                                  answer: "" },
        { question: qsTr("Как задать метку файла?"),                           answer: "" },
        { question: qsTr("Как задать метки для нескольких файлов?"),           answer: "" },
        { question: qsTr("Как открыть, переименовать или удалить файлы?"),     answer: "" },
        { question: qsTr("Как искать файлы по меткам и по пути?"),             answer: "" }
    ]

    property int currentTopic: -1
    readonly property bool answerOpened: currentTopic >= 0

    function openTopic(index)
    {
        currentTopic = index
        topicClicked(index, topics[index].question)
    }

    function back()
    {
        if (answerOpened)
            currentTopic = -1
        else
            close()
    }

    onAboutToShow: currentTopic = -1
    onCurrentTopicChanged: answer_view.contentY = 0

    modal: false
    closePolicy: Popup.NoAutoClose

    anchors.centerIn: Overlay.overlay
    width: Overlay.overlay ? Overlay.overlay.width - 8 : 0
    height: Overlay.overlay ? Overlay.overlay.height - 8 : 0

    Shortcut
    {
        sequence: "Esc"
        enabled: root.visible
        onActivated: root.back()
    }

    background: Rectangle
    {
        radius: 4
        color: Theme.fieldBackground
        border.width: 1
        border.color: Theme.borderColor
    }

    component TopicCard: Rectangle
    {
        id: card

        property string title: ""

        signal clicked()

        implicitHeight: 48
        radius: 8
        color: mouse_area.containsMouse ? Theme.fieldBackground : Theme.backgroundColor
        border.width: 1
        border.color: Theme.borderColor

        scale: mouse_area.pressed ? 0.98 : 1.0

        Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }
        Behavior on color { ColorAnimation { duration: 100 } }

        Text
        {
            anchors.left: parent.left
            anchors.right: chevron.left
            anchors.leftMargin: 14
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            text: card.title
            font.pixelSize: 15
            color: Theme.textColor
            elide: Text.ElideRight
        }

        Text
        {
            id: chevron

            anchors.right: parent.right
            anchors.rightMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            text: "›"
            font.pixelSize: 22
            color: Theme.textColor
            opacity: 0.6
        }

        MouseArea
        {
            id: mouse_area

            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: card.clicked()
        }
    }

    contentItem: ColumnLayout
    {
        spacing: 10

        Text
        {
            Layout.fillWidth: true
            text: root.answerOpened ? root.topics[root.currentTopic].question : qsTr("Справочник")
            font.pixelSize: 18
            color: Theme.textColor
            wrapMode: Text.WordWrap
        }

        StackLayout
        {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.answerOpened ? 1 : 0

            ListView
            {
                clip: true
                spacing: 6
                boundsBehavior: Flickable.StopAtBounds
                model: root.topics

                delegate: TopicCard
                {
                    required property int index
                    required property var modelData

                    width: ListView.view.width
                    title: modelData.question
                    onClicked: root.openTopic(index)
                }
            }

            Flickable
            {
                id: answer_view

                clip: true
                contentWidth: width
                contentHeight: answer_text.implicitHeight
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {}

                Text
                {
                    id: answer_text

                    readonly property string answer: root.answerOpened ? root.topics[root.currentTopic].answer : ""

                    width: answer_view.width
                    text: answer !== "" ? answer : qsTr("Ответ пока не добавлен")
                    textFormat: Text.StyledText
                    font.pixelSize: 15
                    color: Theme.textColor
                    opacity: answer !== "" ? 1.0 : 0.6
                    wrapMode: Text.WordWrap
                }
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

            Example.ThemedButton
            {
                visible: root.answerOpened
                text: qsTr("Назад")
                onClicked: root.back()
            }

            Item { Layout.fillWidth: true }

            Example.ThemedButton
            {
                text: qsTr("Закрыть")
                onClicked: root.close()
            }
        }
    }
}
