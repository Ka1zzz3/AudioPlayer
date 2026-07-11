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

            Rectangle {
                width: parent.width - loadButton.width - refreshButton.width - parent.spacing * 2
                height: 34
                border.color: "#808080"
                color: "transparent"

                TextInput {
                    id: storagePathInput
                    anchors.fill: parent
                    anchors.margins: 6
                    clip: true
                    text: "library.json"
                    verticalAlignment: TextInput.AlignVCenter
                }
            }

            Rectangle {
                id: loadButton
                width: 90
                height: 34
                border.color: "#808080"
                color: loadMouseArea.pressed ? "#dddddd" : "#eeeeee"

                Text {
                    anchors.centerIn: parent
                    text: qsTr("Load")
                }

                MouseArea {
                    id: loadMouseArea
                    anchors.fill: parent
                    onClicked: library.load()
                }
            }

            Rectangle {
                id: refreshButton
                width: 90
                height: 34
                border.color: "#808080"
                color: refreshMouseArea.pressed ? "#dddddd" : "#eeeeee"

                Text {
                    anchors.centerIn: parent
                    text: qsTr("Refresh")
                }

                MouseArea {
                    id: refreshMouseArea
                    anchors.fill: parent
                    onClicked: library.refresh()
                }
            }
        }

        Text {
            width: parent.width
            text: library.lastError.length > 0
                  ? qsTr("Load failed: %1").arg(library.lastError)
                  : qsTr("%n song(s)", "", library.count)
            color: library.lastError.length > 0 ? "#a00000" : "#404040"
            elide: Text.ElideRight
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

                width: ListView.view.width
                height: 52
                color: "white"
                border.color: "#eeeeee"

                Row {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 12

                    Column {
                        width: parent.width - durationText.width - parent.spacing
                        anchors.verticalCenter: parent.verticalCenter

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
                text: qsTr("Load a library file to show songs.")
                color: "#606060"
            }
        }
    }
}
