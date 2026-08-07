#include "LLMService.h"

#include <utility>
#include <QDebug>

bool LLMService::init(
    const std::string &model_path
)
{
    return wrapper_.init(
        model_path
    );
}


void LLMService::chat(
    const std::string &prompt,
    RKLLMWrapper::StreamCallback callback
)
{
    wrapper_.generate(
        prompt,
        std::move(callback)
    );
}


void LLMService::stop()
{
    wrapper_.stop();
}


bool LLMService::resetContext(
    bool keepSystemPrompt
)
{
    qDebug()
        << "[LLMService]"
        << "resetContext"
        << keepSystemPrompt;


    const bool result =
        wrapper_.clearHistory(
            keepSystemPrompt
        );


    qDebug()
        << "[LLMService]"
        << "resetContext result ="
        << result;


    return result;
}