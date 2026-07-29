#include "KnowledgeBase.h"

#include <fstream>
#include <filesystem>


namespace fs = std::filesystem;



bool KnowledgeBase::load(
        const std::string& path)
{

    docs_.clear();


    for(auto& file:
        fs::directory_iterator(path))
    {

        if(file.path().extension()==".txt")
        {

            std::ifstream fin(
                file.path()
            );


            std::string text(
                (std::istreambuf_iterator<char>(fin)),
                std::istreambuf_iterator<char>()
            );


            Document doc;


            doc.filename =
                file.path().filename();


            doc.content =
                text;


            docs_.push_back(doc);

        }

    }


    return !docs_.empty();

}



const std::vector<Document>&
KnowledgeBase::documents() const
{

    return docs_;

}
