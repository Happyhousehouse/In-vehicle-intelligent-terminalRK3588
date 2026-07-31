#pragma once


#include <QObject>
#include <QString>


#include "../llm/LLMService.h"
#include "../rag/RAGService.h"

#include <thread>
#include <atomic>
#include <mutex>

class CameraController;


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

    ~VehicleAssistant();

    Q_INVOKABLE
    void chat(

        const QString& question

    );



signals:



    void answerToken(

        QString token

    );



    void answerFinished();



    /*
        打开/关闭摄像头页面

    */

    void openCameraPage();
    /*
        打开相册页面

    */
    void openGalleryPage();

private:

    LLMService llm_;


    RAGService rag_;

    
    std::thread llmThread_;


    std::atomic<bool> llmRunning_{false};



    std::mutex llmMutex_;

};