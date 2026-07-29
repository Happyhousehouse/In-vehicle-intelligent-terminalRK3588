#pragma once


#include <string>
#include <vector>


class PromptBuilder
{

public:


static std::string build(

    const std::vector<std::string>& docs,

    const std::string& question

);


};
