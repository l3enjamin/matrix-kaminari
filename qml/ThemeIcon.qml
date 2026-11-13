import QtQuick
import MatrixClient

Item {
    id: root

    property string source: ""
    property string fallback: "settings"
    property int size: 18
    property color color: AppTheme.textSecondary
    property bool isMask: true

    implicitWidth: size
    implicitHeight: size

    Icon {
        anchors.centerIn: parent
        name: root.fallback.length > 0 ? root.fallback : "settings"
        size: root.size
        color: root.color
    }
}
