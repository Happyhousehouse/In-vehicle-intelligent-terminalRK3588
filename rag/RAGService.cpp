#include "RAGService.h"

#include "PromptBuilder.h"



bool RAGService::init(
    const std::string& path
)
{


    if(!kb_.load(path))
    {
        return false;
    }


    retriever_ =
        new Retriever(kb_);



    return true;

}





std::string RAGService::buildPrompt(
    const std::string& question
)
{


    auto docs =
        retriever_->search(
            question
        );


    return PromptBuilder::build(
        docs,
        question
    );


}