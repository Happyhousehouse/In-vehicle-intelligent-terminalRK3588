#include <QGuiApplication>

#include <QQmlApplicationEngine>

#include <QQmlContext>

#include <QDebug>



#include "controller/CameraController.h"

#include "controller/CameraImageProvider.h"

#include "controller/GalleryController.h"



#include "assistant/VehicleAssistant.h"








int main(int argc, char *argv[])
{


    QGuiApplication app(argc, argv);







    /*
        AI助手

    */

    VehicleAssistant assistant;







    /*
        摄像头控制

    */

    CameraController cameraController;








    /*
        摄像头图片提供器

    */

    CameraImageProvider cameraProvider;








    /*
        连接Camera和Provider

    */

    cameraController.setProvider(

        &cameraProvider

    );









    /*
        相册控制

    */

    GalleryController galleryController;









    /*
        AI模型路径

    */

    QString modelPath =

    "/home/firefly/VehicleTerminal/model/Qwen3.5-0.8B_w8a8_rk3588.rkllm";









    /*
        知识库路径

    */

    QString knowledgePath =

    "/home/firefly/VehicleTerminal/knowledge";












    /*
        初始化AI

    */

    if(!assistant.init(

        modelPath.toStdString(),

        knowledgePath.toStdString()

    ))

    {


        qDebug()

        <<"VehicleAssistant init failed";


        return -1;


    }









    QQmlApplicationEngine engine;









    /*
        注册AI

    */

    engine.rootContext()

    ->setContextProperty(

        "vehicleAssistant",

        &assistant

    );









    /*
        注册Camera

    */

    engine.rootContext()

    ->setContextProperty(

        "cameraController",

        &cameraController

    );









    /*
        注册Gallery

    */

    engine.rootContext()

    ->setContextProperty(

        "galleryController",

        &galleryController

    );









    /*
        注册图片Provider

        QML:

        image://camera/live

    */

    engine.addImageProvider(

        "camera",

        &cameraProvider

    );









    const QUrl url(

        QStringLiteral(

            "qrc:/qml/Main.qml"

        )

    );









    QObject::connect(

        &engine,

        &QQmlApplicationEngine::objectCreated,

        &app,

        [url](QObject *obj,const QUrl &objUrl)

        {

            if(!obj && url==objUrl)

                QCoreApplication::exit(-1);


        },

        Qt::QueuedConnection

    );









    engine.load(url);







    return app.exec();

}