#pragma once

#include <string>

#include "RKLLMWrapper.h"


class LLMService
{
public:

    bool init(
        const std::string &model_path
    );


    void chat(
        const std::string &prompt,
        RKLLMWrapper::StreamCallback callback
    );


    void stop();


    /*
     * 清除当前模型对话上下文。
     */
    bool resetContext(
        bool keepSystemPrompt = true
    );


private:

    RKLLMWrapper wrapper_;
};