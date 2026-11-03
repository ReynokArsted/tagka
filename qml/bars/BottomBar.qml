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

                    readonly property int tailWidth: 16
                    readonly property int tailHeight: 8
                    readonly property int radius: 10
                    readonly property int maxTextWidth: 280
                    readonly property int contentPaddingH: 14
                    readonly property int contentPaddingV: 10

                    popupType: Popup.Window
                    visible: help_button.hovered
                    delay: 400
                    timeout: 6000
                    padding: 0
                    text: qsTr("TIP TEST")

                    x: (help_button.width - width) / 2
                    y: -height - 6

                    implicitWidth: Math.min(maxTextWidth, tooltip_text.implicitWidth) + 2 * contentPaddingH
                    implicitHeight: tooltip_text.implicitHeight + 2 * contentPaddingV + tailHeight

                    enter: Transition 
                    {
                        NumberAnimation 
                        { 
                            property: "opacity"; 
                            from: 0; to: 1; 
                            duration: 180; 
                            easing.type: Easing.OutCubic 
                        }
                        NumberAnimation 
                        {
                            property: "scale"; 
                            from: 0.92; to: 1; 
                            duration: 180; 
                            easing.type: Easing.OutCubic 
                        }
                    }
                    exit: Transition 
                    {
                        NumberAnimation 
                        { 
                            property: "opacity"; 
                            from: 1; to: 0; 
                            duration: 120 
                        }
                    }

                    transformOrigin: Popup.Bottom

                    contentItem: Text 
                    {
                        id: tooltip_text

                        text: help_tooltip.text
                        color: Theme.textColor
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter

                        x: help_tooltip.contentPaddingH
                        y: (help_tooltip.height - help_tooltip.tailHeight - height) / 2
                        width: Math.min(help_tooltip.maxTextWidth, implicitWidth)
                    }
                    
                    background: Item 
                    {
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
                                var r = help_tooltip.radius
                                var tw = help_tooltip.tailWidth
                                var th = help_tooltip.tailHeight
                                var bodyBottom = h - th
                                var inset = 1

                                ctx.shadowColor = "rgba(0, 0, 0, 0.35)"
                                ctx.shadowBlur = 10
                                ctx.shadowOffsetY = 3

                                ctx.beginPath()
                                ctx.moveTo(inset + r, inset)
                                ctx.lineTo(w - inset - r, inset)
                                ctx.quadraticCurveTo(w - inset, inset, w - inset, inset + r)
                                ctx.lineTo(w - inset, bodyBottom - r)
                                ctx.quadraticCurveTo(w - inset, bodyBottom, w - inset - r, bodyBottom)

                                ctx.lineTo(w / 2 + tw / 2, bodyBottom)
                                ctx.quadraticCurveTo(w / 2 + tw / 4, bodyBottom, w / 2 + 2, bodyBottom + th - 2)
                                ctx.quadraticCurveTo(w / 2, bodyBottom + th, w / 2 - 2, bodyBottom + th - 2)
                                ctx.quadraticCurveTo(w / 2 - tw / 4, bodyBottom, w / 2 - tw / 2, bodyBottom)

                                ctx.lineTo(inset + r, bodyBottom)
                                ctx.quadraticCurveTo(inset, bodyBottom, inset, bodyBottom - r)
                                ctx.lineTo(inset, inset + r)
                                ctx.quadraticCurveTo(inset, inset, inset + r, inset)
                                ctx.closePath()

                                ctx.fillStyle = Theme.fieldBackground
                                ctx.fill()

                                ctx.shadowColor = "transparent"
                                ctx.strokeStyle = Theme.textColor
                                ctx.globalAlpha = 0.25
                                ctx.lineWidth = 1.5
                                ctx.lineJoin = "round"
                                ctx.stroke()
                                ctx.globalAlpha = 1.0
                            }
                        }
                    }
                }
            }
        }
    }
}