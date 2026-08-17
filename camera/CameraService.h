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
     * 启动底层摄像头。
     *
     * 可以重复调用：
     * 已经运行时直接返回 true。
     */
    bool start(
        const std::string& device,
        int width,
        int height
    );


    /*
     * 停止底层摄像头。
     *
     * 可以重复调用。
     */
    void stop();


    /*
     * 查询底层 CameraService
     * 当前是否真正运行。
     */
    bool isRunning() const;


    /*
     * 获取最新 BGR 帧。
     */
    cv::Mat getFrame();


    /*
     * 获取当前帧用于拍照。
     */
    cv::Mat capture();


private:

    bool set_format();

    bool init_mmap();

    void captureLoop();


    /*
     * 释放 fd / mmap 等底层资源。
     *
     * 调用时采集线程必须已经退出。
     */
    void releaseResources();


private:

    int fd_;

    int width_;

    int height_;


    struct Buffer
    {
        void* start = nullptr;

        size_t length = 0;
    };


    std::vector<Buffer> buffers_;


    std::thread captureThread_;


    std::mutex frameMutex_;


    /*
     * 防止 start / stop 同时执行。
     */
    std::mutex lifecycleMutex_;


    cv::Mat latestFrame_;


    /*
     * captureLoop 是否继续运行。
     */
    std::atomic<bool> running_;


    /*
     * V4L2 STREAMON 是否成功。
     */
    std::atomic<bool> streaming_;
};