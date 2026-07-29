#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>


#include "assistant/VehicleAssistant.h"



int main(int argc, char *argv[])
{

    QGuiApplication app(argc, argv);



    /*
        创建AI助手对象
    */

    VehicleAssistant assistant;



    /*
        模型路径
    */

    QString modelPath =
    "/home/firefly/VehicleTerminal/model/Qwen3.5-0.8B_w8a8_rk3588.rkllm";



    /*
        知识库路径
    */

    QString knowledgePath =
    "/home/firefly/VehicleTerminal/knowledge";





    /*
        初始化LLM + RAG
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
        暴露给QML

        QML:
        vehicleAssistant.chat()

    */


    engine.rootContext()
    ->setContextProperty(

        "vehicleAssistant",

        &assistant

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