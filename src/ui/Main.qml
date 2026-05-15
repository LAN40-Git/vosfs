import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Effects

Window {
    id: mainWindow
    width: 1600
    height: 1100
    visible: true
    title: "vosfs"
    flags: Qt.FramelessWindowHint
    color: "#26282c"

    property bool isLogging: false
    property bool isRegistering: false
    property bool isLoggedIn: false
    property ListModel fileListModel: ListModel {}
    property ListModel transportListModel: ListModel {}
    property var backStack: []    // 后退栈
    property bool canGoBack: false
    property var forwardStack: [] // 前进栈
    property bool canGoForward: false
    property string currentDir: "/"   // 当前目录
    property string uid
    property string user_name
    property string avatar
    property int role
    property string quota
    property string create_time

    function goToDir(path) {
        if (currentDir !== path) {
            if (currentDir !== "") {
                backStack.push(currentDir)
            }
            forwardStack = []
        }
        VosfsClient.list_dir(path)
        mainWindow.canGoBack = mainWindow.backStack.length > 0;
        mainWindow.canGoForward = mainWindow.forwardStack.length > 0;
    }

    function backDir() {
        forwardStack.push(currentDir)
        VosfsClient.list_dir(mainWindow.backStack[mainWindow.backStack.length - 1])
        backStack.pop()
        mainWindow.canGoBack = mainWindow.backStack.length > 0;
        mainWindow.canGoForward = mainWindow.forwardStack.length > 0;
    }

    function forwardDir() {
        backStack.push(currentDir)
        VosfsClient.list_dir(mainWindow.forwardStack[mainWindow.forwardStack.length - 1])
        forwardStack.pop()
        mainWindow.canGoBack = mainWindow.backStack.length > 0;
        mainWindow.canGoForward = mainWindow.forwardStack.length > 0;
    }

    Connections {
        target: SignalBrige
        function onLoginFinished(success, msg) {
            if (success) {
                mainWindow.uid = VosfsClient.uid
                mainWindow.user_name = VosfsClient.user_name
                mainWindow.avatar = VosfsClient.avatar
                mainWindow.role = VosfsClient.role
                mainWindow.quota = VosfsClient.quota
                mainWindow.create_time = VosfsClient.create_time
                mainWindow.isLoggedIn = true
                fileBtn.click()
            }
        }

        function onListDirFinished(path, dir_entries) {
            fileListModel.clear()
            for(var i = 0; i < dir_entries.length; i++) {
                var item = dir_entries[i]
                mainWindow.fileListModel.append({
                    ino: item.ino,
                    name: item.name,
                    path: item.path,
                    is_dir: item.is_dir,
                    ctime: item.ctime,
                    mtime: item.mtime
                })
            }
            currentDir = path
        }
    }

    // 顶部标题栏
    Rectangle {
        id: titleBar
        antialiasing: true
        width: parent.width
        height: 42
        anchors.top: parent.top
        anchors.right: parent.right
        color: "transparent"
        border.color: "#373737"
        border.width: 2

        MouseArea {
            anchors.fill: parent
            onPressed: mainWindow.startSystemMove()
            Button {
                id: minimizeBtn
                width: 24
                height: 24
                anchors.right: maximizeBtn.left
                anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter

                background: Rectangle {
                    Image {
                        source: "qrc:/images/min.png"
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.horizontalCenter: parent.horizontalCenter
                        sourceSize.width: 12
                        sourceSize.height: 12
                    }

                    radius: 12
                    color: minimizeBtn.pressed ? "#5A5A5A" :
                        minimizeBtn.hovered ? "#4A4A4A" : "#373737"
                }

                onClicked: mainWindow.showMinimized()
            }

            Button {
                id: maximizeBtn
                width: 24
                height: 24
                anchors.right: closeBtn.left
                anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter

                background: Rectangle {
                    Image {
                        source: "qrc:/images/max.png"
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.horizontalCenter: parent.horizontalCenter
                        sourceSize.width: 12
                        sourceSize.height: 12
                    }

                    radius: 12
                    color: maximizeBtn.pressed ? "#5A5A5A" :
                        maximizeBtn.hovered ? "#4A4A4A" : "#373737"
                }

                onClicked: {
                    if(mainWindow.visibility === Window.Maximized) {
                        mainWindow.showNormal();
                    } else {
                        mainWindow.showMaximized();
                    }
                }
            }

            Button {
                id: closeBtn
                width: 24
                height: 24
                anchors.right: parent.right
                anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter

                background: Rectangle {
                    Image {
                        source: "qrc:/images/close.png"
                        sourceSize.width: 12
                        sourceSize.height: 12
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    radius: 12
                    color: closeBtn.pressed ? "#5A5A5A" :
                        closeBtn.hovered ? "#4A4A4A" : "#373737"
                }

                onClicked: Qt.quit()
            }
        }
    }

    // 左侧边栏
    Rectangle {
        property int topMargin: 30
        property int bottomMargin: 15
        property int btnWidth: 196
        property int btnHeight: 40
        property int btnRadius: 8

        id: leftBar
        width: 224
        height: mainWindow.height - titleBar.height - 40
        anchors.left: parent.left
        anchors.top: titleBar.bottom
        anchors.topMargin: 20
        anchors.leftMargin: 10
        radius: 12
        color: "#191a1c"

        ButtonGroup {
            id: leftBarButtonGroup
            exclusive: true
        }

        // 文件
        Button {
            id: fileBtn
            width: parent.btnWidth
            height: parent.btnHeight
            anchors.top: parent.top
            anchors.topMargin: parent.topMargin
            anchors.horizontalCenter: parent.horizontalCenter
            ButtonGroup.group: leftBarButtonGroup
            checkable: true

            Image {
                id: fileBtnImage
                source: "qrc:/images/folder.png"
                sourceSize.width: 24
                sourceSize.height: 24
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                id: fileBtnText
                text: "文件"
                font.family: "Ubuntu Mono"
                antialiasing: true
                color: "#FFFFFF"
                anchors.left: fileBtnImage.right
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
            }

            background: Rectangle {
                radius: leftBar.btnRadius
                color: fileBtn.checked ? "#5A5A5A" :
                    fileBtn.hovered ? "#4A4A4A" : "transparent"
            }

            onClicked: {
                if (mainWindow.isLoggedIn) {
                    bodyStack.replace("FilePage.qml")
                    mainWindow.goToDir(currentDir)
                }
            }
        }

        // 传输
        Button {
            id: transportBtn
            width: parent.btnWidth
            height: parent.btnHeight
            anchors.top: fileBtn.bottom
            anchors.topMargin: 10
            anchors.horizontalCenter: parent.horizontalCenter
            ButtonGroup.group: leftBarButtonGroup
            checkable: true

            Image {
                id: transportBtnImage
                source: "qrc:/images/transport.png"
                sourceSize.width: 24
                sourceSize.height: 24
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                id: transportBtnText
                text: "传输"
                font.family: "Ubuntu Mono"
                antialiasing: true
                color: "#FFFFFF"
                anchors.left: transportBtnImage.right
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
            }

            background: Rectangle {
                radius: leftBar.btnRadius
                color: transportBtn.checked ? "#5A5A5A" :
                    transportBtn.hovered ? "#4A4A4A" : "transparent"
            }

            onClicked: {

            }
        }

        // 登录（用户）按钮
        Button {
            id: userLoginBtn
            width: parent.btnWidth
            height: parent.btnHeight
            anchors.bottom: parent.bottom
            anchors.bottomMargin: parent.bottomMargin
            anchors.horizontalCenter: parent.horizontalCenter
            visible: !mainWindow.isLoggedIn

            Text {
                id: userLoginBtnText
                text: "登录"
                color: "black"
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }

            background: Rectangle {
                radius: leftBar.btnRadius
                color: userLoginBtn.pressed ? "#d5d5d5" :
                    userLoginBtn.hovered ? "#e5e5e5" : "#FFFFFF"
            }

            onClicked: {
                loginPopup.open();
            }
        }

        Button {
            id: userMenuBtn
            width: parent.btnWidth
            height: parent.btnHeight
            anchors.bottom: parent.bottom
            anchors.bottomMargin: parent.bottomMargin
            anchors.horizontalCenter: parent.horizontalCenter
            visible: mainWindow.isLoggedIn

            Image {
                id: userMenuBtnImage
                source: "qrc:/images/user.png"
                sourceSize.width: 24
                sourceSize.height: 24
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: mainWindow.user_name
                color: "white"
                anchors.left: userMenuBtnImage.right
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
            }

            background: Rectangle {
                radius: leftBar.btnRadius
                color: userMenuBtn.pressed ? "#5A5A5A" :
                    userMenuBtn.hovered ? "#4A4A4A" : "transparent"
            }

            onClicked: userMenu.open()
        }
    }

    // 登录弹窗
    Popup {
        id: loginPopup
        modal: true
        focus: true
        width: 360
        height: 480
        anchors.centerIn: Overlay.overlay

        background: Rectangle {
            radius: 12
            color: "#232222"
        }

        StackView {
            id: loginPopupStack
            anchors.fill: parent
            clip: true
            initialItem: LoginPage{}
        }
    }

    // 用户菜单
    Menu {
        id: userMenu
        parent: userMenuBtn
        x: 0
        y: -userMenu.height

        // 个人信息
        MenuItem {
            text: "个人信息"
        }

        // 退出登录
        MenuItem {
            text: "退出登录"
            onClicked: {
                mainWindow.isLoggedIn = false
                // 清空会话
                mainWindow.uid = ""
                mainWindow.user_name = ""
                mainWindow.avatar = ""
                mainWindow.role = 0
                bodyStack.clear()
            }
        }
    }

    // 主体区域
    Rectangle {
        id: bodyField
        width: mainWindow.width - leftBar.width - consoleField.width - 40
        height: leftBar.height
        anchors.left: leftBar.right
        anchors.top: titleBar.bottom
        anchors.leftMargin: 10
        anchors.topMargin: 20
        color: "#191a1c"
        radius: 10

        StackView {
            id: bodyStack
            anchors.fill: parent
            clip: true
            initialItem: {}
        }
    }

    // 控制台区域
    Rectangle {
        id: consoleField
        width: 500
        height: leftBar.height
        anchors.left: bodyField.right
        anchors.top: titleBar.bottom
        anchors.leftMargin: 10
        anchors.topMargin: 20
        color: "#191a1c"
        radius: 10

        TextEdit {
            id: consoleText
            width: parent.width
            height: parent.height - 40
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            color: "#259151"
            readOnly: true
            selectByMouse: true
            font.pixelSize: 16
            font.bold: true
            padding: 20
            wrapMode: TextEdit.Wrap
            clip: true
        }

        Connections {
            target: SignalBrige
            function onAppendLog(msg) {
                if (consoleText.contentHeight > consoleText.height - 60) {
                    consoleText.clear()
                }

                var now = new Date();
                var time = now.toLocaleString(Qt.Local, "yyyy-MM-dd hh:mm:ss")
                consoleText.append(`[${time}]: ${msg}`)
            }
        }
    }
}