#pragma once

#include "FatigueWindowAnalyzer.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>


// ============================================================
// 疲劳等级
// ============================================================

enum class FatigueLevel
{
    UNKNOWN = 0,

    NORMAL,

    FATIGUE,

    DEEP_FATIGUE
};


// ============================================================
// Decision 阶段
// ============================================================

enum class FatigueDecisionPhase
{
    LEARNING_BASELINE = 0,
    MONITORING
};


// ============================================================
// 判定原因
//
// reasonFlags 可以同时包含多个原因。
// ============================================================

enum FatigueReasonFlag : uint32_t
{
    FATIGUE_REASON_NONE =
        0,

    FATIGUE_REASON_PERCLOS_FATIGUE =
        1u << 0,

    FATIGUE_REASON_PERCLOS_TREND =
        1u << 1,

    FATIGUE_REASON_ECD =
        1u << 2,

    FATIGUE_REASON_LONG_CLOSE =
        1u << 3,

    FATIGUE_REASON_STRONG_LONG_CLOSE =
        1u << 4,

    FATIGUE_REASON_YAWN =
        1u << 5,

    FATIGUE_REASON_BLINK_DURATION =
        1u << 6,

    FATIGUE_REASON_PERCLOS_DEEP =
        1u << 7,

    FATIGUE_REASON_MULTI_EVIDENCE =
        1u << 8
};


// ============================================================
// Config
// ============================================================

struct FatigueDecisionConfig
{
    // --------------------------------------------------------
    // Window 质量
    // --------------------------------------------------------

    bool requireFullWindow = true;

    float minValidObservationRatio = 0.70f;


    // --------------------------------------------------------
    // 初始个人行为基准
    // --------------------------------------------------------

    std::size_t baselineWindowCount = 6;


    // --------------------------------------------------------
    // 最近历史
    // --------------------------------------------------------

    std::size_t historyWindowCount = 6;

    std::size_t trendGroupSize = 3;


    // ========================================================
    // PERCLOS
    // ========================================================

    // recent3 mean >= 5%
    // → FATIGUE 候选
    float fatiguePerclosThreshold =
        0.05f;


    // recent3 mean >= 15%
    // → DEEP_FATIGUE 候选
    float deepPerclosThreshold =
        0.15f;


    // 用于：
    //
    // previous3 → recent3
    //
    // 增加 >= 2 个百分点
    // 视为明显持续恶化。
    float perclosTrendDelta =
        0.02f;


    // LongClose 的辅助证据。
    //
    // 比正式的 0.02 趋势条件稍宽，
    // 这里只作为组合证据，不单独判疲劳。
    float perclosAuxTrendDelta =
        0.01f;


    // 多指标共同严重异常时使用。
    //
    // 10% 本身不直接等于 DEEP，
    // 必须同时出现长闭眼以及其他异常。
    float highPerclosForMultiEvidence =
        0.10f;


    // ========================================================
    // ECD
    // ========================================================

    // recent ECD 相对于个人 baseline
    // 增加 30% 以上，作为同步恶化证据。
    float ecdIncreaseRatio =
        1.30f;


    // ========================================================
    // Blink Duration
    // ========================================================

    // recent Blink Duration 相对于个人 baseline
    // 增加 30% 以上，作为疲劳辅助证据。
    float blinkDurationIncreaseRatio =
        1.30f;


    // ========================================================
    // Strong Long Close
    // ========================================================

    // 当前 60 秒窗口中至少出现 2 次
    // StrongLongClose：
    //
    // 再结合 PERCLOS / ECD / Blink 异常，
    // 可进入 DEEP_FATIGUE 候选。
    int repeatedStrongLongCloseCount =
        2;


    // ========================================================
    // 状态滞回
    // ========================================================

    // 连续多少轮候选等级高于当前等级才升级。
    int upgradeConfirmRounds =
        2;


    // 连续多少轮改善才允许降一级。
    int recoveryConfirmRounds =
        3;
};


// ============================================================
// 个人行为基准
//
// 注意：
// 这不是 FatigueBaselineManager 的眼睛几何基准。
//
// 这是 Decision 层的驾驶员行为参考。
// ============================================================

struct FatigueBehaviorBaseline
{
    bool ready = false;

    std::size_t sampleCount = 0;


    float perclos = 0.0f;


    float medianEcdMs = 0.0f;

    float typicalMaxEcdMs = 0.0f;


    float blinkRatePerMinute = 0.0f;


    // 普通 BLINK 的 totalDurationMs 中位数。
    float blinkDurationMs = 0.0f;


    float longCloseCount = 0.0f;

    float strongLongCloseCount = 0.0f;

    float yawnCount = 0.0f;
};


// ============================================================
// 趋势数据
// ============================================================

struct FatigueTrendMetrics
{
    // 最近 N 轮是否已经存在。
    //
    // 默认 N=3。
    bool recentGroupReady = false;


