#include "CameraImageProvider.h"


#include <QMutexLocker>
#include <QDebug>



CameraImageProvider::CameraImageProvider()

    :
      QQuickImageProvider(
          QQuickImageProvider::Image
      )

{

}






QImage CameraImageProvider::requestImage(

        const QString &id,

        QSize *size,

        const QSize &requestedSize

)
{


    QMutexLocker locker(

        &mutex_

    );



    if(size)

    {

        *size = image_.size();

    }





    return image_;



}







void CameraImageProvider::updateImage(

        const QImage& image

)
{


    QMutexLocker locker(

        &mutex_

    );



    image_ = image.copy();




}