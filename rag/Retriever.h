#pragma once


#include "KnowledgeBase.h"


#include <vector>


class Retriever
{


public:


    Retriever(
        KnowledgeBase& kb
    );


    std::vector<std::string>
    search(
        const std::string& query
    );


private:

    KnowledgeBase& kb_;

};
