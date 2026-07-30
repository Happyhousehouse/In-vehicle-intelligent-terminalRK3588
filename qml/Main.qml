import QtQuick 2.15
import QtQuick.Controls 2.15


ApplicationWindow {


    width:1280

    height:720


    visible:true
    
    Loader {

        id:pageLoader


        anchors.fill:parent


        z:100

    }

    title:"Vehicle AI Assistant"



    property string currentTime:"00:00:00"




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



            font.pixelSize:28


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


        width:850


        height:430



        radius:30



        anchors.centerIn:parent



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


                            font.pixelSize:18


                            wrapMode:
                            Text.WordWrap


                        }


                    }


                }


            }









            //========================
            // 输入框
            //========================


            Rectangle {


                width:600



                height:55



                radius:28



                anchors.horizontalCenter:
                parent.horizontalCenter



                color:"#3300A8FF"





                TextField {


                    id:questionInput



                    anchors.fill:parent



                    anchors.margins:10




                    placeholderText:

                    "请输入您的问题..."




                    placeholderTextColor:
                    "#A0B8D0"



                    color:"white"



                    font.pixelSize:20





                    background:


                    Rectangle{


                        color:"transparent"


                    }


                }


            }








            //========================
            //发送按钮
            //========================


            Button {


                width:160


                height:45



                anchors.horizontalCenter:
                parent.horizontalCenter



                text:"发送"



                font.pixelSize:20






                onClicked:{



                    var question =
                    questionInput.text




                    if(question.length>0)

                    {



                        /*
                            清空输入框

                        */


                        questionInput.text=""







                        /*
                            用户消息

                        */


                        chatModel.append({

                            "role":"User",

                            "message":question

                        })








                        /*
                            AI占位

                        */


                        chatModel.append({

                            "role":"AI",

                            "message":""

                        })








                        /*
                            调用C++

                        */


                        vehicleAssistant.chat(

                            question

                        )


                    }


                }


            }


        }


    }









    //========================
    //底部导航
    //========================


    Rectangle {


        width:parent.width


        height:100



        anchors.bottom:parent.bottom



        color:"#55000000"




        Row {


            anchors.centerIn:parent



            spacing:80






            NavItem{


                icon:"AI"


                title:"AI助手"



                onClicked:{

                    pageLoader.source=""

                }

            }







            NavItem{


                icon:"🚗"


                title:"车辆状态"



                onClicked:{

                    console.log("Vehicle")

                }

            }







            NavItem{


                icon:"📷"


                title:"摄像头"



                onClicked:{

                    pageLoader.source =
                        "qrc:/qml/Camera.qml"

                }

            }







            NavItem{


                icon:"☁"


                title:"云服务"



                onClicked:{

                    console.log("Cloud")

                }

            }







            NavItem{


                icon:"⚙"


                title:"设置"



                onClicked:{

                    console.log("Setting")

                }

            }



        }


    }



}