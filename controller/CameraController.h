#pragma once


#include <QObject>
#include <QImage>
#include <QTimer>
#include <QString>


#include "../camera/CameraService.h"

#include "../photo/GalleryService.h"

#include "CameraImageProvider.h"


// ============================================================
// CameraController
//
// 现在只负责：
//
// Camera.qml预览
// 拍照
//
// 不再：
// 创建 CameraService
// 启停 CameraService
// 创建 FatigueService
// 启停 FatigueService
//
// CameraService 由 main.cpp 统一持有。
// ============================================================

class CameraController : public QObject
{
    Q_OBJECT


public:

    explicit CameraController(
        CameraService* camera,
        QObject* parent = nullptr
    );


    ~CameraController();
    bool isPreviewActive() const;

public slots:

    // 打开摄像头页面时：
    // 只启动UI预览刷新Timer。
    void startCamera();


    // 离开摄像头页面时：
    // 只停止UI预览刷新Timer。
    void stopCamera();


    void capturePhoto();


    void setProvider(
        CameraImageProvider* provider
    );


signals:

    void photoSaved(
        QString path
    );


private slots:

    void updateFrame();


private:

    // 共享 CameraService，不拥有它。
    CameraService* camera_ = nullptr;


    GalleryService gallery_;


    QTimer timer_;


    CameraImageProvider* provider_ =
        nullptr;
};