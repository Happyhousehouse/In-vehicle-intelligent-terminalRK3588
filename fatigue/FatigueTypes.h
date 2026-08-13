#pragma once

#include <cstdint>

// ============================================================
// 眼睛状态
// ============================================================

enum class EyeState
{
    UNKNOWN = 0,
    OPEN,
    CLOSED
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
//
// OPEN -> CLOSED -> OPEN
// ============================================================

enum class EyeClosureType
{
    NONE = 0,

    // 普通眨眼
    BLINK,

    // 长闭眼
    LONG_CLOSE,

    // 强长闭眼
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

    // 双眼同时低于该值，认为 CLOSED
    float eyeCloseThreshold = 0.18f;

    // 至少一只眼明显高于该值，
    // 就不认为是“双眼闭合”
    float eyeOpenThreshold = 0.22f;

    // --------------------------------------------------------
    // Mouth Ratio
    // --------------------------------------------------------

    float mouthOpenThreshold = 0.70f;

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

    // >= 1000 ms
    int64_t strongLongCloseMs = 1000;

    // --------------------------------------------------------
    // Yawn
    // --------------------------------------------------------

    // 持续张嘴至少 1500 ms，
    // 完成 OPEN -> CLOSED 后才生成一次哈欠事件
    int64_t yawnMinDurationMs = 1500;
};

// ============================================================
// 每一帧输入给 FatigueEventTracker 的数据
// ============================================================

struct FatigueFrameInput
{
    float leftEyeRatio = 0.0f;
    float rightEyeRatio = 0.0f;

    float mouthRatio = 0.0f;

    // 单调递增时间，单位 ms
    int64_t timestampMs = 0;

    // Face/PFLD 当前帧是否有效
    bool valid = true;
};

// ============================================================
// 一次完整闭眼事件
// ============================================================

struct EyeClosureEvent
{
    // 本帧是否刚完成 CLOSED -> OPEN
    bool completed = false;

    EyeClosureType type = EyeClosureType::NONE;

    int64_t startMs = 0;
    int64_t durationMs = 0;
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
    bool valid = false;

    EyeState eyeState = EyeState::UNKNOWN;
    MouthState mouthState = MouthState::UNKNOWN;

    // 如果当前正在闭眼：
    // timestamp - closeStart
    //
    // 以后 Emergency 实时检测就使用这个值
    int64_t currentCloseMs = 0;

    // 当前持续张嘴时间
    int64_t currentMouthOpenMs = 0;

    // 本帧刚完成的闭眼事件
    EyeClosureEvent eyeClosure;

    // 本帧刚完成的哈欠事件
    YawnEvent yawn;
};