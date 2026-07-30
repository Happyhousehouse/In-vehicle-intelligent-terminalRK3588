#include "CameraController.h"


#include <opencv2/opencv.hpp>

#include <QDebug>






CameraController::CameraController(

        QObject *parent

)

    :

      QObject(parent)

{


    provider_=nullptr;



    gallery_.init(

        "/home/firefly/Pictures"

    );





    connect(

        &timer_,

        &QTimer::timeout,

        this,

        &CameraController::updateFrame

    );


}








CameraController::~CameraController()

{

    stopCamera();

}








void CameraController::startCamera()

{


    bool ret = camera_.start(

        "/dev/video11",

        1920,

        1080

    );



    if(ret)

    {

        timer_.start(33);

    }


}








void CameraController::stopCamera()

{

    timer_.stop();


    camera_.stop();

}









void CameraController::setProvider(

        CameraImageProvider *provider

)

{

    provider_=provider;

}









void CameraController::updateFrame()

{


    cv::Mat frame =

        camera_.getFrame();



    if(frame.empty())

        return;






    QImage image(

        frame.data,

        frame.cols,

        frame.rows,

        frame.step,

        QImage::Format_BGR888

    );





    if(provider_)

    {

        provider_->updateImage(

            image

        );

    }


}









void CameraController::capturePhoto()

{


    cv::Mat photo =

        camera_.capture();




    if(photo.empty())

    {

        return;

    }







    std::string path =

        gallery_.save(

            photo

        );







    if(!path.empty())

    {


        qDebug()

        <<"photo saved:"

        <<QString::fromStdString(path);




        emit photoSaved(

            QString::fromStdString(path)

        );


    }



}