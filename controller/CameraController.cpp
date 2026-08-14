#include "CameraController.h"


#include <opencv2/opencv.hpp>

#include <QDebug>


CameraController::CameraController(
    CameraService* camera,
    QObject* parent
)
    :
      QObject(parent),
      camera_(camera)
{
    provider_ = nullptr;


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


    // 现在只停止预览Timer。
    //
    // 绝对不关闭共享 CameraService，
    // 更不会操作 FatigueService。
    stopCamera();


    qDebug()
        << "[CameraController] destructor end";
}


// ============================================================
// 开启Camera页面预览
// ============================================================

void CameraController::startCamera()
{
    if (camera_ == nullptr)
    {
        qWarning()
            << "[CameraController] CameraService is null";

        return;
    }


    // CameraService已经由main.cpp常驻运行。
    //
    // Camera页面这里只启动画面刷新。
    if (!timer_.isActive())
    {
        timer_.start(33);
    }


    qDebug()
        << "[CameraController] preview started";
}


// ============================================================
// 停止Camera页面预览
// ============================================================

void CameraController::stopCamera()
{
    if (timer_.isActive())
    {
        timer_.stop();
    }


    // --------------------------------------------------------
    // 注意：
    //
    // 这里不能：
    //
    // camera_->stop();
    // fatigueService.stop();
    //
    // 因为离开Camera页面不能影响疲劳检测。
    // --------------------------------------------------------

    qDebug()
        << "[CameraController] preview stopped";
}


void CameraController::setProvider(
    CameraImageProvider* provider
)
{
    provider_ =
        provider;
}


void CameraController::updateFrame()
{
    if (camera_ == nullptr)
    {
        return;
    }


    cv::Mat frame =
        camera_->getFrame();


    if (frame.empty())
    {
        return;
    }


    QImage image(
        frame.data,
        frame.cols,
        frame.rows,
        frame.step,
        QImage::Format_BGR888
    );


    QImage safeImage =
        image.copy();


    if (provider_)
    {
        provider_->updateImage(
            safeImage
        );
    }
}


void CameraController::capturePhoto()
{
    if (camera_ == nullptr)
    {
        return;
    }


    cv::Mat photo =
        camera_->capture();


    if (photo.empty())
    {
        return;
    }


    std::string path =
        gallery_.save(
            photo
        );


    if (!path.empty())
    {
        qDebug()
            << "photo saved:"
            << QString::fromStdString(
                path
            );


        emit photoSaved(
            QString::fromStdString(
                path
            )
        );
    }
}