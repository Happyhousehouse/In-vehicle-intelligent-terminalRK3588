#pragma once

#include "FatigueEventTracker.h"

#include <cstdint>
#include <deque>
#include <vector>


// ============================================================
// FatigueWindowAnalyzerConfig
//
// 当前第一版：
//
// windowMs
//     统计最近多长时间。
//     默认 60 秒。
//
// reportIntervalMs
//     多久输出一次新的 WindowStats。
//     默认每 10 秒输出一次。
// ============================================================

struct FatigueWindowAnalyzerConfig
{
    int64_t windowMs = 60000;
    int64_t reportIntervalMs = 10000;

    // Blink Duration 只统计真正的 BLINK 事件。
    //
    // 这里沿用项目中已有的 1500ms blink 上限概念，
    // 防止极长 CLOSING episode 被误当成普通眨眼持续时间样本。
    //
    // 注意：
    // 这只影响 Blink Duration 统计，
    // 不改变 blinkCount，也不修改 EventTracker 分类。
    int64_t maxBlinkDurationForStatsMs = 1500;
};


// ============================================================
// FatigueWindowStats
//
// 这里只负责描述“当前时间窗口里发生了什么”。
// 暂时完全不做 NORMAL / FATIGUE 等疲劳等级判断。
// ============================================================

struct FatigueWindowStats
{
    // --------------------------------------------------------
    // Window 时间
    // --------------------------------------------------------

    int64_t startMs = 0;

    int64_t endMs = 0;

    int64_t durationMs = 0;


    // --------------------------------------------------------
    // 有效视觉观测
    // --------------------------------------------------------

    int64_t validObservationMs = 0;

    float validObservationRatio = 0.0f;


    // --------------------------------------------------------
    // CLOSED / PERCLOS
    // --------------------------------------------------------

    int64_t closedObservationMs = 0;

    float perclos = 0.0f;


    int blinkCount = 0;
    float blinkRatePerMinute = 0.0f;


    // ========================================================
    // Blink Duration
    //
    // 只统计 EyeClosureType::BLINK 的 totalDurationMs。
    // ========================================================

    float medianBlinkDurationMs = 0.0f;

    int blinkDurationSampleCount = 0;


    // ========================================================
    // ECD
    // ========================================================

    float medianEcdMs = 0.0f;

    int64_t maxEcdMs = 0;


    // --------------------------------------------------------
    // 长闭眼
    // --------------------------------------------------------

    int longCloseCount = 0;

    int strongLongCloseCount = 0;


    // --------------------------------------------------------
    // Yawn
    // --------------------------------------------------------

    int yawnCount = 0;


    // --------------------------------------------------------
    // 是否已经积累完整 60 秒窗口
    //
    // 启动后的前 60 秒仍然可以输出调试统计，
    // 但 fullWindow = false。
    // --------------------------------------------------------

    bool fullWindow = false;
};


// ============================================================
// FatigueWindowAnalyzer
// ============================================================

class FatigueWindowAnalyzer
{
public:

    explicit FatigueWindowAnalyzer(
        const FatigueWindowAnalyzerConfig& config =
            FatigueWindowAnalyzerConfig()
    );


    // --------------------------------------------------------
    // Config
    // --------------------------------------------------------

    void setConfig(
        const FatigueWindowAnalyzerConfig& config
    );

    const FatigueWindowAnalyzerConfig&
    config() const;


    // --------------------------------------------------------
    // Reset
    // --------------------------------------------------------

    void reset();


    // --------------------------------------------------------
    // Update
    //
    // 每一帧 EventTracker 更新完成以后调用一次。
    //
    // 返回：
    //
    // true
    //     到达了新的 reportInterval，
    //     outStats 中得到最新窗口统计。
    //
    // false
    //     当前还不到下一次 10 秒输出时间。
    // --------------------------------------------------------

    bool update(
        const FatigueEventResult& result,
        int64_t timestampMs,
        FatigueWindowStats& outStats
    );


    // --------------------------------------------------------
    // 不考虑 10 秒输出周期，
    // 随时计算当前窗口统计。
    // --------------------------------------------------------

    FatigueWindowStats currentStats(
        int64_t timestampMs
    ) const;


private:

    // ========================================================
    // ObservationInterval
    //
    // 保存两个相邻采样之间的时间段。
    //
    // 例如：
    //
    // t0 = 1000ms
    // t1 = 1033ms
    //
    // 如果 t0 那一帧 observationValid=true，
    // 那么 [1000,1033) 作为有效观测时间。
    //
    // 如果 t0 EyeState=CLOSED，
    // 那么这 33ms 同时计入 CLOSED 时间。
    // ========================================================

    struct ObservationInterval
    {
        int64_t startMs = 0;

        int64_t endMs = 0;

        bool valid = false;

        bool closed = false;
    };


    // ========================================================
    // EyeEventRecord
    // ========================================================

    struct EyeEventRecord
    {
        int64_t timestampMs = 0;

        EyeClosureType type =
            EyeClosureType::NONE;

        int64_t closedDurationMs = 0;
        int64_t totalDurationMs = 0;
    };


    // ========================================================
    // YawnRecord
    // ========================================================

    struct YawnRecord
    {
        int64_t timestampMs = 0;
    };


private:

    void pruneOldData(
        int64_t timestampMs
    );

    FatigueWindowStats buildStats(
        int64_t timestampMs
    ) const;


private:

    FatigueWindowAnalyzerConfig config_;


    // --------------------------------------------------------
    // Window data
    // --------------------------------------------------------

    std::deque<ObservationInterval>
        observationIntervals_;

    std::deque<EyeEventRecord>
        eyeEvents_;

    std::deque<YawnRecord>
        yawnEvents_;


    // --------------------------------------------------------
    // Previous frame
    // --------------------------------------------------------

    bool previousFrameInitialized_ = false;

    int64_t previousTimestampMs_ = -1;

    bool previousObservationValid_ = false;

    EyeState previousEyeState_ =
        EyeState::UNKNOWN;


    // --------------------------------------------------------
    // Session / report
    // --------------------------------------------------------

    int64_t sessionStartMs_ = -1;

    int64_t nextReportMs_ = -1;
};