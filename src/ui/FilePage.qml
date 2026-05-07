import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: filePage
    anchors.fill: parent

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
        model: mainWindow.fileModel

        delegate: Item {
            width: parent.width
            height: 32

            Rectangle {
                anchors.fill: parent
                color: mouse.containsMouse ? "#444" : "transparent"
                radius: 4
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10

                Image {
                    width: 20; height: 20
                    source: is_dir ? "qrc:/images/folder.png" : "qrc:/images/file.png"
                }

                Text {
                    text: name; color: "white"; font.pixelSize: 14
                    Layout.preferredWidth: 270
                }

                Text {
                    text: new Date(ctime * 1000).toLocaleString()
                    color: "#aaa"; font.pixelSize: 13
                    Layout.preferredWidth: 200
                }

                Text {
                    text: new Date(mtime * 1000).toLocaleString()
                    color: "#aaa"; font.pixelSize: 13
                    Layout.fillWidth: true
                }
            }
    }
}