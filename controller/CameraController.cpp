#include "CameraController.h"


#include <opencv2/opencv.hpp>

#include <QDebug>






CameraController::CameraController(

        QObject *parent

)

    :

      QObject(parent),
      fatigue_(&camera_)

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
    qDebug()
        << "[CameraController] destructor begin";

    stopCamera();

    qDebug()
        << "[CameraController] destructor end";
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

        // ========================================================
        // 启动疲劳检测
        // ========================================================

        if (!fatigue_.start())
        {
            qWarning()
                << "[CameraController] FatigueService start failed";
        }

    }

    


}








void CameraController::stopCamera()
{
    qDebug()
        << "[CameraController] stopCamera begin";


    timer_.stop();


    qDebug()
        << "[CameraController] stopping FatigueService";

    // 必须先停疲劳线程
    fatigue_.stop();


    qDebug()
        << "[CameraController] FatigueService stopped";


    qDebug()
        << "[CameraController] stopping CameraService";

    // 再停摄像头
    camera_.stop();


    qDebug()
        << "[CameraController] CameraService stopped";

    qDebug()
        << "[CameraController] stopCamera end";
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


    // 重要：
    // image 默认引用 frame.data，
    // 但 frame 是局部变量。
    // copy() 后让 QImage 自己拥有图像数据。
    QImage safeImage =
        image.copy();


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