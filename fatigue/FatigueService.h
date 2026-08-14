#pragma once

#include <atomic>
#include <thread>

#include "../camera/CameraService.h"

#include "FatigueVision.h"
#include "FatigueBaselineManager.h"
#include "FatigueEventTracker.h"

// ============================================================
// FatigueService
//
// 职责：
//
// CameraService
//      ↓
// 独立疲劳检测线程
//      ↓
// FatigueVision
//
// 当前阶段：
// 只验证 Face Detection
//
// 后续再加入：
// FatigueEventTracker
// FatigueWindowAnalyzer
// Baseline
// StateMachine
// ============================================================

class FatigueService
{
public:

    explicit FatigueService(
        CameraService* camera
    );

    ~FatigueService();


    // 初始化 RKNN
    bool initialize();


    // 启动疲劳检测线程
    bool start();


    // 停止疲劳检测
    void stop();


    bool isRunning() const;


private:

    void processLoop();


private:

    // 不拥有 CameraService
    // 只是使用 CameraController 中的 camera_
    CameraService* camera_ = nullptr;


    FatigueVision vision_;

    FatigueBaselineManager baseline_;
    
    FatigueEventTracker eventTracker_;

    std::thread workerThread_;


    std::atomic<bool> running_{false};


    bool initialized_ = false;
};