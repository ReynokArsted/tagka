import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import untitled 1.0

Rectangle 
{
    id: bottom_bar

    property string title_text: ""
    property real corner_radius: 0
    property Window move_target: null

    height: 36
    color: Theme.backgroundColor
    radius: corner_radius
    clip: true

    RowLayout 
    {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 8

        Item { Layout.fillWidth: true }
    }
}