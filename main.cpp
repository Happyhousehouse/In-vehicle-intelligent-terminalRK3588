#include <QGuiApplication>

#include <QQmlApplicationEngine>

#include <QQmlContext>

#include <QDebug>



#include "controller/CameraController.h"

#include "controller/CameraImageProvider.h"

#include "controller/GalleryController.h"

#include "controller/OnboardController.h"

#include "assistant/VehicleAssistant.h"

#include "weather/WeatherService.h"

#include "camera/CameraService.h"
#include "fatigue/FatigueService.h"




int main(int argc, char *argv[])
{


    QGuiApplication app(argc, argv);

    OnboardController onboardController;


    /*
        AI助手

    */

    VehicleAssistant assistant;

    /*
        天气服务
    */
    WeatherService weatherService;
    /*
    * VehicleAssistant 使用天气服务。
    */
    assistant.setWeatherService(
        &weatherService
    );
    weatherService.setLocation(
        22.5431,
        114.0579,
        QStringLiteral("深圳")
    );


    /*
        摄像头控制

    */

    /*
    * ============================================================
    * 共享摄像头图像源
    *
    * CameraService 不属于 Camera.qml。
    *
    * 它作为车辆终端底层视觉输入，
    * 程序启动后常驻运行。
    * ============================================================
    */

    CameraService cameraService;


    /*
    * ============================================================
    * 独立疲劳检测服务
    *
    * 开机自动运行。
    * 不属于 CameraController。
    * ============================================================
    */

    FatigueService fatigueService(
        &cameraService
    );

    /*
    * VehicleAssistant 使用独立疲劳检测服务。
    */
    assistant.setFatigueService(
        &fatigueService
    );
    /*
    * ============================================================
    * 摄像头页面控制器
    *
    * 只负责 Camera.qml 的预览和拍照。
    * ============================================================
    */

    CameraController cameraController(
        &cameraService
    );




    /*
        摄像头图片提供器

    */

    CameraImageProvider *cameraProvider =
    new CameraImageProvider();








    /*
        连接Camera和Provider

    */

    cameraController.setProvider(

        cameraProvider

    );


    /*
    * ============================================================
    * 开机启动共享摄像头
    * ============================================================
    */

    const bool cameraStarted =
        cameraService.start(
            "/dev/video11",
            1920,
            1080
        );


    if (!cameraStarted)
    {
        qWarning()
            << "[Main] CameraService start failed";
    }
    else
    {
        /*
        * ========================================================
        * 开机自动启动疲劳检测
        * ========================================================
        */

        if (!fatigueService.start())
        {
            qWarning()
                << "[Main] FatigueService start failed";
        }
    }





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


    engine.rootContext()
        ->setContextProperty(
            "onboardController",
            &onboardController
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
    * 注册独立疲劳检测服务
    */

    engine.rootContext()
        ->setContextProperty(
            "fatigueService",
            &fatigueService
        );


    engine.rootContext()
        ->setContextProperty(
            "weatherService",
            &weatherService
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

        cameraProvider

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

    
    QObject::connect(
        &app,
        &QCoreApplication::aboutToQuit,
        &onboardController,
        &OnboardController::quitKeyboard
    );
    
    QObject::connect(
        &app,
        &QCoreApplication::aboutToQuit,

        &app,

        [&fatigueService, &cameraService]()
        {
            qDebug()
                << "[Main] stopping FatigueService";


            fatigueService.stop();


            qDebug()
                << "[Main] stopping CameraService";


            cameraService.stop();
        }
    );




    engine.load(url);


    /*
        首次获取天气。
    */
    weatherService.refresh();




    return app.exec();

}