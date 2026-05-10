import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: filePage
    anchors.fill: parent

    Connections {
        target: SignalBrige

        function onListDirFinished(path, dir_entries) {
            pathField.text = path
        }
    }

    RowLayout {
        id: toolBar
        width: parent.width - 50
        height: 40
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.topMargin: 10
        spacing: 8

        // 后退按钮
        Button {
            id: backBtn
            text: "←"
            font.bold: true
            font.pixelSize: 16
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
            enabled: mainWindow.backStack.length > 0
            onClicked: mainWindow.goBack()

            background: Rectangle {
                radius: 6
                color: backBtn.pressed && backBtn.enabled ? "#5A5A5A" :
                    backBtn.hovered && backBtn.enabled ? "#4A4A4A" : "transparent"
            }
        }

        // 前进按钮
        Button {
            id: forwardBtn
            text: "→"
            font.bold: true
            font.pixelSize: 16
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
            enabled: mainWindow.forwardStack.length > 0
            onClicked: mainWindow.goForward()

            background: Rectangle {
                radius: 6
                color: forwardBtn.pressed && forwardBtn.enabled ? "#5A5A5A" :
                    forwardBtn.hovered && forwardBtn.enabled ? "#4A4A4A" : "transparent"
            }
        }

        TextField {
            id: pathField
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            readOnly: true
            text: mainWindow.currentDir
            color: "#FFFFFF"
            background: Rectangle {
                color: "#2A2A2A"
                radius: 4
            }
        }
    }

    // 文件列表区域
    ListView {
        id: fileListView
        anchors.top: toolBar.bottom
        anchors.topMargin: 20
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        anchors.bottomMargin: 10
        spacing: 2
        clip: true
        model: mainWindow.fileListModel

        delegate: Item {
            width: ListView.view.width
            height: 32

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10

                Image {
                    source: is_dir ? "qrc:/images/folder.png" : "qrc:/images/file.png"
                    sourceSize.width: 24
                    sourceSize.height: 24
                }

                Text {
                    text: name; color: "white"; font.pixelSize: 15
                    Layout.preferredWidth: 100
                }

                Text {
                    text: mtime
                    color: "#aaa"; font.pixelSize: 15
                    Layout.preferredWidth: 150
                }
            }
        }
    }
}