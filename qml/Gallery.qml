import QtQuick 2.15
import QtQuick.Controls 2.15


Rectangle {


    color:"#101010"



    Text {


        anchors.top:parent.top

        anchors.horizontalCenter:
            parent.horizontalCenter



        anchors.topMargin:30



        text:"我的相册"



        color:"white"



        font.pixelSize:36


    }






    Button {


        text:"返回"



        width:120

        height:50



        anchors.left:parent.left



        anchors.top:parent.top



        anchors.leftMargin:30



        anchors.topMargin:30




        onClicked:

        {

            pageLoader.source =
                "qrc:/qml/Camera.qml"


        }


    }






}