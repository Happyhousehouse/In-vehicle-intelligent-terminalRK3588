#include "LLMService.h"



bool LLMService::init(
    const std::string& model_path
)
{

    return wrapper_.init(
        model_path
    );

}



void LLMService::chat(
    const std::string& prompt,
    RKLLMWrapper::StreamCallback callback
)
{


    wrapper_.generate(
        prompt,
        callback
    );


}



void LLMService::stop()
{

    wrapper_.stop();

}