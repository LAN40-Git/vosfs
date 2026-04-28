import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window {
    width: 640
    height: 480
    visible: true
    title: "登录系统"

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: LoginPage {}
    }
}