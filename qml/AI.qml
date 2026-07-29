import QtQuick 2.15
import QtQuick.Controls 2.15


Rectangle {

    anchors.fill: parent

    color: "transparent"



    Column {

        anchors.centerIn: parent

        spacing: 20



        TextField {

            id: inputBox

            width: 500

            height: 60


            placeholderText:
                "请输入问题"


            font.pixelSize: 24

        }



        Button {

            width: 200

            height: 60


            text:"发送"


            font.pixelSize:24


            onClicked: {


                answer.text =
                vehicleAssistant.chat(
                    inputBox.text
                )


            }

        }




        Rectangle {


            width:600

            height:200


            color:"#333333"


            radius:15



            Text {


                id:answer


                anchors.fill: parent


                anchors.margins:20


                wrapMode:
                    Text.WordWrap



                color:"white"


                font.pixelSize:22



                text:
                "AI回复显示区域"


            }


        }


    }

}