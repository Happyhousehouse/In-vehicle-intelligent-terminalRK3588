#pragma once


#include <QObject>
#include <QString>


#include "../llm/LLMService.h"
#include "../rag/RAGService.h"



class VehicleAssistant : public QObject
{

    Q_OBJECT


public:


    explicit VehicleAssistant(
        QObject *parent=nullptr
    );



    bool init(

        const std::string& model,

        const std::string& knowledge

    );



    /*
        QML调用接口
    */

    Q_INVOKABLE
    void chat(

        const QString& question

    );



signals:



    /*
        AI流式输出

        每收到一次RKLLM callback
        发送一次

    */

    void answerToken(

        QString token

    );



    /*
        生成结束

    */

    void answerFinished();



private:


    LLMService llm_;


    RAGService rag_;



};