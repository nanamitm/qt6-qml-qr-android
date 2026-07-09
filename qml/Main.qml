import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import QrApp

ApplicationWindow {
    id: root
    width: 420
    height: 820
    visible: true
    title: "Qt QR Tools"
    color: "#0f1720"

    QrScanner {
        id: scanner
        duplicateCooldownMs: 1800
    }

    QrGenerator {
        id: generator
    }

    Component.onDestruction: scanner.stop()

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#122033" }
            GradientStop { position: 0.55; color: "#0b1118" }
            GradientStop { position: 1.0; color: "#05070b" }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 14

        Label {
            text: "QR Tools"
            color: "white"
            font.pixelSize: 30
            font.bold: true
            Layout.fillWidth: true
        }

        TabBar {
            id: modeTabs
            Layout.fillWidth: true

            TabButton {
                text: "Scan"
            }

            TabButton {
                text: "Create"
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: modeTabs.currentIndex

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 14

                    Label {
                        text: scanner.statusText
                        color: "#c8d4e5"
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Rectangle {
                            radius: 999
                            color: scanner.running ? "#123f2b" : "#3b1f24"
                            border.color: scanner.running ? "#4ade80" : "#fb7185"
                            border.width: 1
                            implicitHeight: 34
                            implicitWidth: statusRow.implicitWidth + 20

                            Row {
                                id: statusRow
                                anchors.centerIn: parent
                                spacing: 8

                                Rectangle {
                                    width: 10
                                    height: 10
                                    radius: 5
                                    color: scanner.running ? "#4ade80" : "#fb7185"
                                }

                                Label {
                                    text: scanner.running ? "Live camera" : "Stopped"
                                    color: "white"
                                }
                            }
                        }

                        Item { Layout.fillWidth: true }

                        Label {
                            text: "Scans: " + scanner.scanCount
                            color: "#8bb6ff"
                            font.pixelSize: 14
                            font.bold: true
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Rectangle {
                            anchors.fill: parent
                            radius: 24
                            color: "#091018"
                            border.color: "#2a7fff"
                            border.width: 1
                            clip: true

                            VideoOutput {
                                id: preview
                                anchors.fill: parent
                                fillMode: VideoOutput.PreserveAspectCrop
                                Component.onCompleted: {
                                    scanner.videoOutput = preview
                                    scanner.start()
                                }
                            }

                            Rectangle {
                                width: Math.min(parent.width, parent.height) * 0.62
                                height: width
                                anchors.centerIn: parent
                                radius: 18
                                color: "transparent"
                                border.color: scanner.hasResult ? "#fbbf24" : "#4ade80"
                                border.width: 3
                            }

                            Rectangle {
                                width: Math.min(parent.width, parent.height) * 0.62
                                height: 3
                                anchors.horizontalCenter: parent.horizontalCenter
                                y: parent.height * 0.28
                                radius: 2
                                color: "#76ffaf"

                                SequentialAnimation on y {
                                    loops: Animation.Infinite
                                    running: scanner.running
                                    NumberAnimation { from: parent.height * 0.28; to: parent.height * 0.68; duration: 1700; easing.type: Easing.InOutQuad }
                                    NumberAnimation { from: parent.height * 0.68; to: parent.height * 0.28; duration: 1700; easing.type: Easing.InOutQuad }
                                }
                            }

                            Rectangle {
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.bottom: parent.bottom
                                anchors.bottomMargin: 18
                                radius: 999
                                color: scanner.hasResult ? "#f59e0b" : "#122033"
                                border.color: scanner.hasResult ? "#fde68a" : "#2a7fff"
                                border.width: 1
                                visible: scanner.running
                                implicitWidth: statusHint.implicitWidth + 24
                                implicitHeight: statusHint.implicitHeight + 14

                                Label {
                                    id: statusHint
                                    anchors.centerIn: parent
                                    text: scanner.hasResult
                                          ? "Detected. Hold another code up to scan again."
                                          : "Center a QR code inside the frame."
                                    color: "white"
                                    font.pixelSize: 13
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        radius: 18
                        color: "#162231"
                        border.color: "#29435c"
                        border.width: 1
                        implicitHeight: resultColumn.implicitHeight + 24

                        ColumnLayout {
                            id: resultColumn
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 8

                            Label {
                                text: "Last Result"
                                color: "#8bb6ff"
                                font.pixelSize: 14
                                font.bold: true
                            }

                            Rectangle {
                                visible: scanner.lastResultType.length > 0
                                radius: 999
                                color: "#10283a"
                                border.color: "#2f7dbd"
                                border.width: 1
                                implicitWidth: resultTypeLabel.implicitWidth + 22
                                implicitHeight: resultTypeLabel.implicitHeight + 10

                                Label {
                                    id: resultTypeLabel
                                    anchors.centerIn: parent
                                    text: scanner.lastResultType
                                    color: "#d4ecff"
                                    font.pixelSize: 12
                                    font.bold: true
                                }
                            }

                            TextArea {
                                text: scanner.lastResult.length > 0 ? scanner.lastResult : "No QR code detected yet."
                                color: "white"
                                readOnly: true
                                wrapMode: TextEdit.WrapAnywhere
                                background: Rectangle {
                                    radius: 12
                                    color: "#0d1520"
                                    border.color: "#213245"
                                }
                                Layout.fillWidth: true
                                Layout.preferredHeight: 120
                            }

                            Label {
                                text: "Duplicate reads are suppressed for " + Math.round(scanner.duplicateCooldownMs / 100) / 10 + " seconds."
                                color: "#97a9bf"
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                                font.pixelSize: 12
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Button {
                            text: scanner.primaryActionLabel.length > 0 ? scanner.primaryActionLabel : "Action"
                            Layout.fillWidth: true
                            enabled: scanner.primaryActionAvailable
                            onClicked: scanner.triggerPrimaryAction()
                        }

                        Button {
                            text: scanner.running ? "Stop" : "Start"
                            Layout.fillWidth: true
                            onClicked: scanner.running ? scanner.stop() : scanner.start()
                        }

                        Button {
                            text: "Clear"
                            Layout.fillWidth: true
                            onClicked: scanner.clearLastResult()
                        }
                    }
                }
            }

            Flickable {
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: width
                contentHeight: createColumn.implicitHeight
                clip: true

                ColumnLayout {
                    id: createColumn
                    width: parent.width
                    spacing: 14

                    Label {
                        text: generator.statusText
                        color: "#c8d4e5"
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        radius: 18
                        color: "#162231"
                        border.color: "#29435c"
                        border.width: 1
                        implicitHeight: inputColumn.implicitHeight + 24

                        ColumnLayout {
                            id: inputColumn
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 10

                            Label {
                                text: "Text To Encode"
                                color: "#8bb6ff"
                                font.pixelSize: 14
                                font.bold: true
                            }

                            TextArea {
                                id: generatorInput
                                color: "white"
                                wrapMode: TextEdit.Wrap
                                placeholderText: "Type any text, URL, contact string, or note."
                                placeholderTextColor: "#6e8096"
                                selectByMouse: true
                                background: Rectangle {
                                    radius: 12
                                    color: "#0d1520"
                                    border.color: "#213245"
                                }
                                Layout.fillWidth: true
                                Layout.preferredHeight: 140
                                onTextChanged: if (generator.inputText !== text) generator.inputText = text
                            }

                            Connections {
                                target: generator

                                function onInputTextChanged() {
                                    if (generatorInput.text !== generator.inputText)
                                        generatorInput.text = generator.inputText
                                }
                            }

                            Label {
                                text: "Arbitrary UTF-8 text is supported. The QR size adjusts automatically."
                                color: "#97a9bf"
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                                font.pixelSize: 12
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        radius: 24
                        color: "#091018"
                        border.color: "#2a7fff"
                        border.width: 1
                        implicitHeight: qrPreviewBox.implicitHeight + 24

                        ColumnLayout {
                            id: qrPreviewBox
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 10

                            Label {
                                text: "Generated QR Code"
                                color: "#8bb6ff"
                                font.pixelSize: 14
                                font.bold: true
                            }

                            Rectangle {
                                Layout.alignment: Qt.AlignHCenter
                                Layout.preferredWidth: Math.min(root.width - 60, 320)
                                Layout.preferredHeight: Layout.preferredWidth
                                radius: 20
                                color: "#ffffff"
                                border.color: "#dbe7f5"
                                border.width: 1

                                Image {
                                    anchors.fill: parent
                                    anchors.margins: 16
                                    fillMode: Image.PreserveAspectFit
                                    smooth: false
                                    source: generator.generatedImageUrl
                                    visible: generator.hasGenerated
                                }

                                Label {
                                    anchors.centerIn: parent
                                    visible: !generator.hasGenerated
                                    text: "No QR code yet"
                                    color: "#60758d"
                                    font.pixelSize: 16
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Button {
                            text: "Generate"
                            Layout.fillWidth: true
                            enabled: generator.canGenerate
                            onClicked: generator.generate()
                        }

                        Button {
                            text: "Clear"
                            Layout.fillWidth: true
                            onClicked: generator.clear()
                        }
                    }
                }
            }
        }
    }
}
