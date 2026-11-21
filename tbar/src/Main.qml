import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    width: 900
    height: 760
    minimumWidth: 360
    minimumHeight: 320
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

    ScrollView {
        id: editorScroll
        objectName: "editorScroll"
        anchors.fill: parent
        anchors.margins: 12
        clip: true
        contentWidth: Math.max(650, availableWidth)
        contentHeight: editorContent.implicitHeight
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AsNeeded

        ColumnLayout {
            id: editorContent
            width: editorScroll.contentWidth
            height: implicitHeight
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
            Label {
                text: backend.applied === "" ? qsTr("No layout from this app is on the Touch Bar yet.")
                                             : qsTr("On the Touch Bar now: %1").arg(backend.applied)
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
            RowLayout {
                Label { text: qsTr("Colors") }
                ComboBox {
                    id: themeStyle
                    objectName: "themeStyle"
                    Layout.fillWidth: true
                    readonly property var styles: ["", "auto", "tinted", "accent", "magenta", "blue",
                                                   "cyan", "green", "yellow", "orange", "red", "#"]
                    model: [qsTr("Default grey"), qsTr("Omarchy theme: subtle"), qsTr("Omarchy theme: tinted bar"),
                            qsTr("Omarchy theme: accent keys"), qsTr("Omarchy theme: magenta keys"),
                            qsTr("Omarchy theme: blue keys"), qsTr("Omarchy theme: cyan keys"),
                            qsTr("Omarchy theme: green keys"), qsTr("Omarchy theme: yellow keys"),
                            qsTr("Omarchy theme: orange keys"), qsTr("Omarchy theme: red keys"),
                            qsTr("Own colour")]
                    readonly property bool own: currentIndex === styles.length - 1
                    readonly property string chosen: own ? customColor.text : styles[currentIndex]
                    currentIndex: backend.themeStyle.charAt(0) === "#" ? styles.length - 1
                                                                       : Math.max(0, styles.indexOf(backend.themeStyle))
                    onActivated: function(index) { if (!own) backend.setThemeStyle(styles[index], false) }
                }
                TextField {
                    id: customColor
                    objectName: "customColor"
                    visible: themeStyle.own
                    Layout.preferredWidth: 110
                    maximumLength: 7
                    placeholderText: "#cba6f7"
                    text: backend.themeStyle.charAt(0) === "#" ? backend.themeStyle : ""
                    onAccepted: backend.setThemeStyle(text, false)
                }
                Button {
                    visible: themeStyle.own
                    text: qsTr("Set colour")
                    onClicked: backend.setThemeStyle(customColor.text, false)
                }
                Button {
                    text: qsTr("Use for all layouts")
                    onClicked: backend.setThemeStyle(themeStyle.chosen, true)
                }
            }
            Flow {
                visible: themeStyle.own
                Layout.fillWidth: true
                spacing: 6
                Repeater {
                    model: ["#cba6f7", "#b4befe", "#89b4fa", "#74c7ec", "#94e2d5", "#a6e3a1", "#f9e2af",
                            "#fab387", "#f38ba8", "#f5c2e7", "#ffffff", "#9399b2", "#45475a", "#7c3aed",
                            "#2563eb", "#059669", "#dc2626"]
                    Rectangle {
                        required property string modelData
                        width: 34
                        height: 34
                        radius: 4
                        color: modelData
                        border.width: backend.themeStyle === modelData ? 3 : 1
                        border.color: backend.themeStyle === modelData ? "#ffffff" : "#555555"
                        MouseArea {
                            anchors.fill: parent
                            onClicked: backend.setThemeStyle(parent.modelData, false)
                        }
                    }
                }
            }
            RowLayout {
                Label { text: qsTr("Escape") }
                TextField {
                    id: escLabel
                    Layout.preferredWidth: 110
                    maximumLength: 16
                    placeholderText: qsTr("Text")
                    text: backend.look.escLabel
                }
                TextField {
                    id: escIcon
                    Layout.preferredWidth: 70
                    maximumLength: 4
                    placeholderText: qsTr("Icon")
                    text: backend.look.escIcon
                    font.family: "JetBrainsMono Nerd Font"
                }
                ToolButton { text: "…"; onClicked: iconPicker.pick(escIcon) }
                Label { text: qsTr("Font") }
                TextField {
                    id: lookFont
                    Layout.fillWidth: true
                    maximumLength: 64
                    placeholderText: "JetBrainsMono Nerd Font"
                    text: backend.look.font
                }
                Label { text: qsTr("Dim after (s)") }
                SpinBox { id: lookDim; from: 0; to: 3600; stepSize: 10; editable: true; value: backend.look.idleDim }
                Button {
                    text: qsTr("Save look")
                    objectName: "saveLook"
                    onClicked: backend.setLook(escLabel.text, escIcon.text, lookFont.text, lookDim.value)
                }
                Connections {
                    target: backend
                    function onChanged() {
                        escLabel.text = backend.look.escLabel
                        escIcon.text = backend.look.escIcon
                        lookFont.text = backend.look.font
                        lookDim.value = backend.look.idleDim
                        customColor.text = backend.themeStyle.charAt(0) === "#" ? backend.themeStyle : ""
                    }
                }
            }
            RowLayout {
                Label { text: qsTr("Screensaver after (s)") }
                SpinBox { id: saverSeconds; from: 0; to: 3600; stepSize: 30; editable: true; value: backend.saver.seconds }
                ComboBox {
                    id: saverStyle
                    readonly property var styles: ["matrix", "text", "image", "scanner", "waves", "stars",
                                                   "equalizer", "plasma", "pong"]
                    model: [qsTr("Running characters"), qsTr("Floating text"), qsTr("Floating image"),
                            qsTr("Scanner"), qsTr("Waves"), qsTr("Stars"), qsTr("Equalizer"),
                            qsTr("Plasma"), qsTr("Pong")]
                    currentIndex: Math.max(0, styles.indexOf(backend.saver.style))
                }
                TextField {
                    id: saverText
                    visible: saverStyle.currentIndex === 1
                    Layout.fillWidth: true
                    maximumLength: 32
                    placeholderText: "OMARCHY"
                    text: backend.saver.text
                }
                TextField {
                    id: saverImage
                    visible: saverStyle.currentIndex === 2
                    Layout.fillWidth: true
                    maximumLength: 512
                    placeholderText: qsTr("Path to a .png image")
                    text: backend.saver.image
                }
                Item { visible: saverStyle.currentIndex !== 1 && saverStyle.currentIndex !== 2; Layout.fillWidth: true }
            }
            RowLayout {
                Label { text: qsTr("Key corners") }
                SpinBox { id: keyRadius; from: 0; to: 26; stepSize: 2; editable: true; value: backend.keys.radius }
                Label { text: qsTr("Key opacity (%)") }
                SpinBox { id: keyOpacity; from: 10; to: 100; stepSize: 5; editable: true; value: backend.keys.opacity }
                Label { text: qsTr("Outline") }
                SpinBox { id: keyBorder; from: 0; to: 3; editable: true; value: backend.keys.border }
                Label { text: qsTr("Shadow") }
                SpinBox { id: keyShadow; from: 0; to: 4; editable: true; value: backend.keys.shadow }
            }
            RowLayout {
                Label { text: qsTr("Animation behind keys") }
                ComboBox {
                    id: keyAnimation
                    Layout.fillWidth: true
                    readonly property var styles: ["", "matrix", "scanner", "waves", "stars", "equalizer", "plasma", "pong"]
                    model: [qsTr("None"), qsTr("Running characters"), qsTr("Scanner"), qsTr("Waves"), qsTr("Stars"),
                            qsTr("Equalizer"), qsTr("Plasma"), qsTr("Pong")]
                    currentIndex: Math.max(0, styles.indexOf(backend.keys.animation))
                }
                Button {
                    text: qsTr("Save")
                    objectName: "saveKeys"
                    onClicked: backend.setKeys(keyRadius.value, keyOpacity.value,
                                               keyAnimation.styles[keyAnimation.currentIndex],
                                               keyBorder.value, keyShadow.value, false)
                }
                Button {
                    text: qsTr("Use for all layouts")
                    onClicked: backend.setKeys(keyRadius.value, keyOpacity.value,
                                               keyAnimation.styles[keyAnimation.currentIndex],
                                               keyBorder.value, keyShadow.value, true)
                }
                Connections {
                    target: backend
                    function onChanged() {
                        keyRadius.value = backend.keys.radius
                        keyOpacity.value = backend.keys.opacity
                        keyBorder.value = backend.keys.border
                        keyShadow.value = backend.keys.shadow
                        keyAnimation.currentIndex = Math.max(0, keyAnimation.styles.indexOf(backend.keys.animation))
                    }
                }
            }
            RowLayout {
                Label { text: qsTr("Bar background") }
                TextField {
                    id: backgroundImage
                    Layout.fillWidth: true
                    maximumLength: 512
                    placeholderText: qsTr("Path to a .png image")
                    text: backend.saver.background
                }
                Button {
                    text: qsTr("Save")
                    objectName: "saveSaver"
                    onClicked: backend.setSaver(saverSeconds.value, saverStyle.styles[saverStyle.currentIndex],
                                                saverText.text, saverImage.text, backgroundImage.text, false)
                }
                Button {
                    text: qsTr("Use for all layouts")
                    onClicked: backend.setSaver(saverSeconds.value, saverStyle.styles[saverStyle.currentIndex],
                                                saverText.text, saverImage.text, backgroundImage.text, true)
                }
                Connections {
                    target: backend
                    function onChanged() {
                        saverSeconds.value = backend.saver.seconds
                        saverStyle.currentIndex = Math.max(0, saverStyle.styles.indexOf(backend.saver.style))
                        saverText.text = backend.saver.text
                        saverImage.text = backend.saver.image
                        backgroundImage.text = backend.saver.background
                    }
                }
            }
            Label {
                text: qsTr("Rounded keys, screensaver and backgrounds need a renderer that supports them; the stock omarchy-glance ignores them.")
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                color: "#a0a0a0"
            }
            RowLayout {
                Label { text: qsTr("Show for apps") }
                TextField {
                    id: appsInput
                    objectName: "appsInput"
                    Layout.fillWidth: true
                    text: backend.apps
                    placeholderText: qsTr("Window classes, e.g. firefox, code")
                    onAccepted: backend.setApps(text)
                }
                Button { text: qsTr("Save apps"); onClicked: backend.setApps(appsInput.text) }
                Connections {
                    target: backend
                    function onChanged() { appsInput.text = backend.apps }
                }
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
                Layout.preferredHeight: 220
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
                        Label {
                            text: modelData.kind === "scene-next" ? qsTr("Next layout")
                                : modelData.kind === "scene" ? qsTr("Layout: %1").arg(modelData.command)
                                : modelData.command
                            Layout.fillWidth: true
                            elide: Text.ElideMiddle
                        }
                        Label {
                            text: modelData.section === "center" ? qsTr("Center")
                                : modelData.section === "right" ? qsTr("Right") : qsTr("Left")
                        }
                        ToolButton { text: qsTr("Edit"); onClicked: itemSettings.openItem(index, modelData) }
                        ToolButton { text: "↑"; enabled: index > 0; onClicked: backend.moveButton(index, index - 1) }
                        ToolButton { text: "↓"; enabled: index + 1 < list.count; onClicked: backend.moveButton(index, index + 1) }
                        ToolButton { text: "×"; onClicked: backend.removeButton(index) }
                    }
                }
            }
            RowLayout {
                ComboBox {
                    id: template
                    Layout.preferredWidth: 160
                    textRole: "name"
                    displayText: qsTr("Templates")
                    model: [
                        { name: qsTr("Keyboard shortcuts"), label: "Keys", command: "omarchy menu keybindings" },
                        { name: qsTr("Omarchy menu"), label: "Menu", command: "omarchy menu toggle" },
                        { name: qsTr("Change theme"), label: "Theme", command: "omarchy menu toggle appearance.theme" },
                        { name: qsTr("AI agent"), label: "Agent", command: "omarchy agent" },
                        { name: qsTr("Lock screen"), label: "Lock", command: "omarchy system lock" },
                        { name: qsTr("Screenshot"), label: "Shot", command: "omarchy-capture-screenshot" },
                        { name: qsTr("Terminal"), label: "Term", command: "omarchy-launch-terminal" }
                    ]
                    onActivated: function(index) {
                        labelInput.text = model[index].label
                        commandInput.text = model[index].command
                    }
                }
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
                Button {
                    text: qsTr("Add spacer")
                    enabled: backend.buttons.length < 12
                    onClicked: backend.addSpacer()
                }
            }
            RowLayout {
                ComboBox {
                    id: controlKind
                    Layout.fillWidth: true
                    model: [qsTr("Volume (mute, −, +)"), qsTr("Display brightness (−, +)"),
                            qsTr("Keyboard backlight (−, +)"), qsTr("Media player"), qsTr("Microphone")]
                }
                Button {
                    text: qsTr("Add control")
                    objectName: "addControl"
                    enabled: backend.buttons.length < 12
                    onClicked: backend.addControl(["volume", "brightness", "keyboard", "media", "mic"][controlKind.currentIndex])
                }
            }
            RowLayout {
                TextField { id: sceneLabel; placeholderText: qsTr("Label"); maximumLength: 16; Layout.preferredWidth: 160 }
                ComboBox {
                    id: sceneTarget
                    Layout.fillWidth: true
                    model: [qsTr("Next layout")].concat(backend.profiles)
                }
                Button {
                    text: qsTr("Add layout switch")
                    enabled: backend.buttons.length < 12
                    onClicked: if (backend.addSceneButton(sceneLabel.text,
                                       sceneTarget.currentIndex === 0 ? "" : sceneTarget.currentText))
                        sceneLabel.clear()
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
                    objectName: "applyButton"
                    enabled: backend.profiles.length > 0
                    highlighted: true
                    onClicked: applyConfirm.open()
                }
            }
        }
    }

    Dialog {
        id: itemSettings
        objectName: "itemSettings"
        anchors.centerIn: parent
        implicitWidth: Math.min(450, root.width - 24)
        title: qsTr("Edit item")
        modal: true
        background: Rectangle { color: root.color }
        standardButtons: Dialog.Ok | Dialog.Cancel
        property int itemIndex: -1
        property bool graph: false
        property bool cpu: false
        property bool iconic: false
        function openItem(index, item) {
            itemIndex = index
            iconic = item.kind === undefined || item.kind === "button" || item.kind === "scene" || item.kind === "scene-next"
            itemIcon.text = item.icon === undefined ? "" : item.icon
            graph = item.kind === "widget"
            cpu = graph && item.command === "cpu"
            itemSection.currentIndex = item.section === "center" ? 1 : item.section === "right" ? 2 : 0
            itemWidth.value = item.width === undefined ? (graph ? 50 : item.kind === "spacer" ? 40 : 150) : item.width
            itemStyle.currentIndex = item.style === "bars" ? 1 : 0
            itemInterval.value = item.interval === undefined ? 1 : item.interval
            itemTemperature.checked = item.temperature === true
            open()
        }
        onAccepted: {
            backend.editItem(itemIndex, ["left", "center", "right"][itemSection.currentIndex],
                             itemWidth.value, ["dots", "bars"][itemStyle.currentIndex],
                             itemInterval.value, itemTemperature.checked)
            if (iconic) backend.setItemIcon(itemIndex, itemIcon.text)
        }
        ScrollView {
            id: settingsScroll
            implicitWidth: Math.min(390, root.width - 80)
            implicitHeight: Math.min(settingsContent.implicitHeight, root.height - 170)
            contentWidth: Math.min(390, root.width - 80)
            contentHeight: settingsContent.implicitHeight
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            ColumnLayout {
                id: settingsContent
                width: Math.min(390, root.width - 80)
                RowLayout {
                    Label { text: qsTr("Position") }
                    ComboBox {
                        id: itemSection
                        Layout.fillWidth: true
                        model: [qsTr("Left"), qsTr("Center"), qsTr("Right")]
                    }
                }
                RowLayout {
                    Label { text: itemSettings.graph ? qsTr("Graph width (px)") : qsTr("Width (px)") }
                    SpinBox { id: itemWidth; from: 10; to: 500; editable: true }
                }
                RowLayout {
                    visible: itemSettings.graph
                    Label { text: qsTr("Graph style") }
                    ComboBox { id: itemStyle; model: [qsTr("Dots"), qsTr("Bars")] }
                }
                RowLayout {
                    visible: itemSettings.graph
                    Label { text: qsTr("Update interval (s)") }
                    SpinBox { id: itemInterval; from: 1; to: 60; editable: true }
                }
                CheckBox { id: itemTemperature; visible: itemSettings.cpu; text: qsTr("Show CPU temperature") }
                RowLayout {
                    visible: itemSettings.iconic
                    Label { text: qsTr("Icon instead of text") }
                    TextField {
                        id: itemIcon
                        Layout.fillWidth: true
                        maximumLength: 4
                        placeholderText: qsTr("One Nerd Font glyph")
                        font.family: "JetBrainsMono Nerd Font"
                    }
                    ToolButton { text: "…"; onClicked: iconPicker.pick(itemIcon) }
                }
            }
        }
    }
    Popup {
        id: iconPicker
        objectName: "iconPicker"
        anchors.centerIn: Overlay.overlay
        modal: true
        padding: 12
        background: Rectangle { color: "#202020"; border.color: "#555555"; radius: 6 }
        property var target: null
        function pick(field) { target = field; open() }
        ColumnLayout {
            Label { text: qsTr("Choose an icon"); font.bold: true }
            GridLayout {
                columns: 12
                rowSpacing: 2
                columnSpacing: 2
                Repeater {
                    model: ["\uf303", "\uf17c", "\uf120", "\uf121", "\uf023", "\uf09c", "\uf011", "\uf015", "\uf002", "\uf013", "\uf0c9", "\uf07b", "\uf0ac", "\uf269", "\uf268", "\uf09b", "\uf0e0", "\uf1d8", "\uf0f3", "\uf005", "\uf004", "\uf0e7", "\uf06e", "\uf02d", "\uf001", "\uf04b", "\uf04c", "\uf04d", "\uf048", "\uf051", "\uf028", "\uf027", "\uf026", "\uf130", "\uf030", "\uf03e", "\uf008", "\uf185", "\uf186", "\uf108", "\uf109", "\uf11c", "\uf1eb", "\uf240", "\uf019", "\uf093", "\uf0c5", "\uf0c4", "\uf0ea", "\uf044", "\uf1f8", "\uf021", "\uf0ec", "\uf060", "\uf061", "\uf062", "\uf063", "\uf067", "\uf068", "\uf00c", "\uf00d", "\uf128"]
                    ToolButton {
                        required property string modelData
                        text: modelData
                        font.family: "JetBrainsMono Nerd Font"
                        font.pixelSize: 22
                        implicitWidth: 42
                        implicitHeight: 42
                        onClicked: { iconPicker.target.text = modelData; iconPicker.close() }
                    }
                }
            }
            RowLayout {
                Label {
                    text: qsTr("More glyphs: copy one from nerdfonts.com/cheat-sheet into the field.")
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                    Layout.maximumWidth: 420
                }
                Button { text: qsTr("No icon"); onClicked: { iconPicker.target.text = ""; iconPicker.close() } }
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
        Label { text: qsTr("This removes the selected saved layout."); wrapMode: Text.Wrap; width: Math.min(350, root.width - 80) }
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
            width: Math.min(390, root.width - 80)
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
            width: Math.min(390, root.width - 80)
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
