#include "GalleryController.h"



GalleryController::GalleryController(

        QObject *parent

)

:

QObject(parent)

{


    gallery_.init(

        "/home/firefly/Pictures"

    );


}







QStringList GalleryController::getPhotos()
{


    QStringList result;




    auto photos =

        gallery_.getPhotos();




    for(auto &p:photos)

    {


        result.append(

            QString::fromStdString(p)

        );


    }





    return result;


}

bool GalleryController::deletePhoto(
        QString path
)
{


    return gallery_.removePhoto(

        path.toStdString()

    );


}

bool GalleryController::deletePhotos(
        QStringList paths
)
{


    bool result=true;



    for(
        QString path:paths
    )
    {


        if(
            !gallery_.removePhoto(
                path.toStdString()
            )
        )
        {

            result=false;

        }


    }



    return result;


}