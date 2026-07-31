#include "RKLLMWrapper.h"


#include <iostream>
#include <cstring>



RKLLMWrapper::RKLLMWrapper()
{

    handle_ = nullptr;

    initialized_ = false;

    stop_flag_ = false;

}



RKLLMWrapper::~RKLLMWrapper()
{


    if(initialized_)
    {
        rkllm_destroy(handle_);
    }


}




bool RKLLMWrapper::init(
        const std::string& model_path)
{


    RKLLMParam param;


    param =
        rkllm_createDefaultParam();



    param.model_path =
        model_path.c_str();



    param.max_context_len =
        4096;



    param.max_new_tokens =
        512;



    RKLLMCallback callback_struct;


    memset(
        &callback_struct,
        0,
        sizeof(callback_struct)
    );



    /*
        你的SDK版本使用：
        result_callback
    */

    callback_struct.result_callback =
        RKLLMWrapper::callback;




    int ret =
        rkllm_init(
            &handle_,
            &param,
            &callback_struct
        );



    if(ret != 0)
    {

        std::cout
            <<"rkllm_init failed"
            <<std::endl;


        return false;

    }



    initialized_=true;



    return true;

}





void RKLLMWrapper::generate(
        const std::string& prompt,
        StreamCallback callback)
{


    if(!initialized_)
        return;



    stream_callback_ =
        callback;



    stop_flag_=false;



    RKLLMInput input;


    memset(
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
        注意：
        prompt生命周期必须覆盖rkllm_run
    */

    input.prompt_input =
        prompt.c_str();





    RKLLMInferParam param;


    memset(
        &param,
        0,
        sizeof(param)
    );



    param.mode =
        RKLLM_INFER_GENERATE;




    rkllm_run(
        handle_,
        &input,
        &param,
        this
    );


}




void RKLLMWrapper::stop()
{

    stop_flag_=true;

    if(handle_)
    {

        int ret =

        rkllm_abort(handle_);



        if(ret != 0)
        {

            std::cout
            <<"rkllm_abort failed:"
            <<ret
            <<std::endl;

        }

    }

}




int RKLLMWrapper::callback(
        RKLLMResult* result,
        void* userdata,
        LLMCallState state)
{


    RKLLMWrapper* self =
        static_cast<RKLLMWrapper*>(userdata);



    /*
        停止生成
    */
    if(self->stop_flag_)
    {
        return -1;
    }




    if(state == RKLLM_RUN_NORMAL)
    {


        if(result &&
           result->text)
        {


            if(self->stream_callback_)
            {


                self->stream_callback_(
                    result->text
                );


            }


        }


    }



    else if(state == RKLLM_RUN_FINISH)
    {


        if(self->stream_callback_)
        {

            self->stream_callback_(
                "\n"
            );

        }


    }



    return 0;

}
