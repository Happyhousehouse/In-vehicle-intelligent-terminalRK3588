import QtQuick 2.15
import QtQuick.Controls 2.15


ApplicationWindow {

    id:mainWindow

    width:2160

    height:1080


    visible:true
    
    Loader {

        id:pageLoader


        anchors.fill:parent


        z:100

    }

    title:"Vehicle AI Assistant"



    property string currentTime:"00:00:00"

    property string previousPage:""

    property int keyboardLiftDistance: 190


    /*
        聊天数据模型

        role:
        User / AI

        text:
        内容

    */

    ListModel {

        id:chatModel

    }






    //========================
    // C++信号连接
    //========================


    Connections {


        target:vehicleAssistant

        function onOpenCameraPage()
        {

            onboardController.hideKeyboard()

            mainWindow.contentItem.forceActiveFocus()

            pageLoader.source =

                "qrc:/qml/Camera.qml"


        }

        function onOpenGalleryPage()
        {


            console.log(
                "AI open gallery"
            )

            onboardController.hideKeyboard()

            mainWindow.contentItem.forceActiveFocus()

            previousPage="Main"



            pageLoader.source =

                "qrc:/qml/Gallery.qml"


        }

        /*
            AI流式token

        */


        function onAnswerToken(token)
        {


            var index =
            chatModel.count-1



            if(chatModel.get(index).role !== "AI")
            {

                return;

            }



            var oldText =
                chatModel.get(index).message;



            chatModel.setProperty(

                index,

                "message",

                oldText + token

            );


            chatList.positionViewAtEnd();


        }



        function onAnswerFinished()
        {

            chatList.positionViewAtEnd()

        }


    }









    //========================
    // 背景
    //========================


    Image {


        anchors.fill:parent


        source:"qrc:/image/backgroundimage.png"



        fillMode:
        Image.PreserveAspectCrop



        enabled:false


    }






    //暗色遮罩


    Rectangle {


        anchors.fill:parent


        color:"#66000A18"


        enabled:false


    }









    //========================
    //顶部状态栏
    //========================


    Rectangle {


        width:parent.width


        height:60



        color:"#33000000"



        Text {


            anchors.left:parent.left


            anchors.leftMargin:40



            anchors.verticalCenter:
            parent.verticalCenter



            text:"◉ Vehicle AI Assistant"



            color:"#00BFFF"



            font.pixelSize:40


        }





        Text {


            anchors.right:parent.right


            anchors.rightMargin:40



            anchors.verticalCenter:
            parent.verticalCenter



            text:
            "CAN ✓     4G ✓     "
            +
            currentTime



            color:"white"



            font.pixelSize:22


        }


    }








    Timer {


        interval:1000


        running:true


        repeat:true



        onTriggered:{


            currentTime =
            Qt.formatDateTime(

                new Date(),

                "hh:mm:ss"

            )


        }

    }









    //========================
    // AI核心窗口
    //========================


    Rectangle {


        width:1300


        height:650



        radius:30



        anchors.centerIn:parent

            /*
     * 每次 Onboard 显示时都主动上移，
     * 不再依赖窗口管理器自动调整。
     */
        anchors.verticalCenterOffset:
            onboardController.keyboardVisible
            ? -mainWindow.keyboardLiftDistance
            : 0

        Behavior on anchors.verticalCenterOffset {
            NumberAnimation {
                duration: 180
                easing.type: Easing.OutCubic
            }
        }

        color:"#66101828"



        border.width:1



        border.color:"#008CFF"






        Column {


            anchors.fill:parent



            anchors.margins:25



            spacing:15






            // AI头像


            Rectangle {


                width:70


                height:70



                radius:35



                anchors.horizontalCenter:
                parent.horizontalCenter



                color:"#0066FF"




                Text {


                    anchors.centerIn:parent



                    text:"AI"



                    color:"white"



                    font.pixelSize:28


                }


            }









            Text {


                anchors.horizontalCenter:
                parent.horizontalCenter



                text:
                "车辆AI助手"



                color:"white"



                font.pixelSize:26


            }









            //========================
            // 聊天窗口
            //========================


            ScrollView {


                width:parent.width


                height:190



                clip:true





                ListView {


                    id:chatList



                    width:parent.width



                    model:chatModel



                    spacing:8





                    delegate: Rectangle {


                        width:chatList.width


                        height:
                        messageText.height+20



                        radius:10



                        color:

                        model.role==="User"

                        ?

                        "#335577FF"

                        :

                        "#22333333"




                        Text {


                            id:messageText


                            width:
                            parent.width-30



                            anchors.centerIn:parent



                            text:

                            model.role
                            +
                            ":\n"
                            +
                            model.message



                            color:"white"


                            font.pixelSize:30


                            wrapMode:
                            Text.WordWrap


                        }


                    }


                }


            }









            //========================
            // 输入框
            //========================


                        /*
            * 输入区域：
            * 左边是输入框，右边是中英文切换按钮。
            */
            Row {
                id: inputRow

                anchors.horizontalCenter:
                    parent.horizontalCenter

                spacing: 15

                Rectangle {
                    width: 680
                    height: 82

                    radius: 30

                    color: "#3300A8FF"

                    border.width: 1
                    border.color: "#3388CCFF"

                    TextField {
                        id: questionInput

                        anchors.fill: parent

                        /*
                        * 不再通过 anchors.margins 压缩上下高度，
                        * 使用左右内边距。
                        */
                        leftPadding: 20
                        rightPadding: 20
                        topPadding: 0
                        bottomPadding: 0

                        placeholderText:
                            "请输入您的问题..."

                        placeholderTextColor:
                            "#A0B8D0"

                        color: "white"

                        font.pixelSize: 32

                        verticalAlignment:
                            TextInput.AlignVCenter

                        selectByMouse: true

                        background: Rectangle {
                            color: "transparent"
                        }

                        /*
                        * 用户明确点击输入框时显示 Onboard。
                        *
                        * 不再使用 onActiveFocusChanged 自动弹出，
                        * 防止 Onboard 隐藏后因焦点恢复而再次弹出。
                        */
                        TapHandler {
                            onTapped: {
                                console.log(
                                    "[Main.qml] questionInput 被点击"
                                )

                                questionInput.forceActiveFocus()

                                onboardController.showKeyboard()
                            }
                        }

                        onAccepted: {
                            sendButton.sendMessage()
                        }
                    }
                }

                /*
                * 中英文切换按钮。
                *
                * 不再依赖 Onboard 模拟 Ctrl+Space。
                */
                Button {
                    id: inputMethodButton

                    width: 110
                    height: 82

                    text: "中/英"

                    font.pixelSize: 24

                    onClicked: {
                        /*
                        * Fcitx5 切换时需要输入框处于焦点状态。
                        */
                        questionInput.forceActiveFocus()

                        onboardController.showKeyboard()

                        onboardController.toggleInputMethod()
                    }
                }
            }







            //========================
            //发送按钮
            //========================


            Button {
                id: sendButton

                width: 160
                height: 55

                anchors.horizontalCenter:
                    parent.horizontalCenter

                text: "发送"

                font.pixelSize: 22

                /*
                * 点击按钮和输入法回车共用同一个发送函数。
                */
                function sendMessage() {
                    var question =
                        questionInput.text.trim()

                    if (question.length === 0) {
                        return
                    }

                    /*
                    * 发送前隐藏键盘，并恢复 AI 面板位置。
                    */
                    onboardController.hideKeyboard()

                    /*
                    * 将焦点从输入框移走。
                    *
                    * 这样下一次点击输入框时，
                    * TapHandler 会重新触发显示逻辑。
                    */
                    mainWindow.contentItem.forceActiveFocus()

                    /*
                    * 清空输入框。
                    */
                    questionInput.text = ""

                    /*
                    * 用户消息。
                    */
                    chatModel.append({
                        "role": "User",
                        "message": question
                    })

                    /*
                    * AI 回复占位。
                    */
                    chatModel.append({
                        "role": "AI",
                        "message": ""
                    })

                    chatList.positionViewAtEnd()

                    /*
                    * 调用 C++。
                    */
                    vehicleAssistant.chat(
                        question
                    )
                }

                onClicked: {
                    sendMessage()
                }
            }



        }


    }









    //========================
    //底部导航
    //========================


    Rectangle {


        width:parent.width


        height:150



        anchors.bottom:parent.bottom



        color:"#55000000"




        Row {


            anchors.centerIn:parent



            spacing:100






            NavItem{


                icon:"AI"


                title:"AI助手"



                onClicked:{
                    onboardController.hideKeyboard()

                    mainWindow.contentItem.forceActiveFocus()

                    pageLoader.source=""

                }

            }







            NavItem{


                icon:"🚗"


                title:"车辆状态"



                onClicked:{

                    onboardController.hideKeyboard()

                    mainWindow.contentItem.forceActiveFocus()

                    console.log("Vehicle")

                }

            }







            NavItem{


                icon:"📷"


                title:"摄像头"


                onClicked:{
                    
                    onboardController.hideKeyboard()

                    mainWindow.contentItem.forceActiveFocus()

                    pageLoader.source =
                        "qrc:/qml/Camera.qml"

                }

            }

            NavItem{


                icon:"🖼"


                title:"相册"



                onClicked:{

                    onboardController.hideKeyboard()

                    mainWindow.contentItem.forceActiveFocus()

                    mainWindow.previousPage = "Main"

                    pageLoader.source =

                        "qrc:/qml/Gallery.qml"



                }


            }





            NavItem{


                icon:"☁"


                title:"云服务"



                onClicked:{

                    onboardController.hideKeyboard()

                    mainWindow.contentItem.forceActiveFocus()

                    console.log("Cloud")

                }

            }







            NavItem{


                icon:"⚙"


                title:"设置"



                onClicked:{

                    onboardController.hideKeyboard()

                    mainWindow.contentItem.forceActiveFocus()

                    console.log("Setting")

                }

            }



        }


    }



}