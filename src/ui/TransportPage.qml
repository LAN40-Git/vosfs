import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    id: transportPage

    Connections {
        target: SignalBrige


    }

    // 传输列表区域
    ListView {
        id: transportListView
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
        model: mainWindow.transportListView
        currentIndex: -1

        delegate: Item {
            width: transportListView.width
            height: 32

            property bool isHovered: false
            property bool isSelected: transportListView.currentIndex === index

            Rectangle {
                anchors.fill: parent
                radius: 5
                color: isSelected ? "#5A5A5A" :
                    isHovered ? "#4A4A4A" : "transparent"
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                hoverEnabled: true
                property var listView: ListView.view

                onContainsMouseChanged: isHovered = containsMouse

                onClicked: (mouse)=>{
                    transportListView.currentIndex = index
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10

                Text {
                    text: name; color: "white"
                    font.pixelSize: 16
                    font.bold: true
                    Layout.preferredWidth: 100
                }

                Text {
                    text: mtime
                    color: "#aaa"; font.pixelSize: 16
                    Layout.preferredWidth: 150
                }
            }
        }
    }
}