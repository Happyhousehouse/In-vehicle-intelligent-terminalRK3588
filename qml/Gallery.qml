import QtQuick 2.15
import QtQuick.Controls 2.15


Rectangle {


    id:galleryPage


    color:"#101010"




    property var photos: []


    property bool selectMode:false


    property var selectedPhotos:[]


    property string currentPhoto:""





    /*
        清除选择状态
    */


    function clearSelection()
    {

        selectedPhotos=[]

        selectMode=false


        gridView.model=[]

        gridView.model=photos

    }





    Component.onCompleted:
    {

        photos =
        galleryController.getPhotos()

    }




    onVisibleChanged:
    {

        if(visible)
        {

            photos =
            galleryController.getPhotos()


            gridView.model=photos


        }

    }








    /*
        顶部标题
    */


    Rectangle {


        height:70


        width:parent.width



        color:"#22000000"




        Text {


            anchors.centerIn:parent



            text:"相册"



            color:"white"



            font.pixelSize:32


        }


    }








    /*
        删除模式按钮
    */


    Button {


        text:

        selectMode

        ?

        "取消"

        :

        "删除"




        anchors.right:parent.right


        anchors.top:parent.top


        anchors.margins:15




        onClicked:
        {


            if(selectMode)
            {


                clearSelection()


            }
            else
            {


                selectMode=true


            }


        }


    }








    /*
        返回按钮
    */


    Button {


        text:"返回"


        width:100


        height:45



        anchors.left:parent.left


        anchors.top:parent.top



        anchors.leftMargin:20


        anchors.topMargin:15





        onClicked:
        {


            if(mainWindow.previousPage==="Camera")
            {


                pageLoader.source=
                "qrc:/qml/Camera.qml"


            }
            else
            {


                pageLoader.source=""


            }


        }


    }












    /*
        图片列表
    */


    GridView {


        id:gridView


        z:1



        anchors.topMargin:90



        anchors.fill:parent



        anchors.margins:20



        cellWidth:200


        cellHeight:200





        model:photos







        delegate:Rectangle{


            width:180


            height:180



            radius:15



            clip:true



            property bool checked:false






            Image {


                anchors.fill:parent



                source:

                "file:///" + modelData



                fillMode:

                Image.PreserveAspectCrop



            }






            /*
                选择遮罩
            */


            Rectangle {


                anchors.fill:parent


                color:"#6600A8FF"


                visible:checked


                z:0




                Text {


                    anchors.centerIn:parent


                    text:"✓"


                    color:"white"


                    font.pixelSize:40


                }


            }








            MouseArea {


                anchors.fill:parent


                z:10





                onClicked:
                {


                    if(selectMode)
                    {


                        checked=!checked




                        if(checked)
                        {


                            selectedPhotos =
                            selectedPhotos.concat(
                                [modelData]
                            )


                        }
                        else
                        {


                            var index =
                            selectedPhotos.indexOf(modelData)



                            if(index>=0)
                            {

                                selectedPhotos.splice(
                                    index,
                                    1
                                )

                            }




                            selectedPhotos =
                            selectedPhotos.slice()


                        }


                    }
                    else
                    {


                        currentPhoto=modelData



                        bigImage.source=

                        "file:///" + modelData



                        imageViewer.visible=true


                    }


                }


            }



        }


    }













    /*
        大图查看
    */


    Rectangle {


        id:imageViewer



        anchors.fill:parent



        visible:false

        enabled:visible

        color:"black"


        z:100







        Image {


            id:bigImage



            anchors.fill:parent



            fillMode:

            Image.PreserveAspectFit



        }









        Button {


            text:"返回"



            width:100


            height:45



            anchors.left:parent.left


            anchors.top:parent.top



            anchors.margins:20





            onClicked:
            {


                imageViewer.visible=false


            }


        }








        Button {


            text:"删除"



            width:100


            height:45



            anchors.right:parent.right


            anchors.bottom:parent.bottom



            anchors.margins:30





            onClicked:
            {


                deleteSingleDialog.open()


            }


        }


    }












    Rectangle {


        id:deleteBar


        visible:selectMode

        enabled:selectMode

        z:200



        height:80



        anchors.bottom:parent.bottom


        anchors.left:parent.left


        anchors.right:parent.right



        color:"#88000000"





        Button {


            id:batchDeleteButton



            width:160


            height:50



            anchors.centerIn:parent





            text:

            "删除 ("

            +

            selectedPhotos.length

            +

            ")"




            onClicked:
            {


                console.log(
                    "batch delete clicked"
                )


                deleteDialog.open()


            }


        }


    }












    /*
        批量删除确认
    */


    Dialog {


        id:deleteDialog

        z:300


        title:"确认删除"



        standardButtons:

        Dialog.Ok |

        Dialog.Cancel





        Text {


            text:

            "确认删除选择的照片？"


        }






        onAccepted:
        {


            galleryController.deletePhotos(

                selectedPhotos

            )



            photos=

            galleryController.getPhotos()



            clearSelection()


        }

    }












    /*
        单张删除确认
    */


    Dialog {


        id:deleteSingleDialog

        z:300

        title:"确认删除"



        standardButtons:

        Dialog.Ok |

        Dialog.Cancel






        onAccepted:
        {


            galleryController.deletePhoto(

                currentPhoto

            )




            imageViewer.visible=false





            photos=

            galleryController.getPhotos()



            clearSelection()



        }


    }


}