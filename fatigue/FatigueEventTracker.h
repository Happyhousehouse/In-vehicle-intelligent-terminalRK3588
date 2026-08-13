#pragma once

#include "FatigueTypes.h"

class FatigueEventTracker
{
public:
    explicit FatigueEventTracker(
        const FatigueEventConfig& config = FatigueEventConfig());

    // 每处理一帧调用一次
    FatigueEventResult update(
        const FatigueFrameInput& frame);

    // 清空当前时间状态
    void reset();

    void setConfig(
        const FatigueEventConfig& config);

    const FatigueEventConfig& config() const;

private:
    // 判断双眼当前状态
    EyeState resolveEyeState(
        float leftEyeRatio,
        float rightEyeRatio) const;

    // 判断嘴巴状态
    MouthState resolveMouthState(
        float mouthRatio) const;

    // 根据一次闭眼时间分类
    EyeClosureType classifyClosure(
        int64_t durationMs) const;

    // Face / PFLD 丢失时，
    // 中止当前正在统计的事件
    void invalidateTracking();

private:
    FatigueEventConfig config_;

    // ========================================================
    // Eye tracking
    // ========================================================

    EyeState eyeState_ = EyeState::UNKNOWN;

    bool eyeClosureActive_ = false;

    int64_t eyeCloseStartMs_ = 0;

    // ========================================================
    // Mouth tracking
    // ========================================================

    MouthState mouthState_ = MouthState::UNKNOWN;

    bool mouthOpenActive_ = false;

    int64_t mouthOpenStartMs_ = 0;

    float mouthPeakRatio_ = 0.0f;

    // ========================================================
    // Timestamp
    // ========================================================

    int64_t lastTimestampMs_ = -1;
};