#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include <opencv2/opencv.hpp>

#include "rknn_api.h"


// ============================================================
// FatigueVision 配置
// ============================================================

struct FatigueVisionConfig
{
    std::string faceModelPath =
        "/home/firefly/VehicleTerminal/model/fatigue/"
        "version-slim-320_rk3588.rknn";

    std::string pfldModelPath =
        "/home/firefly/VehicleTerminal/model/fatigue/"
        "PFLD_GhostOne_112_1_opt_sim_rk3588.rknn";

    // Face detector 输入
    int faceInputWidth = 320;
    int faceInputHeight = 240;

    // PFLD 输入
    int pfldInputWidth = 112;
    int pfldInputHeight = 112;

    // Face confidence
    float faceScoreThreshold = 0.70f;

    // NMS
    float faceNmsThreshold = 0.30f;
};


// ============================================================
// FatigueVision 每帧输出
// ============================================================

struct FatigueVisionResult
{
    bool valid = false;

    // ------------------------
    // Face
    // ------------------------

    bool faceDetected = false;

    cv::Rect faceRect;

    float faceScore = 0.0f;

    // ------------------------
    // PFLD
    // ------------------------

    bool landmarksValid = false;

    std::vector<cv::Point2f> landmarks;

    // ------------------------
    // Fatigue geometric features
    // ------------------------

    float leftEyeRatio = 0.0f;

    float rightEyeRatio = 0.0f;

    float mouthRatio = 0.0f;
};


// ============================================================
// FatigueVision
//
// 职责：
//
// cv::Mat BGR
//      ↓
// Face RKNN
//      ↓
// PFLD RKNN
//      ↓
// 98 landmarks
//      ↓
// Eye Ratio / Mouth Ratio
//
// 不负责：
// Blink / ECD / PERCLOS / Fatigue Level
// ============================================================

class FatigueVision
{
public:

    explicit FatigueVision(
        const FatigueVisionConfig& config =
            FatigueVisionConfig()
    );

    ~FatigueVision();


    // 初始化 Face + PFLD RKNN
    bool initialize();


    // 释放 RKNN
    void release();


    bool isInitialized() const;


    // 后面正式实现
    FatigueVisionResult process(
        const cv::Mat& bgrFrame
    );


private:

    // 读取 .rknn 文件
    bool loadModelFile(
        const std::string& path,
        std::vector<unsigned char>& data
    );


    // 初始化 Face RKNN
    bool initFaceModel();


    // 初始化 PFLD RKNN
    bool initPfldModel();


    // 打印模型输入输出信息
    void printModelInfo(
        const char* name,
        rknn_context ctx
    );

    // ============================================================
    // Face detection
    // ============================================================

    // 运行 Face RKNN
    bool runFace(
        const cv::Mat& bgrFrame,
        cv::Rect& faceRect,
        float& faceScore
    );

    // 两个矩形的 IoU
    static float boxIoU(
        const cv::Rect2f& a,
        const cv::Rect2f& b
    );

    // ============================================================
    // PFLD landmark detection
    // ============================================================

    // 在已经检测到的人脸区域中运行 PFLD，
    // 输出原始 Camera frame 坐标系下的 98 个关键点。
    bool runPfld(
        const cv::Mat& bgrFrame,
        const cv::Rect& faceRect,
        std::vector<cv::Point2f>& landmarks
    );

    static float pointDistance(
        const cv::Point2f& a,
        const cv::Point2f& b
    );

    static float calculateEyeRatio(
        const std::vector<cv::Point2f>& landmarks,
        int startIndex
    );

    static float calculateMouthRatio(
        const std::vector<cv::Point2f>& landmarks
    );

private:

    FatigueVisionConfig config_;


    // ========================================================
    // Face RKNN
    // ========================================================

    rknn_context faceCtx_ = 0;

    std::vector<unsigned char> faceModelData_;

    bool faceInitialized_ = false;


    // ========================================================
    // PFLD RKNN
    // ========================================================

    rknn_context pfldCtx_ = 0;

    std::vector<unsigned char> pfldModelData_;

    bool pfldInitialized_ = false;


    bool initialized_ = false;
};