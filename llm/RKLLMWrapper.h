#pragma once

#include <atomic>
#include <functional>
#include <string>

#include "rkllm.h"


class RKLLMWrapper
{
public:

    using StreamCallback =
        std::function<
            void(const std::string &)
        >;


public:

    RKLLMWrapper();

    ~RKLLMWrapper();


    bool init(
        const std::string &model_path
    );


    /*
     * 流式生成。
     */
    void generate(
        const std::string &prompt,
        StreamCallback callback
    );


    /*
     * 停止当前回答。
     */
    void stop();


    /*
     * 当前 RKLLM 是否正在运行。
     */
    bool isRunning() const;


    /*
     * 清除当前对话上下文。
     *
     * keepSystemPrompt = true：
     * 保留 system prompt，
     * 清除用户和 AI 的历史上下文。
     */
    bool clearHistory(
        bool keepSystemPrompt = true
    );


private:

    static int callback(
        RKLLMResult *result,
        void *userdata,
        LLMCallState state
    );


private:

    LLMHandle handle_ = nullptr;

    bool initialized_ = false;

    std::atomic<bool> stop_flag_{false};

    StreamCallback stream_callback_;
};