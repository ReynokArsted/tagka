import untitled
import QtQuick
import QtQuick.Controls.Basic

MenuItem
{
    id: root

    implicitHeight: 32

    contentItem: Text
    {
        leftPadding: 10
        rightPadding: 10
        text: root.text
        font.pixelSize: 14
        color: Theme.textColor
        opacity: root.enabled ? 1.0 : 0.5
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle
    {
        implicitWidth: 220
        radius: 4
        color: root.highlighted ? Qt.alpha(Theme.accentColor, 0.35) : "transparent"
    }
}
