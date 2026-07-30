import QtQuick 2.15
import QtQuick.Controls 2.15



Rectangle {


    id:cameraPage


    color:"#101010"




    property int frameCounter:0







    Component.onCompleted:
    {

        console.log("Camera Page Loaded")


        cameraController.startCamera()

    }







    Component.onDestruction:
    {

        cameraController.stopCamera()

    }









    /*
        接收拍照完成信号

    */

    Connections {


        target:cameraController



        function onPhotoSaved(path)

        {


            console.log(
                "photo saved:",
                path
            )



            lastPhoto.source =

                    "file:///" + path





            photoBox.visible=true




            photoShowAnimation.start()



            photoHideTimer.restart()



        }


    }












    /*
        摄像头实时画面

    */


    Image {


        id:cameraView



        anchors.fill:parent



        source:

            "image://camera/live/" + frameCounter





        fillMode:

            Image.PreserveAspectFit





        cache:false



    }









    /*
        刷新ImageProvider

    */


    Timer {


        interval:33


        running:true


        repeat:true



        onTriggered:

        {

            frameCounter++


        }


    }












    /*
        白色快门闪光层

    */


    Rectangle {


        id:flashLayer



        anchors.fill:parent



        color:"white"



        opacity:0



    }









    /*
        快门闪光动画

    */


    SequentialAnimation {


        id:flashAnimation



        NumberAnimation {


            target:flashLayer



            property:"opacity"



            from:0



            to:0.85



            duration:50



        }






        NumberAnimation {


            target:flashLayer



            property:"opacity"



            from:0.85



            to:0



            duration:150



        }


    }













    /*
        摄像头状态

    */


    Rectangle {


        width:180


        height:45



        radius:20



        anchors.top:parent.top



        anchors.right:parent.right



        anchors.margins:20





        color:"#55000000"





        Text {


            anchors.centerIn:parent



            text:"● Camera ON"



            color:"#00FF88"



            font.pixelSize:18


        }


    }












    /*
        最新照片缩略图

    */


    Rectangle {


        id:photoBox



        width:120


        height:120



        radius:15





        visible:false





        anchors.right:

            parent.right



        anchors.rightMargin:30





        y:

            parent.height+20





        color:"#66000000"





        border.width:2


        border.color:"white"







        Image {


            id:lastPhoto



            anchors.fill:parent



            anchors.margins:5




            fillMode:

                Image.PreserveAspectCrop




            cache:false



        }

        MouseArea {


            anchors.fill:parent



            onClicked:

            {


                console.log(
                    "open gallery"
                )



                pageLoader.source =

                    "qrc:/qml/Gallery.qml"



            }


        }

    }









    /*
        缩略图进入动画

    */


    NumberAnimation {


        id:photoShowAnimation



        target:photoBox



        property:"y"




        from:

            parent.height+20




        to:

            parent.height-250




        duration:400




        easing.type:

            Easing.OutCubic



    }









    /*
        缩略图停留时间

    */


    Timer {


        id:photoHideTimer



        interval:3000




        onTriggered:

        {

            photoHideAnimation.start()


        }


    }









    /*
        缩略图退出动画

    */


    NumberAnimation {


        id:photoHideAnimation



        target:photoBox



        property:"y"




        from:

            parent.height-250




        to:

            parent.height+20




        duration:400




        easing.type:

            Easing.InCubic






        onFinished:

        {

            photoBox.visible=false

        }


    }












    /*
        底部区域

    */


    Rectangle {


        id:captureArea



        height:120



        anchors.left:parent.left


        anchors.right:parent.right


        anchors.bottom:parent.bottom




        color:"#22000000"


    }












    /*
        拍照按钮

    */


    Rectangle {


        id:captureButton



        width:90


        height:90



        radius:45




        anchors.centerIn:

            captureArea





        color:"white"







        Rectangle {


            anchors.centerIn:parent



            width:70


            height:70



            radius:35



            color:"#222222"



        }








        MouseArea {


            anchors.fill:parent



            onClicked:

            {


                /*
                    按钮反馈
                */

                captureButton.scale=0.85


                buttonTimer.start()


                /*
                    快门闪光

                */


                flashAnimation.start()






                /*
                    拍照

                */


                cameraController.capturePhoto()



            }


        }



    }









    /*
        按钮恢复动画

    */


    Timer {


        id:buttonTimer



        interval:120



        onTriggered:

        {

            captureButton.scale=1

        }


    }





}