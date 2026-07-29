#pragma once


#include <string>


#include "KnowledgeBase.h"
#include "Retriever.h"



class RAGService
{

public:


    bool init(
        const std::string& path
    );



    std::string buildPrompt(
        const std::string& question
    );



private:


    KnowledgeBase kb_;

    Retriever* retriever_;


};