import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Window {
    id: root
    visible: true
    width: 960
    height: 700
    title: "Wexi Audio Converter"

    property string inputPath: ""
    property string outputPath: ""

    Component.onCompleted: backend.detectCapabilities()

    function guessDefaultOutput() {
        if (!inputPath || inputPath.length === 0 || !formatBox.currentText) {
            return ""
        }
        let withoutExt = inputPath
        const dot = inputPath.lastIndexOf(".")
        if (dot > 0) {
            withoutExt = inputPath.substring(0, dot)
        }
        return withoutExt + "_converted." + formatBox.currentText
    }

    Page{
        anchors.fill: parent
        background: Rectangle
        {
            color:"white"
        }


    FileDialog {
        id: inputDialog
        title: "Select input audio file"
        fileMode: FileDialog.OpenFile
        onAccepted: {
            root.inputPath = selectedFile.toString()
            if (!root.outputPath || root.outputPath.length === 0) {
                root.outputPath = root.guessDefaultOutput()
            }
        }
    }

    FileDialog {
        id: outputDialog
        title: "Select output file"
        fileMode: FileDialog.SaveFile
        onAccepted: root.outputPath = selectedFile.toString()
    }

    MessageDialog {
        id: resultDialog
        title: "Conversion Result"
    }

    Connections {
        target: backend
        function onConversionFinished(success, message) {
            resultDialog.text = message
            resultDialog.icon = success ? MessageDialog.Information : MessageDialog.Critical
            resultDialog.open()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        Label {
            text: "Convert between many audio formats via ffmpeg."
            font.bold: true
        }

        GroupBox {
            title: "Files"
            Layout.fillWidth: true

            GridLayout {
                columns: 3
                anchors.fill: parent
                columnSpacing: 8
                rowSpacing: 8

                Label { text: "Input"; Layout.alignment: Qt.AlignVCenter }
                TextField {
                    text: root.inputPath
                    placeholderText: "Select input file"
                    readOnly: true
                    Layout.fillWidth: true
                }
                Button { text: "Browse"; onClicked: inputDialog.open() }

                Label { text: "Output"; Layout.alignment: Qt.AlignVCenter }
                TextField {
                    text: root.outputPath
                    placeholderText: "Select output file"
                    readOnly: true
                    Layout.fillWidth: true
                }
                Button { text: "Browse"; onClicked: outputDialog.open() }
            }
        }

        GroupBox {
            title: "Encoder settings"
            Layout.fillWidth: true

            GridLayout {
                columns: 4
                anchors.fill: parent
                columnSpacing: 8
                rowSpacing: 8

                Label { text: "Container format" }
                ComboBox {
                    id: formatBox
                    model: backend.outputFormats
                    Layout.fillWidth: true
                    onCurrentTextChanged: {
                        if (root.inputPath && root.inputPath.length > 0) {
                            root.outputPath = root.guessDefaultOutput()
                        }
                    }
                }

                Label { text: "Audio codec" }
                ComboBox {
                    id: codecBox
                    model: backend.audioCodecs
                    Layout.fillWidth: true
                }

                Label { text: "Bitrate (kbps, 0=auto)" }
                SpinBox {
                    id: bitrateBox
                    from: 0
                    to: 1024
                    value: 192
                    editable: true
                    Layout.fillWidth: true
                }

                Label { text: "Sample rate (Hz, 0=auto)" }
                SpinBox {
                    id: sampleRateBox
                    from: 0
                    to: 384000
                    value: 44100
                    stepSize: 100
                    editable: true
                    Layout.fillWidth: true
                }

                Label { text: "Channels (0=auto)" }
                SpinBox {
                    id: channelsBox
                    from: 0
                    to: 8
                    value: 2
                    editable: true
                    Layout.fillWidth: true
                }

                Label { text: "Extra ffmpeg args" }
                TextField {
                    id: extraArgsField
                    placeholderText: "Example: -af loudnorm -compression_level 12"
                    Layout.columnSpan: 3
                    Layout.fillWidth: true
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Button {
                text: "Refresh capabilities"
                onClicked: backend.detectCapabilities()
                enabled: !backend.busy
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Cancel"
                enabled: backend.busy
                onClicked: backend.cancel()
            }

            Button {
                text: backend.busy ? "Converting..." : "Convert"
                enabled: !backend.busy
                         && root.inputPath.length > 0
                         && root.outputPath.length > 0
                onClicked: backend.convert(root.inputPath,
                                           root.outputPath,
                                           codecBox.currentText,
                                           bitrateBox.value,
                                           sampleRateBox.value,
                                           channelsBox.value,
                                           extraArgsField.text)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: backend.busy || backend.progress > 0

            Label {
                text: "Progress"
            }

            ProgressBar {
                Layout.fillWidth: true
                from: 0
                to: 1
                value: backend.progress
                indeterminate: backend.busy && !backend.progressKnown
            }

            Label {
                text: backend.progressKnown
                      ? Math.round(backend.progress * 100) + "%"
                      : (backend.busy ? "Estimating..." : "")
            }
        }

        GroupBox {
            title: "Log"
            Layout.fillWidth: true
            Layout.fillHeight: true

            ScrollView {
                anchors.fill: parent

                TextArea {
                    text: backend.logText
                    readOnly: true
                    wrapMode: Text.WrapAnywhere
                }
            }
        }
    }
    footer: Rectangle
    {
        height: 30

        Button
        {
            onClicked: aboutPopup.open()
            icon.source: "about.png"
        }
         color:"white"

    }
    Popup {
               width: 300
               height: 300
               anchors.centerIn: parent
               font.pixelSize: 10
               visible: false
               id: aboutPopup
               // This will have a pixelSize of 10 and "steelblue" in color.
               background: Rectangle
               {
                   color:"white"
                   border.color: "black"
               }

               Column
               {
                   anchors.centerIn: parent
                   Image {
                       id: name
                       source: "wexi.jpeg"
                         anchors.horizontalCenter: parent.horizontalCenter
                         width: 80
                         height: 100
                   }
                   Text {

                       text: qsTr("Wexi Audio Converter")
                       font.pointSize: 20
                       anchors.horizontalCenter: parent.horizontalCenter
                   }
                   Text {

                       text: qsTr("By Ronel Tchoulayeu")
                        font.pointSize: 18
                          anchors.horizontalCenter: parent.horizontalCenter
                   }
                   Text {

                       text: qsTr("jordanprog@yahoo.fr")
                        font.pointSize: 16
                          anchors.horizontalCenter: parent.horizontalCenter
                   }
               }

           }
}
}
