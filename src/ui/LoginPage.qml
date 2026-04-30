import QtQuick
import QtQuick.Controls 6.3
import QtQuick.Layouts

Item {
    id: loginPage
    property int textFieldWidth: 256
    property int textFieldHeight: 44
    property int textFieldLeftPadding: 10
    property int textFieldFontPixelSize: 16
    property int textFieldRadius: 12
    property int btnWidth: 256
    property int btnHeight: 40

    Connections {
        target: SignalBrige
        function onLoginFinished(success, msg) {
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
                loginPopup.close()
            }
            mainWindow.isLogging = false
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 20

        TextField {
            id: user_name
            width: loginPage.textFieldWidth
            height: loginPage.textFieldHeight
            placeholderText: "输入用户名"
            placeholderTextColor: "#888888"
            color: "#FFFFFF"
            font.pixelSize: loginPage.textFieldFontPixelSize
            font.bold: true
            leftPadding: loginPage.textFieldLeftPadding

            background: Rectangle {
                color: "#2A2A2A"
                radius: loginPage.textFieldRadius
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
            width: loginPage.textFieldWidth
            height: loginPage.textFieldHeight
            placeholderText: "输入密码"
            placeholderTextColor: "#888888"
            color: "#FFFFFF"
            font.pixelSize: loginPage.textFieldFontPixelSize
            font.bold: true
            leftPadding: loginPage.textFieldLeftPadding

            background: Rectangle {
                color: "#2A2A2A"
                radius: loginPage.textFieldRadius
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
                loginBtn.clicked()
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
            id: loginBtn
            width: loginPage.btnWidth
            height: loginPage.btnHeight

            Text {
                id: loginBtnText
                text: "登录"
                color: "black"
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }

            background: Rectangle {
                radius: 8
                color: loginBtn.pressed ? "#d5d5d5" :
                    loginBtn.hovered ? "#e5e5e5" : "#FFFFFF"
            }

            onClicked: {
                if (mainWindow.isLogging ||
                    user_name.text === "" ||
                    password.text === "") {
                    return
                }

                mainWindow.isLogging = true
                AuthClient.login_user_by_name(user_name.text, password.text, 1)
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
            text: "忘记密码"
            color: "#468dde"
            font.pixelSize: parent.textFontSize

            MouseArea {
                anchors.fill: parent
                onClicked: {
                }
            }
        }

        Text {
            text: "注册账号"
            color: "#468dde"
            font.pixelSize: parent.textFontSize

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    loginPopupStack.replace("RegisterPage.qml")
                }
            }
        }
    }
}