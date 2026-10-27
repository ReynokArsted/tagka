import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls

import untitled 1.0

Rectangle 
{
    id: bottom_bar

    property string title_text: ""
    property real corner_radius: 0
    property Window move_target: null

    signal settingsClicked()
    signal helpClicked()

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

        RowLayout 
        {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
                            
            ThemedButton 
            {
                text: "⚙"
                onClicked: bottom_bar.settingsClicked()
            }
            ThemedButton 
            {
                id: help_button

                text: "?"
                onClicked: bottom_bar.helpClicked()

                ToolTip 
                {
                    id: help_tooltip

                    popupType: Popup.Window
                    visible: help_button.hovered
                    delay: 1000
                    timeout: 5000
                    text: qsTr("tip test")

                    x: (help_button.width - width) / 2
                    y: -height - 8

                    contentItem: Text 
                    {
                        id: tooltip_text

                        text: help_tooltip.text
                        color: Theme.textColor
                        wrapMode: Text.WordWrap
                        
                        width: help_tooltip.width
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: 12
                        rightPadding: 12
                        topPadding: 8
                        bottomPadding: 18
                    }

                    background: Item 
                    {
                        implicitWidth: tooltip_text.implicitWidth
                        implicitHeight: tooltip_text.implicitHeight

                        Canvas 
                        {
                            id: canvas

                            anchors.fill: parent

                            Connections
                            {
                                target: Theme
                                function onIsDarkModeChanged() { canvas.requestPaint() }
                            }

                            onPaint: 
                            {
                                var ctx = getContext("2d")
                                ctx.reset()

                                var w = width
                                var h = height
                                var r = 14
                                var tailWidth = 20
                                var tailHeight = 10
                                var bodyBottom = h - tailHeight

                                ctx.beginPath()
                                ctx.moveTo(r, 0)
                                ctx.lineTo(w - r, 0)
                                ctx.quadraticCurveTo(w, 0, w, r)
                                ctx.lineTo(w, bodyBottom - r)
                                ctx.quadraticCurveTo(w, bodyBottom, w - r, bodyBottom)

                                ctx.lineTo((w + tailWidth) / 2, bodyBottom)
                                ctx.lineTo(w / 2, h)
                                ctx.lineTo((w - tailWidth) / 2, bodyBottom)

                                ctx.lineTo(r, bodyBottom)
                                ctx.quadraticCurveTo(0, bodyBottom, 0, bodyBottom - r)
                                ctx.lineTo(0, r)
                                ctx.quadraticCurveTo(0, 0, r, 0)
                                ctx.closePath()

                                ctx.fillStyle = Theme.fieldBackground
                                ctx.fill()

                                ctx.strokeStyle = Theme.textColor
                                ctx.lineWidth = 2
                                ctx.lineJoin = "round"
                                ctx.stroke()
                            }
                        }
                    }
                }
            }
        }
    }
}