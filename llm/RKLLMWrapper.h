#pragma once


#include <string>
#include <atomic>
#include <functional>


#include "rkllm.h"



class RKLLMWrapper
{


public:


    using StreamCallback =
        std::function<void(const std::string&)>;



public:


    RKLLMWrapper();


    ~RKLLMWrapper();



    /*
        初始化模型
    */
    bool init(
        const std::string& model_path
    );



    /*
        流式生成

        每生成一个token
        调用一次callback
    */
    void generate(
        const std::string& prompt,
        StreamCallback callback
    );



    /*
        停止生成
    */
    void stop();



private:


    /*
        RKLLM回调
    */
    static int callback(
        RKLLMResult* result,
        void* userdata,
        LLMCallState state
    );



private:


    LLMHandle handle_;


    bool initialized_;


    std::atomic<bool> stop_flag_;


    StreamCallback stream_callback_;

};
