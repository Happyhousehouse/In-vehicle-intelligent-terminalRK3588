#include "RKLLMWrapper.h"

#include <cstring>
#include <iostream>
#include <utility>


RKLLMWrapper::RKLLMWrapper()
{
    handle_ = nullptr;

    initialized_ = false;

    stop_flag_.store(false);
}


RKLLMWrapper::~RKLLMWrapper()
{
    /*
     * 正在运行时先停止。
     */
    if (
        initialized_
        &&
        handle_
        &&
        rkllm_is_running(handle_) == 1
    )
    {
        rkllm_abort(handle_);
    }

    if (initialized_ && handle_)
    {
        rkllm_destroy(handle_);

        handle_ = nullptr;

        initialized_ = false;
    }
}


bool RKLLMWrapper::init(
    const std::string &model_path
)
{
    RKLLMParam param =
        rkllm_createDefaultParam();

    param.model_path =
        model_path.c_str();

    param.max_context_len =
        4096;

    param.max_new_tokens =
        512;


    RKLLMCallback callback_struct;

    std::memset(
        &callback_struct,
        0,
        sizeof(callback_struct)
    );

    callback_struct.result_callback =
        RKLLMWrapper::callback;


    const int ret =
        rkllm_init(
            &handle_,
            &param,
            &callback_struct
        );

    if (ret != 0)
    {
        std::cout
            << "rkllm_init failed: "
            << ret
            << std::endl;

        handle_ = nullptr;

        initialized_ = false;

        return false;
    }

    initialized_ = true;

    return true;
}


bool RKLLMWrapper::isRunning() const
{
    if (!initialized_ || !handle_)
        return false;

    return
        rkllm_is_running(handle_) == 1;
}


void RKLLMWrapper::generate(
    const std::string &prompt,
    StreamCallback callback
)
{
    if (!initialized_ || !handle_)
    {
        std::cout
            << "RKLLM is not initialized"
            << std::endl;

        return;
    }

    /*
     * 第二层 runtime 防御。
     *
     * 即使上层 VehicleAssistant 出现逻辑错误，
     * 也不允许同一个 handle 并发调用。
     */
    if (rkllm_is_running(handle_) == 1)
    {
        std::cout
            << "RKLLM is already running, "
            << "reject new generate()"
            << std::endl;

        return;
    }

    stream_callback_ =
        std::move(callback);

    stop_flag_.store(false);


    RKLLMInput input;

    std::memset(
        &input,
        0,
        sizeof(input)
    );

    input.input_type =
        RKLLM_INPUT_PROMPT;

    input.role =
        "user";

    input.enable_thinking =
        false;

    /*
     * prompt 是 generate() 参数，
     * 而 rkllm_run 是同步 API。
     *
     * 所以 prompt.c_str() 在 rkllm_run()
     * 返回前一直有效。
     */
    input.prompt_input =
        prompt.c_str();


    RKLLMInferParam param;

    std::memset(
        &param,
        0,
        sizeof(param)
    );

    param.mode =
        RKLLM_INFER_GENERATE;

    /*
     * 第一阶段先保持原来的行为：
     * 不处理历史上下文。
     *
     * 后面实现“当前会话记忆”时，
     * 这里会正式修改为 keep_history = 1。
     */
    param.keep_history = 1;

    param.max_new_tokens = 0;


    const int ret =
        rkllm_run(
            handle_,
            &input,
            &param,
            this
        );

    if (ret != 0)
    {
        std::cout
            << "rkllm_run returned: "
            << ret
            << std::endl;
    }

    /*
     * rkllm_run 是同步函数。
     *
     * 到这里 callback 已经不再需要
     * 当前 stream_callback_。
     */
    stream_callback_ = nullptr;
}


void RKLLMWrapper::stop()
{
    stop_flag_.store(true);

    if (!initialized_ || !handle_)
        return;

    /*
     * 只有真正正在运行时才 abort。
     */
    if (rkllm_is_running(handle_) != 1)
    {
        return;
    }

    const int ret =
        rkllm_abort(handle_);

    if (ret != 0)
    {
        std::cout
            << "rkllm_abort failed: "
            << ret
            << std::endl;
    }
}


int RKLLMWrapper::callback(
    RKLLMResult *result,
    void *userdata,
    LLMCallState state
)
{
    auto *self =
        static_cast<RKLLMWrapper *>(
            userdata
        );

    if (!self)
        return 0;

    /*
     * 用户已经请求 stop。
     *
     * 注意：
     *
     * 不再 return -1。
     *
     * 当前 SDK 文档定义的 callback
     * 返回值只有：
     *
     * 0 = continue
     * 1 = pause
     * 2 = release
     *
     * abort 已由 rkllm_abort() 完成。
     */
    if (self->stop_flag_.load())
    {
        return 0;
    }


    switch (state)
    {
        case RKLLM_RUN_NORMAL:
        {
            if (
                result
                &&
                result->text
                &&
                self->stream_callback_
            )
            {
                self->stream_callback_(
                    result->text
                );
            }

            break;
        }


        case RKLLM_RUN_WAITING:
        {
            /*
             * 等待完整 UTF-8 字符。
             * 不做处理。
             */
            break;
        }


        case RKLLM_RUN_FINISH:
        {
            if (self->stream_callback_)
            {
                self->stream_callback_(
                    "\n"
                );
            }

            break;
        }


        case RKLLM_RUN_ERROR:
        {
            std::cerr
                << "RKLLM_RUN_ERROR"
                << std::endl;

            break;
        }


        default:
        {
            break;
        }
    }

    return 0;
}

bool RKLLMWrapper::clearHistory(
    bool keepSystemPrompt
)
{
    std::cout
        << "[RKLLMWrapper] "
        << "★ clearHistory entered"
        << std::endl;


    if (!initialized_ || !handle_)
    {
        std::cerr
            << "[RKLLMWrapper] "
            << "★ model not initialized"
            << std::endl;

        return false;
    }


    const int running =
        rkllm_is_running(handle_);


    std::cout
        << "[RKLLMWrapper] "
        << "★ rkllm_is_running = "
        << running
        << std::endl;


    if (running == 1)
    {
        std::cerr
            << "[RKLLMWrapper] "
            << "★ clearHistory rejected: running"
            << std::endl;

        return false;
    }


    const int ret =
        rkllm_clear_kv_cache(
            handle_,
            keepSystemPrompt ? 1 : 0,
            nullptr,
            nullptr
        );


    std::cout
        << "[RKLLMWrapper] "
        << "★ rkllm_clear_kv_cache ret = "
        << ret
        << std::endl;


    if (ret != 0)
    {
        return false;
    }


    stream_callback_ = nullptr;

    stop_flag_.store(false);


    std::cout
        << "[RKLLMWrapper] "
        << "★ conversation history cleared"
        << std::endl;


    return true;
}