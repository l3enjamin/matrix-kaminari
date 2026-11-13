import QtQuick.Controls
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    readonly property bool kdeWindowActive: true

    controlsVisible: false
    pageStack.visible: false
    pageStack.enabled: false
    pageStack.focus: false
    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.None

    // Keep Main.qml's configurable app.quit Shortcut unambiguous and preserve quitRequested.
    Component.onCompleted: {
        if (quitAction)
            quitAction.enabled = false
    }
}
