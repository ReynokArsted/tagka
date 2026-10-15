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

    contentItem: ColumnLayout
    {
        spacing: 10

        Text
        {
            text: qsTr("Настройки")
            font.pixelSize: 18
            color: Theme.textColor
        }

        Example.ThemedButton
        {
            text: Theme.isDarkMode ? qsTr("Установить светлую тему") : qsTr("Установить тёмную тему")
            onClicked: Theme.isDarkMode = !Theme.isDarkMode
        }

        Example.ThemedButton
        {
            text: Translator.language === "en" ? "Change language to Russian" : "Изменить язык на английский"
            onClicked: Translator.language = Translator.language === "en" ? "ru" : "en"
        }

        Example.ThemedButton
        {
            text: qsTr("Применить")
            onClicked: root.applyClicked()
        }

        Example.ThemedButton
        {
            text: qsTr("Закрыть")
            onClicked: root.close()
        }
    }
}
