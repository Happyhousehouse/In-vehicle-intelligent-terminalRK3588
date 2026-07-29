#include "VehicleAssistant.h"


#include <QMetaObject>
#include <QDebug>


#include <thread>





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


    std::string q =
            question.toStdString();





    /*
        后台线程执行LLM

        防止阻塞Qt界面

    */


    std::thread(

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



                    QString qtoken =

                    QString::fromStdString(token);






                    /*
                        回到Qt主线程

                    */


                    QMetaObject::invokeMethod(

                        this,


                        [this,qtoken]()

                        {


                            emit answerToken(

                                qtoken

                            );


                        },


                        Qt::QueuedConnection


                    );



                }


            );









            /*
                通知QML结束

            */


            QMetaObject::invokeMethod(

                this,


                [this]()

                {


                    emit answerFinished();


                },


                Qt::QueuedConnection


            );




        }


    ).detach();



}