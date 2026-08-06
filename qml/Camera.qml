import QtQuick 2.15
import QtQuick.Controls 2.15





Rectangle {


    id:cameraPage


    color:"#101010"




    property int frameCounter:0


    property string newPhotoPath:""


    Component.onCompleted:
    {


        console.log(
            "Camera Page Loaded"
        )



        cameraController.startCamera()





        /*
            加载最新照片

        */


        var photos =

            galleryController.getPhotos()






        if(photos.length > 0)

        {


            lastPhoto.source =

                "file:///" + photos[0]


        }



    }









    Component.onDestruction:
    {

        cameraController.stopCamera()

    }









    /*
        拍照完成

    */

    Connections {


        target:cameraController



        function onPhotoSaved(path)

        {


            console.log(

                "photo saved:",

                path

            )




            /*
                更新最新照片

            */


            newPhotoPath = path

            photoChangeAnimation.start()



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
        刷新画面

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
        白色快门闪光

    */


    Rectangle {


        id:flashLayer



        anchors.fill:parent



        color:"white"



        opacity:0



    }









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
        最新照片显示

        固定存在

    */


    Rectangle {


        id:lastPhotoBox



        width:180


        height:180



        radius:15





        anchors.left:parent.left



        anchors.bottom:captureArea.top



        anchors.leftMargin:30



        anchors.bottomMargin:30





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

        Rectangle {


            id:photoBlackLayer



            anchors.fill:parent



            color:"black"



            opacity:0



        }







        MouseArea {


            anchors.fill:parent



            onClicked:

            {

                mainWindow.previousPage="Camera"

                pageLoader.source=

                    "qrc:/qml/Gallery.qml"



            }


        }


    }

    SequentialAnimation {


        id:photoChangeAnimation



        NumberAnimation {


            target:photoBlackLayer


            property:"opacity"



            from:0


            to:1



            duration:100


        }





        ScriptAction {


            script:
            {


                lastPhoto.source =

                "file:///" + newPhotoPath



            }


        }






        NumberAnimation {


            target:photoBlackLayer


            property:"opacity"



            from:1


            to:0



            duration:200


        }



    }


    /*
        底部拍照区域

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



        width:140



        height:140



        radius:45





        anchors.centerIn:captureArea




        color:"white"







        Rectangle {


            anchors.centerIn:parent



            width:110



            height:110



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
        按钮恢复

    */


    Timer {


        id:buttonTimer



        interval:120



        onTriggered:

        {

            captureButton.scale=1


        }


    }

    /*
    返回主界面按钮
*/


    Rectangle {


        id:backButton


        width:160


        height:70


        radius:30



        anchors.left:parent.left


        anchors.top:parent.top



        anchors.leftMargin:20


        anchors.topMargin:20



        color:"#55000000"



        z:999






        Text {


            anchors.centerIn:parent



            text:"← 返回"



            color:"white"



            font.pixelSize:20



        }








        MouseArea {


            anchors.fill:parent



            onClicked:

            {


                pageLoader.source=""



            }


        }



    }




}