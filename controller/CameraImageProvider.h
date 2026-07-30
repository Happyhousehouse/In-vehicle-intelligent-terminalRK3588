#pragma once


#include <QQuickImageProvider>
#include <QImage>
#include <QMutex>



class CameraImageProvider :
        public QQuickImageProvider
{


public:


    CameraImageProvider();



    QImage requestImage(

        const QString &id,

        QSize *size,

        const QSize &requestedSize

    ) override;




    void updateImage(

        const QImage& image

    );



private:


    QImage image_;


    QMutex mutex_;


};