#pragma once

#include <string>
#include <vector>


struct Document
{
    std::string filename;

    std::string content;
};



class KnowledgeBase
{

public:

    bool load(
        const std::string& path
    );


    const std::vector<Document>& documents() const;


private:

    std::vector<Document> docs_;

};