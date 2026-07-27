#pragma once


#include <string>


#include "../llm/LLMService.h"
#include "../rag/RAGService.h"



class VehicleAssistant
{


public:


    bool init(
        const std::string& model,
        const std::string& knowledge
    );



    void chat(
        const std::string& question
    );



private:


    LLMService llm_;

    RAGService rag_;


};