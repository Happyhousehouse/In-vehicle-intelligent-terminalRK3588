import QtQuick 2.15
import QtQuick.Controls 2.15


Rectangle {


    color:"#202020"



    Column
    {

        anchors.centerIn: parent

        spacing:30



        Text
        {

            text:"Vehicle Assistant"

            color:"white"

            font.pixelSize:50

            horizontalAlignment:
            Text.AlignHCenter

        }



        Grid
        {

            columns:2

            spacing:20



            Button
            {

                width:220

                height:100

                text:"Camera"


                onClicked:
                {

                    stack.push("Camera.qml")

                }

            }



            Button
            {

                width:220

                height:100

                text:"AI Assistant"


                onClicked:
                {

                    stack.push("AI.qml")

                }

            }



            Button
            {

                width:220

                height:100

                text:"Vehicle CAN"


                onClicked:
                {

                    stack.push("Vehicle.qml")

                }

            }



            Button
            {

                width:220

                height:100

                text:"Cloud MQTT"


                onClicked:
                {

                    stack.push("Cloud.qml")

                }

            }


        }

    }

}