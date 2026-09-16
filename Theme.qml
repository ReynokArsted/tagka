pragma Singleton
import QtQuick

QtObject {
    id: root

    property bool isDarkMode: false
    property bool isTagSelectMode: false

    readonly property color lightBg: "#FDFDFD"
    readonly property color lightText: "#142528"
    readonly property color lightFieldBg: "#FDFDFD"
    readonly property color lightBorder: "#CCCCCC"

    readonly property color darkBg: "#1E1E1E"
    readonly property color darkText: "#FDFDFD"
    readonly property color darkFieldBg: "#2D2D2D"
    readonly property color darkBorder: "#444444"

    readonly property color onBorder: '#81d8bb'
    readonly property color offBorder: "transparent"

    property color backgroundColor: isDarkMode ? darkBg : lightBg
    property color textColor: isDarkMode ? darkText : lightText
    property color fieldBackground: isDarkMode ? darkFieldBg : lightFieldBg
    property color borderColor: isDarkMode ? darkBorder : lightBorder
    property color tagsBorder: isTagSelectMode ? darkBorder : lightBorder
}
