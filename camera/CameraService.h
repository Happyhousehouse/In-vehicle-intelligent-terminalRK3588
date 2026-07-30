#pragma once


#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>


#include <linux/videodev2.h>

#include <opencv2/opencv.hpp>



class CameraService
{


public:


    CameraService();


    ~CameraService();



    /*
        打开摄像头并持续采集

        device:
            /dev/video11

    */
    bool start(
        const std::string& device,
        int width,
        int height
    );



    /*
        停止摄像头
    */
    void stop();



    /*
        获取最新一帧

        BGR格式

    */
    cv::Mat getFrame();



    /*
        拍照

        返回当前frame

    */
    cv::Mat capture();



private:


    bool set_format();


    bool init_mmap();



    void captureLoop();



private:


    int fd_;


    int width_;

    int height_;



    struct Buffer
    {

        void* start=nullptr;

        size_t length=0;

    };



    std::vector<Buffer> buffers_;



    std::thread captureThread_;



    std::mutex frameMutex_;



    cv::Mat latestFrame_;



    std::atomic<bool> running_;


};