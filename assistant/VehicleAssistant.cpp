#include "VehicleAssistant.h"

#include <iostream>



bool VehicleAssistant::init(
    const std::string& model,
    const std::string& knowledge
)
{


    if(!llm_.init(model))
    {
        return false;
    }



    if(!rag_.init(knowledge))
    {
        return false;
    }



    return true;

}





void VehicleAssistant::chat(
    const std::string& question
)
{


    /*
        1.
        RAG增强
    */


    std::string prompt =
        rag_.buildPrompt(
            question
        );



    /*
        2.
        LLM生成
    */


    llm_.chat(

        prompt,


        [](const std::string& token)
        {

            std::cout
            <<token
            <<std::flush;

        }

    );


}