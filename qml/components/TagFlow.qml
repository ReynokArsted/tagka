import QtQuick
import QtQuick.Controls

import untitled 1.0
import "./" as Example

Item
{
    id: tag_flow

    property bool highlightBorders: false
    property bool isTagSelectionMode: false
    property var selectedTagIds: []
    property Item dragOverlay

    signal tagSelected(int id, string name)
    signal tagFilesRequested(int id, string name)
    signal tagDeleteRequested(int id, string name)

    width: 600
    height: 48

    TagColorDialog
    {
        id: tag_color_dialog

        onColorChosen: (tagId, color) =>
        {
            if (!ThingieListModel.setThingColor(tagId, color))
                console.warn("ERROR: cannot change color of tag", tagId)
        }
    }

    Flickable 
    {
        id: flickable

        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: flow.implicitHeight
        boundsBehavior: Flickable.StopAtBounds

        Flow 
        {
            id: flow
    
            width: flickable.width
            spacing: 8
            clip: true

            property int cellHeight: 48

            move: Transition 
            {
                NumberAnimation 
                {
                    properties: "x,y"
                    duration: 200
                    easing.type: Easing.OutQuad
                }
            }

            Repeater 
            {
                id: repeater

                model: ThingieListModel

                delegate: DropArea 
                {
                    id: delegateRoot

                    required property color color
                    required property string name
                    required property int id
                    required property int index 

                    width: nameMetrics.width + 24
                    height: flow.cellHeight

                    readonly property bool isSelected:
                        tag_flow.isTagSelectionMode &&
                        tag_flow.selectedTagIds.indexOf(id) !== -1

                    TextMetrics 
                    {
                        id: nameMetrics

                        font.pixelSize: 18
                        text: delegateRoot.name
                    }

                    onEntered: function (drag) 
                    {
                        const sourceTile = drag.source

                        if (!sourceTile || sourceTile === thingTile) return

                        const from = sourceTile.visualIndex 
                        const to = delegateRoot.index

                        if (from !== to) 	ThingieListModel.move(from, to)
                    }

                    onDropped: function (drag) {}

                    Example.ThingTile 
                    {
                        id: thingTile

                        width: delegateRoot.width
                        height: delegateRoot.height

                        dragParent: tag_flow.dragOverlay
                        visualIndex: delegateRoot.index

                        color: delegateRoot.color
                        borderHighlight: delegateRoot.isSelected

                        onClicked: 
                        {
                            tag_flow.tagSelected(delegateRoot.id, delegateRoot.name)
                            if (delegateRoot.index !== 0)
                                	ThingieListModel.move(delegateRoot.index, 0)
                        }

                        onRightClicked: tagMenu.popup()

                        ThemedMenu
                        {
                            id: tagMenu

                            ThemedMenuItem
                            {
                                text: qsTr("Показать файлы по метке")

                                onTriggered: 
                                {
                                    tag_flow.tagFilesRequested(delegateRoot.id, delegateRoot.name)
                                    if (delegateRoot.index !== 0)
                                        	ThingieListModel.move(delegateRoot.index, 0)
                                }
                            }

                            ThemedMenuItem
                            {
                                text: qsTr("Изменить цвет")

                                onTriggered: tag_color_dialog.openFor(delegateRoot.id, delegateRoot.name, delegateRoot.color)
                            }

                            ThemedMenuItem
                            {
                                text: qsTr("Удалить метку")

                                onTriggered: 
                                {
                                    tag_flow.tagDeleteRequested(delegateRoot.id, delegateRoot.name)
                                }
                            }
                        }

                        Text 
                        {
                            anchors.fill: parent
                            anchors.margins: 5
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter

                            color: tag_color_dialog.textColorOn(delegateRoot.color)
                            text: delegateRoot.name
                            font.pixelSize: 18
                        }
                    }
                }    
            }
        }
    }
}
