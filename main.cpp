#include <QGuiApplication>

#include <QQmlApplicationEngine>

#include <QQmlContext>

#include <QDebug>
#include "audio/AudioInputService.h"
#include "audio/WakeWordService.h"

#include "controller/CameraController.h"

#include "controller/CameraImageProvider.h"

#include "controller/GalleryController.h"

#include "controller/OnboardController.h"

#include "assistant/VehicleAssistant.h"

#include "weather/WeatherService.h"

#include "camera/CameraService.h"
#include "fatigue/FatigueService.h"
#include "audio/VADService.h"
#include "audio/ASRService.h"
#include "vision/VisionServiceManager.h"
#include "audio/VoiceController.h"

int main(int argc, char *argv[])
{


    QGuiApplication app(argc, argv);

    OnboardController onboardController;


    /*
        AI助手

    */

    VehicleAssistant assistant;
    /*
    * ============================================================
    * 麦克风输入服务
    *
    * 程序启动后持续采集。
    *
    * 后面 WakeWordService 会持续监听这里输出的 PCM。
    * ============================================================
    */
    WakeWordService wakeWordService;
    VADService vadService;
    ASRService asrService;
    AudioInputService audioInputService;

    const std::string asrModelPath =
        "/home/firefly/VehicleTerminal/model/asr/"
        "sherpa-onnx-sense-voice-zh-en-ja-ko-yue-int8-2024-07-17";


    if (!asrService.init(asrModelPath))
    {
        qWarning()
            << "[Main] ASRService init failed";
    }

    const std::string vadModelPath =
        "/home/firefly/VehicleTerminal/model/vad/"
        "silero_vad.int8.onnx";


    if (!vadService.init(vadModelPath))
    {
        qWarning()
            << "[Main] VADService init failed";
    }

    const std::string kwsModelPath =
    "/home/firefly/VehicleTerminal/model/kws/"
    "sherpa-onnx-kws-zipformer-wenetspeech-3.3M-2024-01-01";


    const bool wakeWordReady =
        wakeWordService.init(
            kwsModelPath
        );

    if (!wakeWordReady)
    {
        qWarning()
            << "[Main] WakeWordService init failed";
    }
    else
    {
        qDebug()
            << "[Main] WakeWordService ready";
    }

    QObject::connect(
        &audioInputService,
        &AudioInputService::pcmDataReady,

        &wakeWordService,
        &WakeWordService::processPcm,

        Qt::DirectConnection
    );
    QObject::connect(
        &audioInputService,
        &AudioInputService::pcmDataReady,

        &vadService,
        &VADService::processPcm,

        Qt::DirectConnection
    );



    QObject::connect(
        &vadService,
        &VADService::speechSegmentReady,

        &app,

        [](const QByteArray &pcm)
        {
            const int samples =
                pcm.size() / 2;


            const double duration =
                static_cast<double>(
                    samples
                )
                /
                16000.0;


            qDebug()
                << "====================================";

            qDebug()
                << "[Main] VOICE COMMAND CAPTURED";

            qDebug()
                << "[Main] duration ="
                << duration
                << "seconds";

            qDebug()
                << "[Main] bytes ="
                << pcm.size();

            qDebug()
                << "====================================";
        }
    );

    if (
        !audioInputService.open(
            "hw:CARD=Camera,DEV=0",
            16000,
            1
        )
    )
    {
        qWarning()
            << "[Main] AudioInputService open failed";
    }
    else
    {
        if (!audioInputService.start())
        {
            qWarning()
                << "[Main] AudioInputService start failed";
        }
    }
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
    * ============================================================
    * 底层共享摄像头
    * ============================================================
    */
    CameraService cameraService;


    /*
    * ============================================================
    * 疲劳检测业务
    *
    * 注意：
    * 对象仍然必须存在。
    * 只是 VehicleAssistant 不再直接持有它。
    * ============================================================
    */
    FatigueService fatigueService(
        &cameraService
    );


    /*
    * ============================================================
    * 相机业务 Controller
    *
    * 对象仍然必须存在。
    * Camera.qml 还需要它。
    * ============================================================
    */
    CameraController cameraController(
        &cameraService
    );


    /*
    * ============================================================
    * 摄像头图片 Provider
    * ============================================================
    */
    CameraImageProvider *cameraProvider =
        new CameraImageProvider();


    cameraController.setProvider(
        cameraProvider
    );


    /*
    * ============================================================
    * 视觉业务统一管理器
    * ============================================================
    */
    VisionServiceManager visionManager(
        &cameraService,
        &cameraController,
        &fatigueService
    );


    visionManager.configureCamera(
        "/dev/video11",
        1920,
        1080
    );


    /*
    * VehicleAssistant 只绑定 VisionServiceManager。
    */
    assistant.setVisionServiceManager(
        &visionManager
    );


    /*
    * ============================================================
    * 开机启动摄像头
    * ============================================================
    */
    const bool cameraStarted =
        visionManager.openCameraDevice();


    if (!cameraStarted)
    {
        qWarning()
            << "[Main] CameraService start failed";
    }
    else
    {
        /*
        * 开机自动开启疲劳检测。
        */
        if (!visionManager.enableFatigue())
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

    /*
    * ============================================================
    * Voice Controller
    *
    * 统一管理：
    *
    * WakeWord
    * VAD
    * ASR
    * VehicleAssistant
    *
    * 首次唤醒等待：5秒
    * 正式交流等待：30秒
    * ============================================================
    */

    VoiceController voiceController(
        &wakeWordService,
        &vadService,
        &asrService,
        &assistant
    );







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

        [url](QObject *obj, const QUrl &objUrl)
        {
            if (!obj && url == objUrl)
            {
                QCoreApplication::exit(-1);
            }
        },

        Qt::QueuedConnection
    );


    /*
    * ============================================================
    * 程序退出：
    * 关闭软键盘
    * ============================================================
    */

    QObject::connect(
        &app,
        &QCoreApplication::aboutToQuit,

        &onboardController,
        &OnboardController::quitKeyboard
    );


    /*
    * ============================================================
    * 程序退出：
    * 停止麦克风采集
    * ============================================================
    */

    QObject::connect(
        &app,
        &QCoreApplication::aboutToQuit,

        &app,

        [&audioInputService]()
        {
            qDebug()
                << "[Main] stopping AudioInputService";

            audioInputService.stop();
        }
    );


    /*
    * ============================================================
    * 程序退出：
    * 停止疲劳检测和摄像头
    * ============================================================
    */

    QObject::connect(
        &app,
        &QCoreApplication::aboutToQuit,

        &app,

        [&visionManager]()
        {
            qDebug()
                << "[Main] stopping vision services";


            /*
            * 先关闭上层疲劳检测依赖。
            */
            visionManager.disableFatigue();


            /*
            * 再关闭底层摄像头。
            */
            visionManager.closeCameraDevice();
        }
    );




    engine.load(url);


    /*
        首次获取天气。
    */
    weatherService.refresh();




    return app.exec();

}