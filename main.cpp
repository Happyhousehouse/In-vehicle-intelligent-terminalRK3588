#include "assistant/VehicleAssistant.h"


#include <iostream>



int main(
    int argc,
    char** argv
)
{


    if(argc < 2)
    {
        return -1;
    }
    
    VehicleAssistant assistant;



    if(!assistant.init(

        argv[1],

        "../knowledge"

    ))
    {

        std::cout
        <<"init failed"
        <<std::endl;


        return -1;

    }



    while(true)
    {

        std::string question;


        std::cout
        <<"User:";


        getline(
            std::cin,
            question
        );


        if(question=="exit")
            break;



        assistant.chat(
            question
        );


        std::cout
        <<std::endl;


    }



    return 0;

}