import QtQuick
import org.kde.kirigami as Kirigami
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

    Kirigami.Icon {
        id: kdeIcon
        anchors.fill: parent
        source: root.source
        color: root.color
        isMask: root.isMask
        visible: valid
    }

    Icon {
        anchors.centerIn: parent
        visible: !kdeIcon.valid
        name: root.fallback.length > 0 ? root.fallback : "settings"
        size: root.size
        color: root.color
    }
}
