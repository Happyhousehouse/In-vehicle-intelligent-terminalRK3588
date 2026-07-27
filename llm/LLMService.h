#pragma once


#include <string>

#include "RKLLMWrapper.h"



class LLMService
{

public:


    bool init(
        const std::string& model_path
    );



    void chat(
        const std::string& prompt,
        RKLLMWrapper::StreamCallback callback
    );



    void stop();



private:


    RKLLMWrapper wrapper_;


};