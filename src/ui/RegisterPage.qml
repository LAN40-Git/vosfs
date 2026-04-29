import QtQuick
import QtQuick.Controls 6.3
import QtQuick.Layouts

Item {
    id: registerPage
    property int textFieldWidth: 256
    property int textFieldHeight: 44
    property int textFieldLeftPadding: 10
    property int textFieldFontPixelSize: 16
    property int textFieldRadius: 12
    property int btnWidth: 256
    property int btnHeight: 40

    Column {
        anchors.centerIn: parent
        spacing: 30

        TextField {
            id: user_name
            width: registerPage.textFieldWidth
            height: registerPage.textFieldHeight
            placeholderText: "输入用户名"
            placeholderTextColor: "#888888"
            color: "#FFFFFF"
            font.pixelSize: registerPage.textFieldFontPixelSize
            font.bold: true
            leftPadding: registerPage.textFieldLeftPadding

            background: Rectangle {
                color: "#2A2A2A"
                radius: registerPage.textFieldRadius
            }
        }

        TextField {
            id: password
            echoMode: TextInput.Password
            width: registerPage.textFieldWidth
            height: registerPage.textFieldHeight
            placeholderText: "输入密码"
            placeholderTextColor: "#888888"
            color: "#FFFFFF"
            font.pixelSize: registerPage.textFieldFontPixelSize
            font.bold: true
            leftPadding: registerPage.textFieldLeftPadding

            background: Rectangle {
                color: "#2A2A2A"
                radius: registerPage.textFieldRadius
            }
        }

        Button {
            id: registerBtn
            width: registerPage.btnWidth
            height: registerPage.btnHeight

            Text {
                id: registerBtnText
                text: "注册"
                color: "black"
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }

            background: Rectangle {
                radius: 8
                color: registerBtn.pressed ? "#d5d5d5" :
                    registerBtn.hovered ? "#e5e5e5" : "#FFFFFF"
            }

            onClicked: {
                AuthClient.register_user(user_name.text, password.text, 1)
            }
        }
    }

    RowLayout {
        property int textFontSize: 16

        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 16

        Text {
            text: "登录"
            color: "#468dde"
            font.pixelSize: parent.textFontSize

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    loginPopupStack.replace("LoginPage.qml")
                }
            }
        }
    }
}