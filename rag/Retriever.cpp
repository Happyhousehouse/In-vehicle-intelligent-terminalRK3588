#include "Retriever.h"


#include <sstream>


Retriever::Retriever(
        KnowledgeBase& kb)
    :
    kb_(kb)
{

}



std::vector<std::string>
Retriever::search(
        const std::string& query)
{


    std::vector<std::string> result;



    for(auto& doc:
        kb_.documents())
    {


        if(doc.content.find(query)
            !=std::string::npos)
        {

            result.push_back(
                doc.content
            );

        }


    }


    return result;

}
