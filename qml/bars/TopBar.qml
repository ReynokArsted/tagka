import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import untitled 1.0

Rectangle 
{
    id: top_bar

    property string title_text: ""
    property real corner_radius: 0
    property Window move_target: null

    signal minimizeClicked()
    signal closeClicked()

    height: 36
    color: Theme.backgroundColor
    radius: corner_radius
    clip: true

    MouseArea 
    { 
        anchors.fill: parent; 
        onPressed: if (top_bar.move_target) top_bar.move_target.startSystemMove() 
    }

    RowLayout 
    {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 8

        Text 
        { 
            text: top_bar.title_text 
            color: Theme.textColor 
        }

        Item { Layout.fillWidth: true }

        ThemedButton 
        { 
            text: "—"
            showBorder: false
            onClicked: top_bar.minimizeClicked()
        }
        ThemedButton
        { 
            text: "x"
            showBorder: false
            onClicked: top_bar.closeClicked() 
        }
    }
}