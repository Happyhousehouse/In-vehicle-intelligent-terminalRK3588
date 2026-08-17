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
# sherpa-onnx
# =========================

set(
    SHERPA_ONNX_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/model/third_party/sherpa-onnx-v1.13.4-linux-aarch64-shared-cpu"
)



# =========================
# 查找Qt
# =========================

find_package(Qt5 REQUIRED COMPONENTS

    Core
    Gui
    Qml
    Quick
    DBus
    Network
)





# =========================
# OpenCV
# 摄像头模块需要
# =========================

find_package(OpenCV REQUIRED)


find_package(ALSA REQUIRED)


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
    # Audio
    # =====================

    audio/AudioInputService.cpp
    audio/AudioInputService.h

    audio/WakeWordService.cpp
    audio/WakeWordService.h
    audio/VADService.cpp
    audio/VADService.h
    audio/ASRService.cpp
    audio/ASRService.h
    audio/VoiceController.cpp
    audio/VoiceController.h
    audio/VoiceCommandNormalizer.cpp
    audio/VoiceCommandNormalizer.h
    # =====================
    # Camera模块
    # =====================


    camera/CameraService.cpp

    camera/CameraService.h
    # =====================
    # Vision模块
    # =====================
    vision/VisionServiceManager.cpp
    vision/VisionServiceManager.h
    # =====================
    # Fatigue Detection
    # =====================

    fatigue/FatigueEventTracker.cpp
    fatigue/FatigueEventTracker.h
    fatigue/FatigueTypes.h
    fatigue/FatigueVision.cpp
    fatigue/FatigueVision.h
    fatigue/FatigueService.cpp
    fatigue/FatigueService.h
    fatigue/FatigueWindowAnalyzer.cpp
    fatigue/FatigueWindowAnalyzer.h
    fatigue/FatigueBaselineManager.cpp
    fatigue/FatigueBaselineManager.h
    fatigue/FatigueDecision.cpp
    fatigue/FatigueDecision.h
    # =====================
    # Photo模块
    # =====================


    photo/GalleryService.cpp

    photo/GalleryService.h

    # =====================
    # Audio
    # =====================

    audio/AudioInputService.cpp
    audio/AudioInputService.h

    # =====================
    # Weather
    # =====================

    weather/WeatherService.cpp
    weather/WeatherService.h


    # =====================
    # Qt控制层
    # =====================


    controller/CameraController.cpp

    controller/CameraController.h


    controller/CameraImageProvider.cpp

    controller/CameraImageProvider.h

    controller/GalleryController.cpp

    controller/GalleryController.h

    controller/OnboardController.cpp
    controller/OnboardController.h

    # =====================
    # AI Assistant
    # =====================


    assistant/VehicleAssistant.cpp

    assistant/VehicleAssistant.h

    # =====================
    # Conversation
    # =====================

    conversation/ConversationManager.cpp
    conversation/ConversationManager.h



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

    # Audio
    ${CMAKE_CURRENT_SOURCE_DIR}/audio

    ${CMAKE_CURRENT_SOURCE_DIR}


    # sherpa-onnx C API
    ${SHERPA_ONNX_ROOT}/include

    ${CMAKE_CURRENT_SOURCE_DIR}/include



    # AI

    ${CMAKE_CURRENT_SOURCE_DIR}/assistant

    # Conversation

    ${CMAKE_CURRENT_SOURCE_DIR}/conversation

    # LLM

    ${CMAKE_CURRENT_SOURCE_DIR}/llm

    # Fatigue

    ${CMAKE_CURRENT_SOURCE_DIR}/fatigue

    # RAG

    ${CMAKE_CURRENT_SOURCE_DIR}/rag



    # Camera

    ${CMAKE_CURRENT_SOURCE_DIR}/camera



    # Photo

    ${CMAKE_CURRENT_SOURCE_DIR}/photo

    # Weather

    ${CMAKE_CURRENT_SOURCE_DIR}/weather

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

    Qt5::DBus

    Qt5::Network

    # OpenCV

    ${OpenCV_LIBS}

    # ALSA
    asound

    # sherpa-onnx KWS
    ${SHERPA_ONNX_ROOT}/lib/libsherpa-onnx-c-api.so
    ${SHERPA_ONNX_ROOT}/lib/libonnxruntime.so

    # RKLLM

    ${CMAKE_CURRENT_SOURCE_DIR}/lib/librkllmrt.so

    # RKNN
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/librknnrt.so
)









# =========================
# RKLLM运行时搜索路径
# =========================


set_target_properties(

    VehicleTerminal

    PROPERTIES



    BUILD_RPATH

    "${CMAKE_CURRENT_SOURCE_DIR}/lib;${SHERPA_ONNX_ROOT}/lib"


)