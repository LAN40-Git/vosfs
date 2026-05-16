import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    id: filePage

    Connections {
        target: SignalBrige

        function onListDirFinished(path, dir_entries) {
            pathField.text = path
        }

        function onMakeDirFinished() {
            newFolderPopup.close()
        }
    }

    // 空白区域菜单
    Menu {
        id: emptyAreaMenu

        function isValidFolderName() {
            if (folderNameInput.text === "") {
                folderNameErrorText.text = ""
                return false
            }
            if (folderNameInput.text.includes("/")) {
                folderNameErrorText.text = "文件夹名称中不能包含'/'"
                return false
            }
            return true
        }

        function createFolder() {
            VosfsClient.make_dir(mainWindow.currentDir, folderNameInput.text)
        }

        Popup {
            id: newFolderPopup
            modal: true
            anchors.centerIn: parent
            width: 400
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

            background: Rectangle {
                radius: 8
                color: "#232222"
            }

            onOpened: {
                folderNameInput.text = ""
                folderNameInput.forceActiveFocus()
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "新建文件夹"
                    color: "white"
                    font.bold: true
                    font.pixelSize: 16
                }

                TextField {
                    id: folderNameInput
                    Layout.fillWidth: true
                    font.pixelSize: 16
                    font.bold: true
                    placeholderText: "文件夹名称"
                    background: Rectangle {
                        color: "#2A2A2A"
                        radius: 5
                    }

                    onAccepted: {
                        emptyAreaMenu.createFolder()
                    }
                }

                RowLayout {
                    spacing: 15
                    Layout.alignment: Qt.AlignRight

                    Text {
                        id: folderNameErrorText
                        color: "#FF4444"
                        font.pixelSize: 14
                        visible: !emptyAreaMenu.isValidFolderName()
                    }

                    Button {
                        text: "取消"

                        background: Rectangle {
                            radius: 8
                            color: parent.pressed ? "#5A5A5A" :
                                parent.hovered ? "#4A4A4A" : "transparent"
                        }
                        onClicked: newFolderPopup.close()
                    }

                    Button {
                        text: "创建"
                        enabled: emptyAreaMenu.isValidFolderName()
                        background: Rectangle {
                            radius: 5
                            color: parent.enabled && parent.pressed ? "#5A5A5A" :
                                    parent.enabled && parent.hovered ? "#4A4A4A" : "transparent"
                        }
                        onClicked: {
                            emptyAreaMenu.createFolder()
                        }
                    }
                }
            }
        }

        // 文件选择弹窗
        FileDialog {
            id: uploadFileDialog
            title: "选择要上传的文件"

            onAccepted: {
                var filePath = selectedFile.toString().replace(/^file:\/\//, "")
                VosfsClient.prepare_upload_file(filePath, mainWindow.currentDir);
            }
        }

        MenuItem {
            text: "新建文件夹"
            onTriggered: {
                newFolderPopup.open()
            }
        }

        MenuItem {
            text: "上传"
            onTriggered: {
                uploadFileDialog.open()
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onClicked: (mouse)=>{
            if (mouse.button === Qt.RightButton && pathField.text !== "") {
                emptyAreaMenu.popup()
            }
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
            enabled: mainWindow.canGoBack
            onClicked: mainWindow.backDir()

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
            enabled: mainWindow.canGoForward
            onClicked: mainWindow.forwardDir()

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
                radius: 47
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
        currentIndex: -1

        // 文件/文件夹菜单
        Menu {
            id: elementAreaMenu

            MenuItem {
                text: "打开"
                onTriggered: {
                    var ItemData = fileListView.model.get(fileListView.currentIndex)
                    if (ItemData.is_dir) {
                        goToDir(ItemData.path)
                    }
                }
            }

            MenuItem {
                text: "下载"
                onTriggered: {
                    var ItemData = fileListView.model.get(fileListView.currentIndex)
                    if (!ItemData.is_dir) {
                        VosfsClient.prepare_download_file(ItemData.name, ItemData.path);
                    }
                }
            }

            MenuItem {
                text: "删除"
                onTriggered: {

                }
            }

            MenuItem {
                text: "属性"
                onTriggered: {

                }
            }
        }

        delegate: Item {
            width: fileListView.width
            height: 32

            property bool isHovered: false
            property bool isSelected: fileListView.currentIndex === index

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

                onDoubleClicked: (mouse)=>{
                    if (is_dir && mouse.button === Qt.LeftButton) {
                        mainWindow.goToDir(path)
                    }
                }

                onClicked: (mouse)=>{
                    fileListView.currentIndex = index
                    if (mouse.button === Qt.RightButton) {
                        elementAreaMenu.popup()
                    }
                }
            }

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