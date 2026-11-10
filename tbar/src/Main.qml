import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    width: 900
    height: 760
    minimumWidth: 720
    minimumHeight: 540
    title: qsTr("Touch Bar layouts")
    color: "#171717"
    Material.theme: Material.Dark
    Material.accent: desktop.accent
    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true
    Component.onCompleted: backend.startPreview()

    Shortcut {
        sequence: "?"
        context: Qt.ApplicationShortcut
        onActivated: help.open()
    }
    Shortcut {
        sequence: "Ctrl+Return"
        context: Qt.ApplicationShortcut
        onActivated: applyConfirm.open()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12

        Label {
            text: qsTr("Touch Bar layouts")
            font.pixelSize: 24
            font.bold: true
        }
        Label {
            text: backend.rendererAvailable
                ? qsTr("Renderer found. T1Bridge must also be running.")
                : qsTr("Renderer not found. Hardware support is also required.")
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }
        RowLayout {
            Label { text: qsTr("Layout") }
            ComboBox {
                id: profiles
                Layout.fillWidth: true
                model: backend.profiles
                currentIndex: backend.selected
                onActivated: function(index) { backend.select(index) }
            }
            Button { text: qsTr("New"); onClicked: createProfile.open() }
            Button { text: qsTr("Add examples"); onClicked: backend.addExamples() }
            Button {
                text: qsTr("Delete")
                enabled: backend.profiles.length > 1
                onClicked: deleteProfile.open()
            }
        }
        Label {
            text: qsTr("Escape is always present. Hold Fn for F1–F12.")
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }
        RowLayout {
            Label { text: qsTr("Live preview") }
            CheckBox {
                text: qsTr("Fn layer")
                checked: backend.previewFn
                onToggled: backend.previewFn = checked
            }
            Label {
                text: backend.previewError
                color: "#e05a5a"
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: "#000000"
            Image {
                anchors.fill: parent
                source: backend.preview
                cache: false
                fillMode: Image.PreserveAspectFit
                asynchronous: true
            }
        }
        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 4
            model: backend.buttons
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                required property int index
                required property var modelData
                width: list.width
                height: 54
                color: "#252525"
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    Label { text: modelData.label; font.bold: true; Layout.preferredWidth: 120; elide: Text.ElideRight }
                    Label { text: modelData.command; Layout.fillWidth: true; elide: Text.ElideMiddle }
                    ToolButton { text: "↑"; enabled: index > 0; onClicked: backend.moveButton(index, index - 1) }
                    ToolButton { text: "↓"; enabled: index + 1 < list.count; onClicked: backend.moveButton(index, index + 1) }
                    ToolButton { text: "×"; onClicked: backend.removeButton(index) }
                }
            }
        }
        RowLayout {
            TextField {
                id: labelInput
                Layout.preferredWidth: 160
                placeholderText: qsTr("Label")
                maximumLength: 16
            }
            TextField {
                id: commandInput
                Layout.fillWidth: true
                placeholderText: qsTr("Command, e.g. omarchy agent")
                maximumLength: 1024
            }
            Button {
                text: qsTr("Add button")
                enabled: backend.buttons.length < 12
                onClicked: if (backend.addButton(labelInput.text, commandInput.text)) {
                    labelInput.clear()
                    commandInput.clear()
                }
            }
        }
        RowLayout {
            ComboBox {
                id: statusKind
                model: [qsTr("Internet"), qsTr("CPU temperature"), qsTr("Application")]
            }
            TextField { id: statusLabel; placeholderText: qsTr("Label"); maximumLength: 16 }
            TextField {
                id: statusArgument
                Layout.fillWidth: true
                enabled: statusKind.currentIndex === 2
                placeholderText: qsTr("Process name, e.g. firefox")
                maximumLength: 15
            }
            Button {
                text: qsTr("Add status")
                enabled: backend.buttons.length < 12
                onClicked: if (backend.addStatus(["internet", "temperature", "process"][statusKind.currentIndex],
                                                  statusLabel.text, statusArgument.text)) {
                    statusLabel.clear()
                    statusArgument.clear()
                }
            }
        }
        RowLayout {
            ComboBox {
                id: widgetKind
                model: [qsTr("CPU graph"), qsTr("Memory graph"), qsTr("Network graph"),
                        qsTr("Disk graph"), qsTr("Battery")]
                Layout.fillWidth: true
            }
            Button {
                text: qsTr("Add live widget")
                enabled: backend.buttons.length < 12
                onClicked: backend.addWidget(["cpu", "memory", "network", "disk", "battery"][widgetKind.currentIndex])
            }
        }
        Label {
            text: qsTr("Commands run as your user. Only use commands you trust.")
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }
        RowLayout {
            Label { text: backend.message; Layout.fillWidth: true; wrapMode: Text.Wrap }
            Button { text: qsTr("Restore original"); onClicked: restoreConfirm.open() }
            Button { text: qsTr("Help (?)"); onClicked: help.open() }
            Button {
                text: qsTr("Apply layout")
                enabled: backend.profiles.length > 0
                highlighted: true
                onClicked: applyConfirm.open()
            }
        }
    }

    Dialog {
        id: createProfile
        anchors.centerIn: parent
        title: qsTr("New layout")
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: { backend.addProfile(profileName.text); profileName.clear() }
        TextField { id: profileName; placeholderText: qsTr("Layout name"); maximumLength: 40 }
    }
    Dialog {
        id: deleteProfile
        anchors.centerIn: parent
        title: qsTr("Delete layout?")
        modal: true
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: backend.deleteProfile()
        Label { text: qsTr("This removes the selected saved layout."); wrapMode: Text.Wrap; width: 350 }
    }
    Dialog {
        id: applyConfirm
        anchors.centerIn: parent
        title: qsTr("Apply layout?")
        modal: true
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: backend.apply()
        Label {
            text: qsTr("This replaces the omarchy-glance configuration and saves a backup. Commands run as your user.")
            wrapMode: Text.Wrap
            width: 390
        }
    }
    Dialog {
        id: restoreConfirm
        anchors.centerIn: parent
        title: qsTr("Restore original layout?")
        modal: true
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: backend.restoreOriginal()
        Label {
            text: qsTr("Restore the renderer configuration saved before the first apply.")
            wrapMode: Text.Wrap
            width: 390
        }
    }
    Dialog {
        id: help
        anchors.centerIn: parent
        title: qsTr("Keyboard shortcuts")
        modal: true
        standardButtons: Dialog.Close
        Label {
            text: qsTr("? — help\nCtrl+Enter — apply layout\nEscape — close dialog")
            wrapMode: Text.Wrap
        }
    }
}
