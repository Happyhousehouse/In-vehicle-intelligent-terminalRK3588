#pragma once

#include "FatigueTypes.h"


// ============================================================
// FatigueEventTracker
//
// 输入：
// Baseline 归一化后的左右眼 openness
//
// 正常睁眼约：
// 1.0
//
// 状态：
// UNKNOWN
//   ↓
// OPEN
//   ↓
// CLOSING
//   ↓
// CLOSED
//   ↓
// OPENING
//   ↓
// OPEN
//
// 当前阶段主要负责：
//
// 1. Eye openness EMA
// 2. Eye state machine
// 3. Blink / LongClose / StrongLongClose
// 4. currentClosedDurationMs
// 5. Emergency >= 2000ms
//
// Mouth 暂时保留第一版简单逻辑。
// ============================================================

class FatigueEventTracker
{
public:

    explicit FatigueEventTracker(
        const FatigueEventConfig& config =
            FatigueEventConfig()
    );


    // ========================================================
    // 每处理一帧调用一次
    //
    // observation 中的：
    //
    // leftEyeOpenness
    // rightEyeOpenness
    //
    // 必须已经经过 Baseline 归一化。
    // ========================================================

    FatigueEventResult update(
        const FatigueObservation& observation
    );


    // 清除当前所有时序状态
    void reset();


    void setConfig(
        const FatigueEventConfig& config
    );


    const FatigueEventConfig& config() const;


private:

    // ========================================================
    // Eye helpers
    // ========================================================

    // 一次完整闭眼 episode 完成后，
    // 根据真正 CLOSED 的持续时间进行分类。
    EyeClosureType classifyClosure(
        int64_t closedDurationMs,
        int64_t totalDurationMs
    ) const;


    // 开始一个新的：
    //
    // OPEN -> CLOSING
    //
    // episode
    void beginEyeEpisode(
        int64_t timestampMs,
        float leftOpenness,
        float rightOpenness
    );


    // 更新当前 episode 中
    // 左右眼最低 openness
    void updateEyeEpisodeMinimum(
        float leftOpenness,
        float rightOpenness
    );


    // 取消当前正在进行的闭眼 episode。
    //
    // 例如：
    //
    // CLOSING 后迅速重新睁眼
    // invalid 超过 maxInvalidGapMs
    void cancelEyeEpisode();


    // Face / PFLD 长时间无效后，
    // 整个 Eye tracking 回 UNKNOWN。
    void invalidateEyeTracking();


    // ========================================================
    // Mouth helpers
    // ========================================================

    MouthState resolveMouthState(
        float mouthRatio
    ) const;


    void resetMouthTracking();


private:

    FatigueEventConfig config_;


    // ========================================================
    // Eye EMA
    // ========================================================

    bool eyeEmaInitialized_ = false;

    float smoothedLeftOpenness_ = 0.0f;

    float smoothedRightOpenness_ = 0.0f;


    // 上一帧 combined openness，
    // 用于计算 slope。
    float previousCombinedOpenness_ = 0.0f;


    // 上一个有效 Eye observation 的时间。
    //
    // 用于：
    // openness slope
    // invalid gap
    int64_t previousValidTimestampMs_ = -1;


    // ========================================================
    // Eye state machine
    // ========================================================

    EyeState eyeState_ =
        EyeState::UNKNOWN;


    // 当前 EyeState 从什么时候开始。
    //
    // 用于：
    // stateDurationMs
    int64_t eyeStateStartMs_ = -1;


    // ========================================================
    // Eye closure episode
    //
    // 一个 episode：
    //
    // CLOSING
    //   ↓
    // CLOSED
    //   ↓
    // OPENING
    //   ↓
    // OPEN
    // ========================================================

    bool eyeEpisodeActive_ = false;


    // CLOSING 开始时间
    int64_t eyeEpisodeStartMs_ = -1;


    // 真正进入 CLOSED 的时间
    int64_t eyeClosedStartMs_ = -1;


    // 真正离开 CLOSED 的时间
    int64_t eyeClosedEndMs_ = -1;


    // ========================================================
    // Confirmation timers
    // ========================================================

    // openness 已经低于 closedEnterRatio，
    // 但还在等待 closedConfirmMs。
    int64_t closedCandidateStartMs_ = -1;


    // OPENING 中已经满足 openReturnRatio，
    // 但还在等待 openConfirmMs。
    int64_t openCandidateStartMs_ = -1;


    // ========================================================
    // Episode minimum
    // ========================================================

    float minLeftOpenness_ = 0.0f;

    float minRightOpenness_ = 0.0f;


    // ========================================================
    // Emergency
    // ========================================================

    // 一次 CLOSED episode 中，
    // Emergency 只触发一次。
    bool emergencyLatched_ = false;

    // ============================================================
    // Prolonged Low Openness tracking
    //
    // 独立于 CLOSED / EyeClosureEvent。
    // ============================================================

    int64_t prolongedLowStartMs_ = -1;

    bool prolongedLowEmergencyLatched_ = false;
    // ========================================================
    // Invalid observation
    //
    // 短暂 No Face 不立即取消闭眼事件。
    //
    // invalid <= maxInvalidGapMs
    //     保留当前状态
    //
    // invalid > maxInvalidGapMs
    //     回 UNKNOWN
    // ========================================================

    int64_t invalidStartMs_ = -1;


    // ========================================================
    // Mouth tracking
    //
    // 目前仍保留原来的简单：
    //
    // CLOSED -> OPEN -> CLOSED
    //
    // 后面 Eye Tracker 验证完成后，
    // 再升级完整 Yawn state machine。
    // ========================================================

    MouthState mouthState_ =
        MouthState::UNKNOWN;


    bool mouthOpenActive_ = false;


    int64_t mouthOpenStartMs_ = -1;
    // 当前 5 秒窗口内已经累计了多少 ms 的“张嘴时间”
    int64_t mouthOpenAccumMs_ = 0;

    // 上一次有效 Mouth 观测时间
    int64_t mouthLastTimestampMs_ = -1;

    // 上一个时间段嘴巴是否 >= 0.80
    bool mouthWasOpen_ = false;

    float mouthPeakRatio_ = 0.0f;

    bool yawnLatched_ = false;
    // ========================================================
    // Global timestamp
    // ========================================================

    int64_t lastTimestampMs_ = -1;
};