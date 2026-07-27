cmake_minimum_required(VERSION 3.10)

project(rkllm_vehicle_assistant)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



# ======================
# 源文件
# ======================

set(SOURCES

    main.cpp


    assistant/VehicleAssistant.cpp


    llm/LLMService.cpp
    llm/RKLLMWrapper.cpp


    rag/KnowledgeBase.cpp
    rag/Retriever.cpp
    rag/PromptBuilder.cpp
    rag/RAGService.cpp

)



add_executable(
    rkllm_demo
    ${SOURCES}
)



# ======================
# 头文件路径
# ======================

target_include_directories(
    rkllm_demo

    PRIVATE

    include

    assistant

    llm

    rag
)



# ======================
# RKLLM动态库
# ======================

target_link_directories(
    rkllm_demo

    PRIVATE

    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)



target_link_libraries(

    rkllm_demo

    rkllmrt

    pthread

)
