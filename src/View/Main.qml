import QtQuick
import QtQuick.Window
import AudioPlayer 1.0

Window {
    id: root
    width: 900
    height: 600
    visible: true
    title: qsTr("AudioPlayer")
    color: "white"

    LibraryViewModel {
        id: library
        storagePath: storagePathInput.text
        scanDirectoryPath: scanDirectoryPathInput.text
    }

    function formatDuration(seconds) {
        if (seconds <= 0) {
            return "--:--"
        }

        const minutes = Math.floor(seconds / 60)
        const remainingSeconds = seconds % 60
        return minutes + ":" + (remainingSeconds < 10 ? "0" : "") + remainingSeconds
    }

    function secondaryText(artist, album) {
        if (artist.length > 0 && album.length > 0) {
            return artist + " · " + album
        }

        return artist.length > 0 ? artist : album
    }

    component LabeledInput: Column {
        property alias text: input.text
        property string label: ""

        spacing: 4

        Text {
            text: parent.label
            color: "#404040"
            font.pixelSize: 12
        }

        Rectangle {
            width: parent.width
            height: 34
            border.color: "#808080"
            color: "transparent"

            TextInput {
                id: input
                anchors.fill: parent
                anchors.margins: 6
                clip: true
                verticalAlignment: TextInput.AlignVCenter
            }
        }
    }

    component ActionButton: Rectangle {
        id: buttonRoot

        signal clicked()

        property string text: ""

        width: 90
        height: 34
        border.color: "#808080"
        color: buttonMouseArea.pressed ? "#dddddd" : "#eeeeee"

        Text {
            anchors.centerIn: parent
            text: buttonRoot.text
        }

        MouseArea {
            id: buttonMouseArea
            anchors.fill: parent
            onClicked: buttonRoot.clicked()
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        Text {
            text: qsTr("Audio Library")
            font.pixelSize: 24
            font.bold: true
        }

        Row {
            width: parent.width
            spacing: 8

            LabeledInput {
                id: storagePathInput
                width: (parent.width - scanButton.width - saveButton.width - loadButton.width - refreshButton.width - parent.spacing * 4) / 2
                label: qsTr("Storage path")
                text: "library.json"
            }

            LabeledInput {
                id: scanDirectoryPathInput
                width: storagePathInput.width
                label: qsTr("Scan directory")
            }

            ActionButton {
                id: scanButton
                anchors.bottom: parent.bottom
                text: qsTr("Scan")
                onClicked: library.scanDirectory()
            }

            ActionButton {
                id: saveButton
                anchors.bottom: parent.bottom
                text: qsTr("Save")
                onClicked: library.save()
            }

            ActionButton {
                id: loadButton
                anchors.bottom: parent.bottom
                text: qsTr("Load")
                onClicked: library.load()
            }

            ActionButton {
                id: refreshButton
                anchors.bottom: parent.bottom
                text: qsTr("Refresh")
                onClicked: library.refresh()
            }
        }

        Column {
            width: parent.width
            spacing: 4

            Text {
                width: parent.width
                text: qsTr("%n song(s)", "", library.count)
                color: "#404040"
                elide: Text.ElideRight
            }

            Text {
                width: parent.width
                visible: library.statusMessage.length > 0
                text: library.statusMessage
                color: "#404040"
                elide: Text.ElideRight
            }

            Text {
                width: parent.width
                visible: library.lastError.length > 0
                text: library.lastError
                color: "#a00000"
                elide: Text.ElideRight
            }

            Repeater {
                model: library.warnings

                Text {
                    width: parent.width
                    text: qsTr("Warning: %1").arg(modelData)
                    color: "#9a6600"
                    elide: Text.ElideRight
                }
            }
        }

        ListView {
            width: parent.width
            height: parent.height - y
            clip: true
            model: library.songs

            delegate: Rectangle {
                required property string displayTitle
                required property string artist
                required property string album
                required property int durationSeconds
                required property string filePath
                required property string extension

                width: ListView.view.width
                height: 74
                color: "white"
                border.color: "#eeeeee"

                Row {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 12

                    Column {
                        width: parent.width - durationText.width - extensionText.width - parent.spacing * 2
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 2

                        Text {
                            width: parent.width
                            text: displayTitle
                            font.pixelSize: 16
                            elide: Text.ElideRight
                        }

                        Text {
                            width: parent.width
                            text: root.secondaryText(artist, album)
                            color: "#606060"
                            elide: Text.ElideRight
                        }

                        Text {
                            width: parent.width
                            text: filePath
                            color: "#808080"
                            font.pixelSize: 11
                            elide: Text.ElideMiddle
                        }
                    }

                    Text {
                        id: extensionText
                        anchors.verticalCenter: parent.verticalCenter
                        text: extension.length > 0 ? extension : qsTr("unknown")
                        color: "#606060"
                    }

                    Text {
                        id: durationText
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.formatDuration(durationSeconds)
                        color: "#606060"
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                visible: library.count === 0 && library.lastError.length === 0
                text: qsTr("Load a library file or scan a directory to show songs.")
                color: "#606060"
            }
        }
    }
}
