import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Effects

Window {
    id: mainWindow
    width: 1200
    height: 900
    visible: true
    title: "vosfs"
    flags: Qt.FramelessWindowHint
    color: "#1E1E1E"

    Connections {
        target: SignalBrige

        function onRegisterFinished(success, msg) {
            console.log("注册结果：", success, msg)
        }

        function onDeleteFinshed(success, msg) {

        }

        function onLoginFinshed(success, msg) {

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
        border.width: 1

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
        property int bottomMargin: 10
        property int btnWidth: 196
        property int btnHeight: 40
        property int btnRadius: 8

        id: leftBar
        width: 224
        height: parent.height - titleBar.height
        anchors.left: parent.left
        anchors.top: titleBar.bottom
        color: "transparent"
        border.color: "#373737"
        border.width: 1

        // 文件
        Button {
            id: fileBtn
            width: parent.btnWidth
            height: parent.btnHeight
            anchors.top: parent.top
            anchors.topMargin: parent.topMargin
            anchors.horizontalCenter: parent.horizontalCenter

            Image {
                id: fileBtnImage
                source: "qrc:/images/file.png"
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
                color: fileBtn.pressed ? "#5A5A5A" :
                    fileBtn.hovered ? "#4A4A4A" : "transparent"
            }
        }

        Button {
            id: userBtn
            width: parent.btnWidth
            height: parent.btnHeight
            anchors.bottom: parent.bottom
            anchors.bottomMargin: parent.bottomMargin
            anchors.horizontalCenter: parent.horizontalCenter

            Text {
                id: userBtnText
                text: "登录"
                color: "black"
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }

            background: Rectangle {
                radius: leftBar.btnRadius
                color: userBtn.pressed ? "#d5d5d5" :
                    userBtn.hovered ? "#e5e5e5" : "#FFFFFF"
            }

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

            onClicked: {
                loginPopup.open();
            }
        }
    }
}