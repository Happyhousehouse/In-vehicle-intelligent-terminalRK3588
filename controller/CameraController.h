#pragma once


#include <QObject>
#include <QImage>
#include <QTimer>
#include <QString>


#include "../camera/CameraService.h"

#include "../photo/GalleryService.h"


#include "CameraImageProvider.h"





class CameraController : public QObject
{

    Q_OBJECT



public:


    explicit CameraController(

        QObject *parent=nullptr

    );


    ~CameraController();





public slots:


    void startCamera();


    void stopCamera();


    void capturePhoto();




    void setProvider(

        CameraImageProvider *provider

    );





signals:


    //通知QML照片保存完成

    void photoSaved(

        QString path

    );





private slots:


    void updateFrame();





private:


    CameraService camera_;


    GalleryService gallery_;



    QTimer timer_;



    CameraImageProvider *provider_;



};