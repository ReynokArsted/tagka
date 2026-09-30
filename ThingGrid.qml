import QtQuick 
import QtQml.Models
import QtQuick.Controls
import QtQuick.Effects

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
    height: 200

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

                model: ThingModel.listOfThingies

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

                        if (from !== to) ThingModel.listOfThingies.move(from, to)
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
                                ThingModel.listOfThingies.move(delegateRoot.index, 0)
                        }

                        onRightClicked: tagMenu.popup()

                        Menu 
                        {
                            id: tagMenu

                            MenuItem 
                            {
                                text: qsTr("Показать файлы по метке")

                                onTriggered: 
                                {
                                    tag_flow.tagFilesRequested(delegateRoot.id, delegateRoot.name)
                                    if (delegateRoot.index !== 0)
                                        ThingModel.listOfThingies.move(delegateRoot.index, 0)
                                }
                            }

                            MenuItem 
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

                            color: "white"
                            text: delegateRoot.name
                            font.pixelSize: 18
                        }
                    }
                }    
            }
        }
    }
}
