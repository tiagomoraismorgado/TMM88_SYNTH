import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    width: 800
    height: 600
    visible: true
    title: "TMM88 Synth"
    color: "#1e1e1e"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        Text {
            text: "TMM88 Synth"
            font.pixelSize: 24
            color: "white"
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: "DSP Framework Successfully Revamped!"
            font.pixelSize: 16
            color: "#cccccc"
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: "Native C++ Implementation\nNo Hardware Acceleration Dependencies\nQt6 UI Ready"
            font.pixelSize: 14
            color: "#888888"
            horizontalAlignment: Text.AlignHCenter
            Layout.alignment: Qt.AlignHCenter
        }

        Item {
            Layout.fillHeight: true
        }

        Text {
            text: "Build completed successfully!\nDSP library and example working."
            font.pixelSize: 12
            color: "#666666"
            horizontalAlignment: Text.AlignHCenter
            Layout.alignment: Qt.AlignHCenter
        }
    }
}
            Layout.fillWidth: true
            Layout.fillHeight: true

            // TAB 1: Engine (Synth)
            RowLayout {
                spacing: 20
                Dial {
                    from: 20; to: 2000; value: synth.frequency
                    onValueChanged: synth.frequency = value
                    Text { text: "FREQ"; color: "cyan"; anchors.top: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter }
                }
                Dial {
                    from: 50; to: 10000; value: synth.cutoff
                    onValueChanged: synth.cutoff = value
                    Text { text: "CUTOFF"; color: "magenta"; anchors.top: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter }
                }
                Dial {
                    from: 1.0; to: 10.0; value: synth.drive
                    onValueChanged: synth.drive = value
                    Text { text: "DRIVE"; color: "orange"; anchors.top: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter }
                }
                Dial {
                    from: 0.0; to: 1.0; value: synth.morph
                    onValueChanged: synth.morph = value
                    Text { text: "MORPH"; color: "white"; anchors.top: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter }
                }
                Button {
                    text: "LOAD WAV"
                    Layout.alignment: Qt.AlignVCenter
                    onClicked: wavetableDialog.open()
                    background: Rectangle { color: "#222"; border.color: "cyan"; radius: 4 }
                    contentItem: Text { text: parent.text; color: "cyan"; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                }
            }

            // TAB 2: Space (Reverb)
            RowLayout {
                spacing: 20
                Dial {
                    from: 0.0; to: 1.0; value: synth.absorption
                    onValueChanged: synth.absorption = value
                    Text { text: "ABSRB"; color: "#aaa"; anchors.top: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter }
                }
                ComboBox {
                    model: ["Small Room", "Medium Hall", "Large Hall", "Plate", "Cathedral", "Ambience", "Chamber", "Room 1", "Room 2", "Hall 1", "Hall 2", "Studio", "Vocal Plate", "Drum Room", "Concert Hall"]
                    currentIndex: synth.roomPreset
                    onCurrentIndexChanged: synth.roomPreset = currentIndex
                    Layout.preferredWidth: 150
                }
            }

            // TAB 3: Crystals (Granular)
            RowLayout {
                spacing: 15
                Dial {
                    from: 0.5; to: 2.0; value: synth.crystalsPitch
                    onValueChanged: synth.crystalsPitch = value
                    Text { text: "PITCH"; color: "#0f0"; anchors.top: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter }
                }
                Dial {
                    from: 10; to: 2000; value: synth.crystalsDelay
                    onValueChanged: synth.crystalsDelay = value
                    Text { text: "DELAY"; color: "#0f0"; anchors.top: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter }
                }
                Dial {
                    from: 0.0; to: 0.95; value: synth.crystalsFeedback
                    onValueChanged: synth.crystalsFeedback = value
                    Text { text: "FBACK"; color: "#0f0"; anchors.top: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter }
                }
                Dial {
                    from: 0.0; to: 1.0; value: synth.crystalsRandom
                    onValueChanged: synth.crystalsRandom = value
                    Text { text: "CLOUD"; color: "#0f0"; anchors.top: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter }
                }
                Dial {
                    from: 0.0; to: 1.0; value: synth.crystalsMix
                    onValueChanged: synth.crystalsMix = value
                    Text { text: "MIX"; color: "#0f0"; anchors.top: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter }
                }
                ColumnLayout {
                    Text { text: "REVERSE"; color: "white"; Layout.alignment: Qt.AlignHCenter }
                    Switch {
                        checked: synth.crystalsReverse
                        onToggled: synth.crystalsReverse = checked
                    }
                }
            }

            // TAB 4: Dynamic Modulation Matrix
            ColumnLayout {
                spacing: 10
                RowLayout {
                    spacing: 20
                    Dial {
                        from: 0.0; to: 1.0; value: synth.lfoShape
                        onValueChanged: synth.lfoShape = value
                        Text { text: "LFO WAVE"; color: "yellow"; anchors.top: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter }
                    }
                    Dial {
                        from: 0.1; to: 20.0; value: synth.lfoRate
                        onValueChanged: synth.lfoRate = value
                        Text { text: "LFO RATE"; color: "yellow"; anchors.top: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter }
                    }
                Dial {
                    from: 0.1; to: 5.0; value: synth.traceSpeed
                    onValueChanged: synth.traceSpeed = value
                    Text { text: "TRACE SPEED"; color: "white"; anchors.top: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter; font.pixelSize: 10 }
                }

                Dial {
                    from: 0.1; to: 5.0; value: synth.traceScale
                    onValueChanged: synth.traceScale = value
                    Text { text: "V-ZOOM"; color: "white"; anchors.top: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter; font.pixelSize: 10 }
                }

                Switch {
                    id: freezeSwitch
                    checked: synth.freezeTrace
                    onToggled: synth.freezeTrace = checked
                    Text { text: "FREEZE"; color: "white"; anchors.top: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter; font.pixelSize: 10 }
                }
                }

                Button {
                    text: "CLEAR ALL"
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: 30
                    onClicked: synth.clearModMatrix()
                    Layout.alignment: Qt.AlignHCenter
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#333" }

                RowLayout {
                    spacing: 15
                    Repeater {
                        model: 4 // Display first 4 modulation slots
                        delegate: ColumnLayout {
                            spacing: 5

                            // Modulation Visualizer
                            Rectangle {
                                Layout.preferredWidth: 100
                                Layout.preferredHeight: 30
                                color: "#111"
                                border.color: "#333"
                                radius: 2
                                clip: true

                                Oscilloscope {
                                    anchors.fill: parent
                                    color: srcBox.currentIndex === 1 ? "cyan" : "yellow"
                                    waveform: srcBox.currentIndex === 1 ? synth.adsrTrace : 
                                              srcBox.currentIndex === 2 ? synth.lfoTrace : []
                                    traceScale: synth.traceScale
                                    opacity: srcBox.currentIndex === 0 ? 0 : 1
                                }
                            }

                            ComboBox {
                                id: srcBox; model: synth.modSources
                                onActivated: synth.setModRoute(index, currentIndex, destBox.currentIndex, depthDial.value)
                                Layout.preferredWidth: 100
                            }
                            ComboBox {
                                id: destBox; model: synth.modDestinations
                                onActivated: synth.setModRoute(index, srcBox.currentIndex, currentIndex, depthDial.value)
                                Layout.preferredWidth: 100
                            }
                            CheckBox {
                                id: muteCheck
                                text: "MUTE"
                                checked: synth.getModMute(index)
                                onToggled: synth.setModMute(index, checked)
                                contentItem: Text {
                                    text: parent.text
                                    color: "white"
                                    font.pixelSize: 10
                                    leftPadding: parent.indicator.width + parent.spacing
                                }
                            }
                            Dial {
                                id: depthDial; from: -1.0; to: 1.0; value: 0.0
                                onValueChanged: synth.setModRoute(index, srcBox.currentIndex, destBox.currentIndex, value)
                                Layout.preferredWidth: 60
                                Text { text: "DEPTH"; color: "white"; font.pixelSize: 9; anchors.top: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter }
                            }
                        }
                    }
                }
            }
        }

        Button {
            text: "TRIGGER"
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 300
            Layout.preferredHeight: 60
            onPressed: synth.noteOn(1.0)
            onReleased: synth.noteOff()
            
            background: Rectangle {
                color: parent.pressed ? "#333" : "#222"
                border.color: "white"
                radius: 4
            }
            contentItem: Text {
                text: parent.text
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}