cmake_minimum_required(VERSION 3.16)


project(VehicleTerminal)



# =========================
# C++设置
# =========================

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED ON)



# =========================
# Qt自动功能
# =========================

set(CMAKE_AUTOMOC ON)

set(CMAKE_AUTOUIC ON)

set(CMAKE_AUTORCC ON)





# =========================
# 查找Qt
# =========================

find_package(Qt5 REQUIRED COMPONENTS

    Core

    Gui

    Qml

    Quick

)





# =========================
# OpenCV
# 摄像头模块需要
# =========================

find_package(OpenCV REQUIRED)






# =========================
# QML资源
# =========================

set(QML_FILES

    qml/Main.qml

    qml/Home.qml

    qml/AI.qml

    qml/Camera.qml

    qml/Cloud.qml

    qml/Vehicle.qml

    qml/Setting.qml

    qml/NavItem.qml

)





qt5_add_resources(

    RESOURCE_FILES

    resources.qrc

)








# =========================
# C++源码
# =========================


set(SOURCES


    # =====================
    # Qt入口
    # =====================

    main.cpp





    # =====================
    # Camera模块
    # =====================


    camera/CameraService.cpp

    camera/CameraService.h





    # =====================
    # Photo模块
    # =====================


    photo/GalleryService.cpp

    photo/GalleryService.h





    # =====================
    # Qt控制层
    # =====================


    controller/CameraController.cpp

    controller/CameraController.h


    controller/CameraImageProvider.cpp

    controller/CameraImageProvider.h





    # =====================
    # AI Assistant
    # =====================


    assistant/VehicleAssistant.cpp

    assistant/VehicleAssistant.h





    # =====================
    # LLM
    # =====================


    llm/LLMService.cpp

    llm/LLMService.h


    llm/RKLLMWrapper.cpp

    llm/RKLLMWrapper.h





    # =====================
    # RAG
    # =====================


    rag/KnowledgeBase.cpp

    rag/KnowledgeBase.h


    rag/RAGService.cpp

    rag/RAGService.h


    rag/PromptBuilder.cpp

    rag/PromptBuilder.h


    rag/Retriever.cpp

    rag/Retriever.h



)









# =========================
# 生成可执行文件
# =========================


add_executable(

    VehicleTerminal


    ${SOURCES}


    ${RESOURCE_FILES}

)









# =========================
# 头文件路径
# =========================


target_include_directories(

    VehicleTerminal

    PRIVATE



    ${CMAKE_CURRENT_SOURCE_DIR}



    ${CMAKE_CURRENT_SOURCE_DIR}/include



    # AI

    ${CMAKE_CURRENT_SOURCE_DIR}/assistant



    # LLM

    ${CMAKE_CURRENT_SOURCE_DIR}/llm



    # RAG

    ${CMAKE_CURRENT_SOURCE_DIR}/rag



    # Camera

    ${CMAKE_CURRENT_SOURCE_DIR}/camera



    # Photo

    ${CMAKE_CURRENT_SOURCE_DIR}/photo



    # Controller

    ${CMAKE_CURRENT_SOURCE_DIR}/controller


)









# =========================
# 链接库
# =========================


target_link_libraries(

    VehicleTerminal



    Qt5::Core

    Qt5::Gui

    Qt5::Qml

    Qt5::Quick




    # OpenCV

    ${OpenCV_LIBS}





    # RKLLM

    ${CMAKE_CURRENT_SOURCE_DIR}/lib/librkllmrt.so


)









# =========================
# RKLLM运行时搜索路径
# =========================


set_target_properties(

    VehicleTerminal

    PROPERTIES



    BUILD_RPATH

    "${CMAKE_CURRENT_SOURCE_DIR}/lib"


)