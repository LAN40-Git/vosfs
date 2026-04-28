import QtQuick
import QtQuick.Controls

Item {
    Column {
        spacing: 8

        TextField {
            id: user_name
            width: 300;
            placeholderText: "用户名"
        }

        TextField {
            id: password
            width: 300;
            placeholderText: "密码";
            echoMode: TextInput.Password
        }

        Button {
            text: "注册";
            width: 300;
            onClicked: {
                AuthClient.register_user(user_name.text, password.text, 1)
            }
        }

        Button {
            text: "返回登录"
            width: 300
            onClicked: stack.pop()
        }
    }
}