    // 是否已经可以：
    //
    // previous3 VS recent3
    bool enoughHistory = false;


    // ========================================================
    // 当前窗口 VS 个人 baseline
    // ========================================================

    float perclosDeltaFromBaseline =
        0.0f;


    float medianEcdDeltaFromBaselineMs =
        0.0f;


    float maxEcdDeltaFromBaselineMs =
        0.0f;


    float blinkRateDeltaFromBaseline =
        0.0f;


    float blinkDurationDeltaFromBaselineMs =
        0.0f;


    float longCloseDeltaFromBaseline =
        0.0f;


    float strongLongCloseDeltaFromBaseline =
        0.0f;


    float yawnDeltaFromBaseline =
        0.0f;


    // ========================================================
    // Recent group
    // ========================================================

    float recentPerclosMean =
        0.0f;


    float recentMedianEcdMean =
        0.0f;


    float recentMaxEcdMean =
        0.0f;


    float recentBlinkRateMean =
        0.0f;


    float recentBlinkDurationMean =
        0.0f;


    float recentLongCloseMean =
        0.0f;


    float recentStrongLongCloseMean =
        0.0f;


    float recentYawnMean =
        0.0f;


    // 最近3轮中：
    //
    // PERCLOS >= deepPerclosThreshold
    //
    // 的窗口数量。
    int recentHighPerclosCount =
        0;


    // ========================================================
    // Previous group
    // ========================================================

    float previousPerclosMean =
        0.0f;


    float previousMedianEcdMean =
        0.0f;


    float previousMaxEcdMean =
        0.0f;


    float previousBlinkRateMean =
        0.0f;


    float previousBlinkDurationMean =
        0.0f;


    float previousLongCloseMean =
        0.0f;


    float previousStrongLongCloseMean =
        0.0f;


    float previousYawnMean =
        0.0f;


    // ========================================================
    // recent - previous
    // ========================================================

    float perclosRecentDelta =
        0.0f;


    float medianEcdRecentDeltaMs =
        0.0f;


    float maxEcdRecentDeltaMs =
        0.0f;


    float blinkRateRecentDelta =
        0.0f;


    float blinkDurationRecentDeltaMs =
        0.0f;


    float longCloseRecentDelta =
        0.0f;


    float strongLongCloseRecentDelta =
        0.0f;


    float yawnRecentDelta =
        0.0f;
};


// ============================================================
// 输出
// ============================================================

struct FatigueDecisionResult
{
    bool ready = false;

    bool baselineReady = false;


    FatigueDecisionPhase phase =
        FatigueDecisionPhase::LEARNING_BASELINE;


    // 已经经过滞回确认后的最终等级。
    FatigueLevel level =
        FatigueLevel::UNKNOWN;


    // 当前这一轮原始计算出的候选等级。
    //
    // candidateLevel 可能先变，
    // level 只有连续满足确认条件后才改变。
    FatigueLevel candidateLevel =
        FatigueLevel::UNKNOWN;


    std::size_t baselineSamplesCollected =
        0;


    std::size_t historySize =
        0;


    FatigueBehaviorBaseline baseline;

    FatigueTrendMetrics trend;


    int worseningStreak = 0;

    int recoveringStreak = 0;


    uint32_t reasonFlags =
        FATIGUE_REASON_NONE;
};


// ============================================================
// FatigueDecision
// ============================================================

class FatigueDecision
{
public:

    explicit FatigueDecision(
        const FatigueDecisionConfig& config =
            FatigueDecisionConfig()
    );


    void setConfig(
        const FatigueDecisionConfig& config
    );


    const FatigueDecisionConfig&
    config() const;


    void reset();


    FatigueDecisionResult evaluate(
        const FatigueWindowStats& stats
    );


    const FatigueBehaviorBaseline&
    baseline() const;


private:

    bool statsQualified(
        const FatigueWindowStats& stats
    ) const;


    void buildBehaviorBaseline();


    void pushHistory(
        const FatigueWindowStats& stats
    );


    FatigueTrendMetrics buildTrend(
        const FatigueWindowStats& current
    ) const;


    void applyCandidateLevel(
        FatigueLevel candidate
    );


    FatigueDecisionResult makeResult(
        FatigueDecisionPhase phase,
        bool ready,
        FatigueLevel candidateLevel,
        uint32_t reasonFlags,
        const FatigueTrendMetrics& trend
    ) const;


private:

    FatigueDecisionConfig config_;


    std::vector<FatigueWindowStats>
        baselineSamples_;


    FatigueBehaviorBaseline
        baseline_;


    std::deque<FatigueWindowStats>
        history_;


    FatigueLevel currentLevel_ =
        FatigueLevel::UNKNOWN;


    int worseningStreak_ = 0;

    int recoveringStreak_ = 0;
};


// ============================================================
// Debug names
// ============================================================

const char* fatigueLevelName(
    FatigueLevel level
);


const char* fatigueDecisionPhaseName(
    FatigueDecisionPhase phase
);