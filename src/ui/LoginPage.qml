import QtQuick
import QtQuick.Controls

Item {
    anchors.fill: parent

    Column {
        anchors.centerIn: parent
        spacing: 20

        TextField {
            id: editUser
            width: 300
            placeholderText: "用户名"
        }

        TextField {
            id: editPwd
            width: 300
            placeholderText: "密码"
            echoMode: TextInput.Password   // ✅ 正确
        }

        Button {
            text: "登录"
            width: 300
            onClicked: {
                AuthClient.login(editUser.text, editPwd.text, 0)
            }
        }

        Button {
            text: "去注册"
            width: 300
            onClicked: {
                stack.push(RegisterPage)   // ✅ 正确
            }
        }
    }
}