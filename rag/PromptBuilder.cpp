#include "PromptBuilder.h"



std::string PromptBuilder::build(

    const std::vector<std::string>& docs,

    const std::string& question)

{


    std::string prompt;



    prompt +=
    "你是一名汽车智能助手。\n";


    prompt +=
    "请根据下面资料回答问题。\n\n";


    prompt +=
    "参考资料:\n";


    for(auto& d:docs)
    {

        prompt += d;

        prompt += "\n";

    }



    prompt +=
    "\n用户问题:\n";


    prompt += question;



    return prompt;

}
