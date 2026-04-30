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

    Connections {
        target: SignalBrige
        function onRegisterFinished(success, msg) {
            if (!success) {
                passwordErrorText.visible = true
                passwordErrorText.text = msg
            } else {
                user_name.focus = false
                password.focus = false
                userNameErrorText.visible = false
                passwordErrorText.visible = false
                user_name.clear()
                password.clear()
                loginPopupStack.replace("LoginPage.qml")
            }
            mainWindow.isRegistering = false
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 20

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

            onEditingFinished: {
                const username = user_name.text.trim()
                if (username === "") {
                    userNameErrorText.visible = true
                    userNameErrorText.text = "用户名不能为空"
                } else {
                    userNameErrorText.visible = false
                }
            }
        }

        Text {
            id: userNameErrorText
            anchors.leftMargin: 8
            color: "#FF4444"
            font.pixelSize: 13
            visible: false
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

            onEditingFinished: {
                const username = password.text.trim()
                if (username === "") {
                    passwordErrorText.visible = true
                    passwordErrorText.text = "密码不能为空"
                } else {
                    passwordErrorText.visible = false
                }
            }

            onAccepted: {
                registerBtn.clicked()
            }
        }

        Text {
            id: passwordErrorText
            anchors.leftMargin: 8
            color: "#FF4444"
            font.pixelSize: 13
            visible: false
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
                if (mainWindow.isRegistering ||
                    user_name.text === "" ||
                    password.text === "") {
                    return
                }
                mainWindow.isRegistering = true
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