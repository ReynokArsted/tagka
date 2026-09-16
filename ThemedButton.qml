import untitled
import QtQuick

Item 
{
    id: button

    property alias text: label.text
    property int fontSize: 14
    property bool showBorder: true
    property bool pressed: false
    readonly property bool hovered: mouse_area.containsMouse

    signal clicked()

    implicitWidth: label.implicitWidth + 24
    implicitHeight: 32
    height: implicitHeight
    width: implicitWidth      

    Rectangle 
    {
        anchors.fill: parent
        radius: 4
        border.width: button.showBorder ? 1 : 0
        border.color: Theme.borderColor
        color: button.pressed ? Theme.fieldBackground : Theme.backgroundColor
    }

    Text 
    {
        id: label
        
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: button.fontSize
        color: Theme.textColor
        elide: Text.ElideRight
    }

    MouseArea 
    {
        id: mouse_area

        anchors.fill: parent
        hoverEnabled: true

        onPressed: button.pressed = true

        onReleased: 
        { 
            button.pressed = false
            button.clicked() 
        }

        onCanceled: button.pressed = false
    }
}