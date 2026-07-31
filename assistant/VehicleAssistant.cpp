#include "VehicleAssistant.h"


#include <QMetaObject>
#include <QDebug>


#include <thread>

#include <mutex>

VehicleAssistant::VehicleAssistant(
        QObject *parent
)
    :
    QObject(parent)

{

}






bool VehicleAssistant::init(

        const std::string& model,

        const std::string& knowledge

)
{


    /*
        初始化RKLLM
    */


    if(!llm_.init(model))
    {

        return false;

    }





    /*
        初始化RAG

    */


    if(!rag_.init(knowledge))
    {

        return false;

    }



    return true;


}









void VehicleAssistant::chat(

        const QString& question

)
{

    /*
        中断当前回答
    */
    if(llmRunning_)
    {

        llm_.stop();

        llmRunning_=false;

    }

    QString cmd = question.trimmed();

    /*
        相册控制命令
    */

    if(cmd.contains("打开相册") ||
       cmd.contains("查看相册") ||
       cmd.contains("打开照片") ||
       cmd.contains("查看照片"))
    {


        QMetaObject::invokeMethod(

            this,

            [this]()
            {

                emit openGalleryPage();


            },

            Qt::QueuedConnection

        );



        emit answerToken(

            "正在打开相册"

        );



        emit answerFinished();



        return;


    }

    /*
        摄像头控制命令
    */


    if(cmd.contains("打开摄像头") ||
       cmd.contains("开启摄像头") ||
       cmd.contains("打开相机"))
    {


        QMetaObject::invokeMethod(

            this,

            [this]()
            {

                emit openCameraPage();


            },

            Qt::QueuedConnection

        );



        emit answerToken(

            "正在打开摄像头"

        );


        emit answerFinished();


        return;


    }






    /*
        拍照命令

    */


    if(cmd.contains("拍照") ||
       cmd.contains("拍一张"))
    {


        emit answerToken(

            "拍照功能暂未绑定"

        );


        emit answerFinished();


        return;


    }


    
    std::string q =
            question.toStdString();





    /*
        后台线程执行LLM

        防止阻塞Qt界面

    */


    {


        std::lock_guard<std::mutex> lock(
            llmMutex_
        );


        llmRunning_=true;


    }



    llmThread_=std::thread(

    [this,q]()

    {


        /*
            RAG增强
        */

        std::string prompt =

                rag_.buildPrompt(q);





        /*
            RKLLM推理
        */

        llm_.chat(

            prompt,


            [this](const std::string& token)

            {


                if(!llmRunning_)
                    return;



                QString qtoken =

                QString::fromStdString(token);





                QMetaObject::invokeMethod(

                    this,


                    [this,qtoken]()

                    {

                        if(llmRunning_)
                        {


                            emit answerToken(

                                qtoken

                            );


                        }


                    },


                    Qt::QueuedConnection


                );



            }


        );





        llmRunning_=false;





        QMetaObject::invokeMethod(

            this,


            [this]()

            {

                emit answerFinished();


            },


            Qt::QueuedConnection

        );



    }

    );

}

VehicleAssistant::~VehicleAssistant()
{


    llm_.stop();



    llmRunning_=false;



    if(llmThread_.joinable())
    {

        llmThread_.join();

    }


}