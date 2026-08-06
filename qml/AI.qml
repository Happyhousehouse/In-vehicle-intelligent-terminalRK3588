import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: root

    anchors.fill: parent
    color: "transparent"

    /*
     * 让根界面可以接收焦点。
     * 点击空白区域时，将焦点从输入框移开。
     */
    focus: true

    /*
     * 点击 Column 外面的空白区域时隐藏键盘。
     *
     * MouseArea 写在 Column 前面，因此位于 Column 下层，
     * 不会阻挡 TextField 和 Button 的点击。
     */
    MouseArea {
        anchors.fill: parent

        onClicked: {
            console.log("[AI.qml] 点击空白区域，隐藏键盘")

            root.forceActiveFocus()
            onboardController.hideKeyboard()
        }
    }

    Column {
        id: contentColumn

        anchors.centerIn: parent

        spacing: 20

        TextField {
            id: inputBox

            width: 500
            height: 60

            placeholderText: "请输入问题"

            font.pixelSize: 24

            selectByMouse: true

            /*
             * 输入框真正获得焦点时显示键盘。
             */
            onActiveFocusChanged: {
                console.log(
                    "[AI.qml] inputBox activeFocus:",
                    activeFocus
                )

                if (activeFocus) {
                    onboardController.showKeyboard()
                }
            }

            /*
             * 对触摸屏点击再做一次主动处理。
             *
             * TapHandler 不会像覆盖式 MouseArea 那样
             * 阻止 TextField 自身接收输入。
             */
            TapHandler {
                onTapped: {
                    console.log(
                        "[AI.qml] TextField 被点击"
                    )

                    inputBox.forceActiveFocus()
                    onboardController.showKeyboard()
                }
            }

            /*
             * 在键盘上按回车时发送消息。
             */
            onAccepted: {
                sendButton.sendMessage()
            }
        }

        /*
         * 临时测试按钮。
         *
         * 用于验证：
         * QML 是否能够调用 onboardController.showKeyboard()。
         *
         * 测试正常以后可以删除这个按钮。
         */
        Button {
            id: keyboardTestButton

            width: 200
            height: 60

            text: "测试键盘"

            font.pixelSize: 24

            onClicked: {
                console.log(
                    "[AI.qml] 测试键盘按钮被点击"
                )

                onboardController.showKeyboard()
            }
        }

        Button {
            id: sendButton

            width: 200
            height: 60

            text: "发送"

            font.pixelSize: 24

            /*
             * 统一发送函数。
             *
             * 点击发送按钮和输入框回车都会调用这里。
             */
            function sendMessage() {
                var question = inputBox.text.trim()

                if (question.length === 0) {
                    console.log(
                        "[AI.qml] 输入内容为空，不发送"
                    )

                    return
                }

                console.log(
                    "[AI.qml] 准备发送问题:",
                    question
                )

                /*
                 * 必须先隐藏键盘，再调用本地模型。
                 *
                 * 当前 vehicleAssistant.chat() 是同步调用，
                 * 模型运行期间可能阻塞 QML 主线程。
                 */
                onboardController.hideKeyboard()

                root.forceActiveFocus()

                answer.text =
                    vehicleAssistant.chat(
                        question
                    )
            }

            onClicked: {
                sendMessage()
            }
        }

        Rectangle {
            width: 600
            height: 200

            color: "#333333"

            radius: 15

            Text {
                id: answer

                anchors.fill: parent
                anchors.margins: 20

                wrapMode: Text.WordWrap

                color: "white"

                font.pixelSize: 22

                text: "AI回复显示区域"
            }
        }
    }

    /*
     * 离开 AI 页面时隐藏 Onboard。
     */
    onVisibleChanged: {
        if (!visible) {
            console.log(
                "[AI.qml] AI 页面不可见，隐藏键盘"
            )

            onboardController.hideKeyboard()
        }
    }

    /*
     * AI.qml 被销毁时隐藏键盘。
     */
    Component.onDestruction: {
        onboardController.hideKeyboard()
    }
}
