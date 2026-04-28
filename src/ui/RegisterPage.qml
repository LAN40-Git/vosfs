import QtQuick
import QtQuick.Controls

Item {
    anchors.fill: parent

    Column {
        anchors.centerIn: parent
        spacing: 20

        TextField { width: 300; placeholderText: "用户名" }
        TextField { width: 300; placeholderText: "密码"; echoMode: TextInput.Password }

        Button { text: "注册"; width: 300 }

        Button {
            text: "返回登录"
            width: 300
            onClicked: stack.pop()
        }
    }
}