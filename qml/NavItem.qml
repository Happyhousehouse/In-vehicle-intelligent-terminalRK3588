import QtQuick 2.15



Item {


    width:80

    height:80



    property string icon:""

    property string title:""



    signal clicked()






    Column {


        anchors.centerIn:parent


        spacing:5





        Text {


            anchors.horizontalCenter:
            parent.horizontalCenter



            text:icon



            color:"white"



            font.pixelSize:30


        }







        Text {


            anchors.horizontalCenter:
            parent.horizontalCenter



            text:title



            color:"#CCCCCC"



            font.pixelSize:16


        }



    }







    MouseArea {


        anchors.fill:parent



        hoverEnabled:true



        onClicked:{


            parent.clicked()


        }


    }


}