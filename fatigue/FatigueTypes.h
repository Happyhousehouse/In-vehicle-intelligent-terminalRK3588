#pragma once

#include <cstdint>

// ============================================================
// 眼睛时序状态
// ============================================================

enum class EyeState
{
    UNKNOWN = 0,
    OPEN,
    CLOSING,
    CLOSED,
    OPENING
};

// ============================================================
// 嘴巴状态
// ============================================================

enum class MouthState
{
    UNKNOWN = 0,
    CLOSED,
    OPEN
};

// ============================================================
// 一次闭眼事件类型
// ============================================================

enum class EyeClosureType
{
    NONE = 0,

    // 正常快速眨眼
    BLINK,

    // ECD 本身不长，但完整眼睑动作异常缓慢
    SLOW_CLOSURE,

    // ECD >= longCloseMs
    LONG_CLOSE,

    // ECD >= strongLongCloseMs
    STRONG_LONG_CLOSE
};

// ============================================================
// EventTracker 第一阶段配置
//
// 目前阈值只是第一版工程参数。
// 后续加入个人 EyeOpenRatio 基准以后，
// Eye Ratio 阈值会进一步个性化。
// ============================================================

struct FatigueEventConfig
{
    // --------------------------------------------------------
    // Eye Ratio
    // --------------------------------------------------------

    // // 双眼同时低于该值，认为 CLOSED
    // float eyeCloseThreshold = 0.18f;

    // // 至少一只眼明显高于该值，
    // // 就不认为是“双眼闭合”
    // float eyeOpenThreshold = 0.22f;

    // --------------------------------------------------------
    // Mouth Ratio
    // --------------------------------------------------------

    // ============================================================
    // Normalized Eye Openness
    // ============================================================

    // EMA
    float emaAlpha = 0.65f;

    // OPEN 开始离开的边界
    float openExitRatio = 0.75f;

    // CLOSED 进入边界
    float closedEnterRatio = 0.45f;

    // CLOSED 开始退出边界
    float closedExitRatio = 0.55f;

    // 完全恢复 OPEN
    float openReturnRatio = 0.80f;

    // 每秒 openness 变化速度
    float closingSlopePerSecond = -0.25f;
    float openingSlopePerSecond =  0.25f;

    // CLOSED 必须持续确认，避免一帧尖峰
    int64_t closedConfirmMs = 50;

    // OPEN 恢复确认
    int64_t openConfirmMs = 80;

    // 短时 Face/PFLD 丢失允许保留 episode
    int64_t maxInvalidGapMs = 500;




    float mouthOpenThreshold = 0.8f;

    int64_t yawnWindowMs = 5000;

    float yawnOpenFraction = 0.7f;

    // 张嘴之后降到该值以下才认为真正闭嘴
    // 与 openThreshold 形成滞回
    float mouthCloseThreshold = 0.55f;

    // --------------------------------------------------------
    // 闭眼事件时间
    // --------------------------------------------------------

    // 小于该值认为可能是关键点抖动
    int64_t minClosureMs = 80;

    // >= 500 ms
    int64_t longCloseMs = 500;
    int64_t maxBlinkTotalMs = 1500;
    // >= 1000 ms
    int64_t strongLongCloseMs = 1000;
    int64_t emergencyCloseMs = 2000;
    // --------------------------------------------------------
    // Yawn
    // --------------------------------------------------------

    // 持续张嘴至少 1500 ms，
    // 完成 OPEN -> CLOSED 后才生成一次哈欠事件
    int64_t yawnMinDurationMs = 4000;

    // ============================================================
    // Prolonged Low Openness Emergency
    //
    // 注意：这不是 CLOSED 阈值。
    //
    // CLOSED:
    //     <= closedEnterRatio (0.45)
    //     用于 Blink / ECD / LongClose
    //
    // Prolonged Low:
    //     <= prolongedLowEnterRatio (0.65)
    //     持续过久时用于紧急疲劳报警
    // ============================================================

    float prolongedLowEnterRatio = 0.65f;

    // 滞回退出阈值。
    // 进入 low 后，不要求一超过 0.65 就立即清零，
    // 必须恢复到 >= 0.75 才认为真正脱离低开合状态。
    float prolongedLowExitRatio = 0.75f;

    int64_t prolongedLowEmergencyMs = 2000;


    
    // ============================================================
    // 基础疲劳警告
    //
    // 只出现 CLOSING：
    // 持续 4 秒警告。
    //
    // 当前 episode 一旦真正出现 CLOSED：
    // 整个 episode 的警告门槛缩短到 2 秒。
    // ============================================================

    int64_t closingWarningMs = 4000;

    int64_t closedWarningMs = 2000;
};

// ============================================================
// 每一帧输入给 FatigueEventTracker 的数据
// ============================================================

struct FatigueObservation
{
    bool valid = false;

    // 已经经过个人 Baseline 归一化
    // 正常睁眼约为 1.0
    float leftEyeOpenness = 0.0f;
    float rightEyeOpenness = 0.0f;

    // Mouth 暂时仍使用原始几何 Ratio
    float mouthRatio = 0.0f;

    int64_t timestampMs = 0;
};

// ============================================================
// 一次完整闭眼事件
// ============================================================

struct EyeClosureEvent
{
    bool completed = false;

    EyeClosureType type =
        EyeClosureType::NONE;

    // CLOSING 开始
    int64_t startMs = 0;

    // 真正进入 CLOSED
    int64_t closedStartMs = 0;

    // 真正离开 CLOSED
    int64_t closedEndMs = 0;

    // OPENING -> OPEN 完成
    int64_t endMs = 0;

    // CLOSED 持续时间
    // 后面 ECD 主要使用它
    int64_t closedDurationMs = 0;

    // CLOSING + CLOSED + OPENING
    int64_t totalDurationMs = 0;

    // 本次 episode 中的最低 openness
    float minLeftOpenness = 0.0f;
    float minRightOpenness = 0.0f;
};

// ============================================================
// 一次完整哈欠事件
// ============================================================

struct YawnEvent
{
    bool detected = false;

    int64_t startMs = 0;
    int64_t durationMs = 0;

    // 本次张嘴过程中的最大 Mouth Ratio
    float peakMouthRatio = 0.0f;
};

// ============================================================
// EventTracker 每帧输出
// ============================================================

struct FatigueEventResult
{
    bool observationValid = false;

    EyeState eyeState =
        EyeState::UNKNOWN;

    // EMA 后
    float smoothedLeftOpenness = 0.0f;
    float smoothedRightOpenness = 0.0f;

    // 第一版：
    // max(left, right)
    float combinedOpenness = 0.0f;

    // openness / second
    float opennessSlopePerSecond = 0.0f;

    // 当前 EyeState 已经持续多久
    int64_t stateDurationMs = 0;

    // 当前连续 CLOSED 时间
    int64_t currentClosedDurationMs = 0;

    int64_t currentLowOpennessDurationMs = 0;

    // 当前这一轮
    // CLOSING / CLOSED / OPENING
    // 已经持续多久。
    int64_t currentEyeEpisodeDurationMs = 0;

    EyeClosureEvent eyeClosure;

    // 只在跨过 emergencyCloseMs 时触发
    bool emergencyTriggered = false;


    // ========================================================
    // Mouth 暂时保留现有第一版
    // ========================================================

    MouthState mouthState =
        MouthState::UNKNOWN;

    int64_t currentMouthOpenMs = 0;



    YawnEvent yawn;
};