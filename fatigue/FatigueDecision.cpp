#include "FatigueDecision.h"

#include <algorithm>


namespace
{

// ============================================================
// 配置保护
// ============================================================

void sanitizeConfig(
    FatigueDecisionConfig& config
)
{
    if (config.baselineWindowCount == 0)
    {
        config.baselineWindowCount = 1;
    }


    if (config.trendGroupSize == 0)
    {
        config.trendGroupSize = 1;
    }


    const std::size_t minHistory =
        config.trendGroupSize * 2;


    if (
        config.historyWindowCount <
        minHistory
    )
    {
        config.historyWindowCount =
            minHistory;
    }


    if (config.upgradeConfirmRounds < 1)
    {
        config.upgradeConfirmRounds = 1;
    }


    if (config.recoveryConfirmRounds < 1)
    {
        config.recoveryConfirmRounds = 1;
    }


    if (
        config.repeatedStrongLongCloseCount <
        1
    )
    {
        config.repeatedStrongLongCloseCount =
            1;
    }
}


// ============================================================
// median
// ============================================================

float medianValue(
    std::vector<float> values
)
{
    if (values.empty())
    {
        return 0.0f;
    }


    std::sort(
        values.begin(),
        values.end()
    );


    const std::size_t count =
        values.size();


    const std::size_t middle =
        count / 2;


    if ((count % 2) != 0)
    {
        return values[middle];
    }


    return
        (
            values[middle - 1] +
            values[middle]
        ) * 0.5f;
}


// ============================================================
// 普通平均
// ============================================================

template <typename Getter>
float meanRange(
    const std::deque<FatigueWindowStats>& history,
    std::size_t begin,
    std::size_t end,
    Getter getter
)
{
    if (
        begin >= end ||
        end > history.size()
    )
    {
        return 0.0f;
    }


    double sum = 0.0;


    for (
        std::size_t i = begin;
        i < end;
        ++i
    )
    {
        sum +=
            static_cast<double>(
                getter(history[i])
            );
    }


    return static_cast<float>(
        sum /
        static_cast<double>(
            end - begin
        )
    );
}


// ============================================================
// 只统计 > 0 的平均
//
// 用于：
// ECD
// Blink Duration
//
// 因为 0 往往代表“这个窗口没有样本”，
// 而不是实际持续时间为 0ms。
// ============================================================

template <typename Getter>
float positiveMeanRange(
    const std::deque<FatigueWindowStats>& history,
    std::size_t begin,
    std::size_t end,
    Getter getter
)
{
    if (
        begin >= end ||
        end > history.size()
    )
    {
        return 0.0f;
    }


    double sum = 0.0;

    std::size_t count = 0;


    for (
        std::size_t i = begin;
        i < end;
        ++i
    )
    {
        const float value =
            static_cast<float>(
                getter(history[i])
            );


        if (value > 0.0f)
        {
            sum += value;

            ++count;
        }
    }


    if (count == 0)
    {
        return 0.0f;
    }


    return static_cast<float>(
        sum /
        static_cast<double>(count)
    );
}


// ============================================================
// Level rank
// ============================================================

int levelRank(
    FatigueLevel level
)
{
    switch (level)
    {
        case FatigueLevel::NORMAL:
            return 1;

        case FatigueLevel::FATIGUE:
            return 2;

        case FatigueLevel::DEEP_FATIGUE:
            return 3;

        case FatigueLevel::UNKNOWN:
        default:
            return 0;
    }
}

} // namespace


// ============================================================
// Constructor
// ============================================================

FatigueDecision::FatigueDecision(
    const FatigueDecisionConfig& config
)
    : config_(config)
{
    sanitizeConfig(
        config_
    );


    reset();
}


// ============================================================
// Config
// ============================================================

void FatigueDecision::setConfig(
    const FatigueDecisionConfig& config
)
{
    config_ = config;


    sanitizeConfig(
        config_
    );


    reset();
}


const FatigueDecisionConfig&
FatigueDecision::config() const
{
    return config_;
}


// ============================================================
// Reset
// ============================================================

void FatigueDecision::reset()
{
    baselineSamples_.clear();


    baseline_ =
        FatigueBehaviorBaseline();


    history_.clear();


    currentLevel_ =
        FatigueLevel::UNKNOWN;


    worseningStreak_ = 0;

    recoveringStreak_ = 0;
}


// ============================================================
// Baseline
// ============================================================

const FatigueBehaviorBaseline&
FatigueDecision::baseline() const
{
    return baseline_;
}


// ============================================================
// Window 质量
// ============================================================

bool FatigueDecision::statsQualified(
    const FatigueWindowStats& stats
) const
{
    if (
        config_.requireFullWindow &&
        !stats.fullWindow
    )
    {
        return false;
    }


    if (
        stats.validObservationRatio <
        config_.minValidObservationRatio
    )
    {
        return false;
    }


    return true;
}


// ============================================================
// 建立个人行为 baseline
// ============================================================

void FatigueDecision::buildBehaviorBaseline()
{
    std::vector<float> perclosValues;

    std::vector<float> medianEcdValues;

    std::vector<float> maxEcdValues;

    std::vector<float> blinkRateValues;

    std::vector<float> blinkDurationValues;

    std::vector<float> longCloseValues;

    std::vector<float> strongLongCloseValues;

    std::vector<float> yawnValues;


    for (
        const FatigueWindowStats& stats :
        baselineSamples_
    )
    {
        perclosValues.push_back(
            stats.perclos
        );


        if (stats.medianEcdMs > 0.0f)
        {
            medianEcdValues.push_back(
                stats.medianEcdMs
            );
        }


        if (stats.maxEcdMs > 0)
        {
            maxEcdValues.push_back(
                static_cast<float>(
                    stats.maxEcdMs
                )
            );
        }


        blinkRateValues.push_back(
            stats.blinkRatePerMinute
        );


        // ----------------------------------------------------
        // Blink Duration baseline
        // ----------------------------------------------------

        if (
            stats.blinkDurationSampleCount > 0 &&
            stats.medianBlinkDurationMs > 0.0f
        )
        {
            blinkDurationValues.push_back(
                stats.medianBlinkDurationMs
            );
        }


        longCloseValues.push_back(
            static_cast<float>(
                stats.longCloseCount
            )
        );


        strongLongCloseValues.push_back(
            static_cast<float>(
                stats.strongLongCloseCount
            )
        );


        yawnValues.push_back(
            static_cast<float>(
                stats.yawnCount
            )
        );
    }


    baseline_.perclos =
        medianValue(
            perclosValues
        );


    baseline_.medianEcdMs =
        medianValue(
            medianEcdValues
        );


    baseline_.typicalMaxEcdMs =
        medianValue(
            maxEcdValues
        );


    baseline_.blinkRatePerMinute =
        medianValue(
            blinkRateValues
        );


    baseline_.blinkDurationMs =
        medianValue(
            blinkDurationValues
        );


    baseline_.longCloseCount =
        medianValue(
            longCloseValues
        );


    baseline_.strongLongCloseCount =
        medianValue(
            strongLongCloseValues
        );


    baseline_.yawnCount =
        medianValue(
            yawnValues
        );


    baseline_.sampleCount =
        baselineSamples_.size();


    baseline_.ready = true;
}


// ============================================================
// Push history
// ============================================================

void FatigueDecision::pushHistory(
    const FatigueWindowStats& stats
)
{
    history_.push_back(
        stats
    );


    while (
        history_.size() >
        config_.historyWindowCount
    )
    {
        history_.pop_front();
    }
}


// ============================================================
// Build trend
// ============================================================

FatigueTrendMetrics
FatigueDecision::buildTrend(
    const FatigueWindowStats& current
) const
{
    FatigueTrendMetrics trend;


    // ========================================================
    // Current VS baseline
    // ========================================================

    trend.perclosDeltaFromBaseline =
        current.perclos -
        baseline_.perclos;


    trend.medianEcdDeltaFromBaselineMs =
        current.medianEcdMs -
        baseline_.medianEcdMs;


    trend.maxEcdDeltaFromBaselineMs =
        static_cast<float>(
            current.maxEcdMs
        ) -
        baseline_.typicalMaxEcdMs;


    trend.blinkRateDeltaFromBaseline =
        current.blinkRatePerMinute -
        baseline_.blinkRatePerMinute;


    trend.blinkDurationDeltaFromBaselineMs =
        current.medianBlinkDurationMs -
        baseline_.blinkDurationMs;


    trend.longCloseDeltaFromBaseline =
        static_cast<float>(
            current.longCloseCount
        ) -
        baseline_.longCloseCount;


    trend.strongLongCloseDeltaFromBaseline =
        static_cast<float>(
            current.strongLongCloseCount
        ) -
        baseline_.strongLongCloseCount;


    trend.yawnDeltaFromBaseline =
        static_cast<float>(
            current.yawnCount
        ) -
        baseline_.yawnCount;


    const std::size_t groupSize =
        config_.trendGroupSize;


    if (
        history_.size() <
        groupSize
    )
    {
        return trend;
    }


    // ========================================================
    // Recent group
    // ========================================================

    trend.recentGroupReady = true;


    const std::size_t size =
        history_.size();


    const std::size_t recentBegin =
        size - groupSize;


    const std::size_t recentEnd =
        size;


    trend.recentPerclosMean =
        meanRange(
            history_,
            recentBegin,
            recentEnd,
            [](
                const FatigueWindowStats& s
            )
            {
                return s.perclos;
            }
        );


    trend.recentMedianEcdMean =
        positiveMeanRange(
            history_,
            recentBegin,
            recentEnd,
            [](
                const FatigueWindowStats& s
            )
            {
                return s.medianEcdMs;
            }
        );


    trend.recentMaxEcdMean =
        positiveMeanRange(
            history_,
            recentBegin,
            recentEnd,
            [](
                const FatigueWindowStats& s
            )
            {
                return static_cast<float>(
                    s.maxEcdMs
                );
            }
        );


    trend.recentBlinkRateMean =
        meanRange(
            history_,
            recentBegin,
            recentEnd,
            [](
                const FatigueWindowStats& s
            )
            {
                return
                    s.blinkRatePerMinute;
            }
        );


    trend.recentBlinkDurationMean =
        positiveMeanRange(
            history_,
            recentBegin,
            recentEnd,
            [](
                const FatigueWindowStats& s
            )
            {
                return
                    s.medianBlinkDurationMs;
            }
        );


    trend.recentLongCloseMean =
        meanRange(
            history_,
            recentBegin,
            recentEnd,
            [](
                const FatigueWindowStats& s
            )
            {
                return static_cast<float>(
                    s.longCloseCount
                );
            }
        );


    trend.recentStrongLongCloseMean =
        meanRange(
            history_,
            recentBegin,
            recentEnd,
            [](
                const FatigueWindowStats& s
            )
            {
                return static_cast<float>(
                    s.strongLongCloseCount
                );
            }
        );


    trend.recentYawnMean =
        meanRange(
            history_,
            recentBegin,
            recentEnd,
            [](
                const FatigueWindowStats& s
            )
            {
                return static_cast<float>(
                    s.yawnCount
                );
            }
        );


    // --------------------------------------------------------
    // 最近3轮中有几轮 PERCLOS >= 15%
    // --------------------------------------------------------

    for (
        std::size_t i = recentBegin;
        i < recentEnd;
        ++i
    )
    {
        if (
            history_[i].perclos >=
            config_.deepPerclosThreshold
        )
        {
            ++trend.recentHighPerclosCount;
        }
    }


    // ========================================================
    // Previous group
    // ========================================================

    const std::size_t required =
        groupSize * 2;


    if (
        history_.size() <
        required
    )
    {
        return trend;
    }


    trend.enoughHistory = true;


    const std::size_t previousBegin =
        recentBegin - groupSize;


    const std::size_t previousEnd =
        recentBegin;


    trend.previousPerclosMean =
        meanRange(
            history_,
            previousBegin,
            previousEnd,
            [](
                const FatigueWindowStats& s
            )
            {
                return s.perclos;
            }
        );


    trend.previousMedianEcdMean =
        positiveMeanRange(
            history_,
            previousBegin,
            previousEnd,
            [](
                const FatigueWindowStats& s
            )
            {
                return s.medianEcdMs;
            }
        );


    trend.previousMaxEcdMean =
        positiveMeanRange(
            history_,
            previousBegin,
            previousEnd,
            [](
                const FatigueWindowStats& s
            )
            {
                return static_cast<float>(
                    s.maxEcdMs
                );
            }
        );


    trend.previousBlinkRateMean =
        meanRange(
            history_,
            previousBegin,
            previousEnd,
            [](
                const FatigueWindowStats& s
            )
            {
                return
                    s.blinkRatePerMinute;
            }
        );


    trend.previousBlinkDurationMean =
        positiveMeanRange(
            history_,
            previousBegin,
            previousEnd,
            [](
                const FatigueWindowStats& s
            )
            {
                return
                    s.medianBlinkDurationMs;
            }
        );


    trend.previousLongCloseMean =
        meanRange(
            history_,
            previousBegin,
            previousEnd,
            [](
                const FatigueWindowStats& s
            )
            {
                return static_cast<float>(
                    s.longCloseCount
                );
            }
        );


    trend.previousStrongLongCloseMean =
        meanRange(
            history_,
            previousBegin,
            previousEnd,
            [](
                const FatigueWindowStats& s
            )
            {
                return static_cast<float>(
                    s.strongLongCloseCount
                );
            }
        );


    trend.previousYawnMean =
        meanRange(
            history_,
            previousBegin,
            previousEnd,
            [](
                const FatigueWindowStats& s
            )
            {
                return static_cast<float>(
                    s.yawnCount
                );
            }
        );


    // ========================================================
    // Delta
    // ========================================================

    trend.perclosRecentDelta =
        trend.recentPerclosMean -
        trend.previousPerclosMean;


    trend.medianEcdRecentDeltaMs =
        trend.recentMedianEcdMean -
        trend.previousMedianEcdMean;


    trend.maxEcdRecentDeltaMs =
        trend.recentMaxEcdMean -
        trend.previousMaxEcdMean;


    trend.blinkRateRecentDelta =
        trend.recentBlinkRateMean -
        trend.previousBlinkRateMean;


    trend.blinkDurationRecentDeltaMs =
        trend.recentBlinkDurationMean -
        trend.previousBlinkDurationMean;


    trend.longCloseRecentDelta =
        trend.recentLongCloseMean -
        trend.previousLongCloseMean;


    trend.strongLongCloseRecentDelta =
        trend.recentStrongLongCloseMean -
        trend.previousStrongLongCloseMean;


    trend.yawnRecentDelta =
        trend.recentYawnMean -
        trend.previousYawnMean;


    return trend;
}


// ============================================================
// 状态滞回
// ============================================================

void FatigueDecision::applyCandidateLevel(
    FatigueLevel candidate
)
{
    if (
        currentLevel_ ==
        FatigueLevel::UNKNOWN
    )
    {
        currentLevel_ =
            FatigueLevel::NORMAL;
    }


    const int currentRank =
        levelRank(
            currentLevel_
        );


    const int candidateRank =
        levelRank(
            candidate
        );


    // ========================================================
    // 候选等级比当前严重
    // ========================================================

    if (
        candidateRank >
        currentRank
    )
    {
        ++worseningStreak_;

        recoveringStreak_ = 0;


        if (
            worseningStreak_ >=
            config_.upgradeConfirmRounds
        )
        {
            currentLevel_ =
                candidate;


            worseningStreak_ = 0;

            recoveringStreak_ = 0;
        }


        return;
    }


    // ========================================================
    // 候选等级比当前轻
    // ========================================================

    if (
        candidateRank <
        currentRank
    )
    {
        ++recoveringStreak_;

        worseningStreak_ = 0;


        if (
            recoveringStreak_ >=
            config_.recoveryConfirmRounds
        )
        {
            // 恢复时一次只降一级。
            //
            // DEEP → FATIGUE
            // FATIGUE → NORMAL

            if (
                currentLevel_ ==
                FatigueLevel::DEEP_FATIGUE
            )
            {
                currentLevel_ =
                    FatigueLevel::FATIGUE;
            }
            else
            {
                currentLevel_ =
                    FatigueLevel::NORMAL;
            }


            recoveringStreak_ = 0;

            worseningStreak_ = 0;
        }


        return;
    }


    // ========================================================
    // 当前状态一致
    // ========================================================

    worseningStreak_ = 0;

    recoveringStreak_ = 0;
}


// ============================================================
// Make result
// ============================================================

FatigueDecisionResult
FatigueDecision::makeResult(
    FatigueDecisionPhase phase,
    bool ready,
    FatigueLevel candidateLevel,
    uint32_t reasonFlags,
    const FatigueTrendMetrics& trend
) const
{
    FatigueDecisionResult result;


    result.ready =
        ready;


    result.baselineReady =
        baseline_.ready;


    result.phase =
        phase;


    result.level =
        currentLevel_;


    result.candidateLevel =
        candidateLevel;


    result.baselineSamplesCollected =
        baselineSamples_.size();


    result.historySize =
        history_.size();


    result.baseline =
        baseline_;


    result.trend =
        trend;


    result.worseningStreak =
        worseningStreak_;


    result.recoveringStreak =
        recoveringStreak_;


    result.reasonFlags =
        reasonFlags;


    return result;
}


// ============================================================
// Evaluate
// ============================================================

FatigueDecisionResult
FatigueDecision::evaluate(
    const FatigueWindowStats& stats
)
{
    // ========================================================
    // 1. Window 不可信
    //
    // 不更新状态，不更新 streak。
    // ========================================================

    if (
        !statsQualified(stats)
    )
    {
        return makeResult(
            baseline_.ready
                ? FatigueDecisionPhase::MONITORING
                : FatigueDecisionPhase::LEARNING_BASELINE,

            false,

            baseline_.ready
                ? currentLevel_
                : FatigueLevel::UNKNOWN,

            FATIGUE_REASON_NONE,

            FatigueTrendMetrics()
        );
    }


    // ========================================================
    // 2. 初始个人行为 baseline
    // ========================================================

    if (!baseline_.ready)
    {
        baselineSamples_.push_back(
            stats
        );


        if (
            baselineSamples_.size() >=
            config_.baselineWindowCount
        )
        {
            buildBehaviorBaseline();


            currentLevel_ =
                FatigueLevel::NORMAL;


            history_.clear();


            worseningStreak_ = 0;

            recoveringStreak_ = 0;


            return makeResult(
                FatigueDecisionPhase::MONITORING,

                true,

                FatigueLevel::NORMAL,

                FATIGUE_REASON_NONE,

                FatigueTrendMetrics()
            );
        }


        return makeResult(
            FatigueDecisionPhase::LEARNING_BASELINE,

            false,

            FatigueLevel::UNKNOWN,

            FATIGUE_REASON_NONE,

            FatigueTrendMetrics()
        );
    }


    // ========================================================
    // 3. 正式监测
    // ========================================================

    pushHistory(
        stats
    );


    const FatigueTrendMetrics trend =
        buildTrend(
            stats
        );


    uint32_t reasons =
        FATIGUE_REASON_NONE;


    // ========================================================
    // 基础疲劳证据
    // ========================================================


    // --------------------------------------------------------
    // PERCLOS 5% ~ 15%
    // --------------------------------------------------------

    const bool fatiguePerclos =
        trend.recentGroupReady &&
        trend.recentPerclosMean >=
            config_.fatiguePerclosThreshold &&
        trend.recentPerclosMean <
            config_.deepPerclosThreshold;


    if (fatiguePerclos)
    {
        reasons |=
            FATIGUE_REASON_PERCLOS_FATIGUE;
    }


    // --------------------------------------------------------
    // PERCLOS 明显持续恶化
    // --------------------------------------------------------

    const bool perclosTrendWorsening =
        trend.enoughHistory &&

        trend.recentPerclosMean >
            baseline_.perclos &&

        trend.perclosRecentDelta >=
            config_.perclosTrendDelta;


    if (perclosTrendWorsening)
    {
        reasons |=
            FATIGUE_REASON_PERCLOS_TREND;
    }


    // --------------------------------------------------------
    // ECD 相对个人 baseline 恶化
    // --------------------------------------------------------

    bool ecdWorsening =
        false;


    if (
        baseline_.medianEcdMs > 0.0f &&
        trend.recentMedianEcdMean > 0.0f
    )
    {
        const bool aboveBaseline =
            trend.recentMedianEcdMean >=
            baseline_.medianEcdMs *
            config_.ecdIncreaseRatio;


        const bool recentStillRising =
            !trend.enoughHistory ||

            trend.previousMedianEcdMean <= 0.0f ||

            trend.recentMedianEcdMean >
                trend.previousMedianEcdMean;


        ecdWorsening =
            aboveBaseline &&
            recentStillRising;
    }


    if (ecdWorsening)
    {
        reasons |=
            FATIGUE_REASON_ECD;
    }


    // --------------------------------------------------------
    // Blink Duration
    // --------------------------------------------------------

    bool blinkDurationWorsening =
        false;


    if (
        baseline_.blinkDurationMs > 0.0f &&
        trend.recentBlinkDurationMean > 0.0f
    )
    {
        const bool aboveBaseline =
            trend.recentBlinkDurationMean >=
            baseline_.blinkDurationMs *
            config_.blinkDurationIncreaseRatio;


        const bool recentStillRising =
            !trend.enoughHistory ||

            trend.previousBlinkDurationMean <= 0.0f ||

            trend.recentBlinkDurationMean >
                trend.previousBlinkDurationMean;


        blinkDurationWorsening =
            aboveBaseline &&
            recentStillRising;
    }


    if (blinkDurationWorsening)
    {
        reasons |=
            FATIGUE_REASON_BLINK_DURATION;
    }


    // --------------------------------------------------------
    // LongClose / StrongLongClose
    // --------------------------------------------------------

    const bool longClosePresent =
        stats.longCloseCount > 0;


    const bool strongLongClosePresent =
        stats.strongLongCloseCount > 0;


    const bool longCloseFamilyPresent =
        longClosePresent ||
        strongLongClosePresent;


    if (longClosePresent)
    {
        reasons |=
            FATIGUE_REASON_LONG_CLOSE;
    }


    if (strongLongClosePresent)
    {
        reasons |=
            FATIGUE_REASON_STRONG_LONG_CLOSE;
    }


    // --------------------------------------------------------
    // LongClose 是否近期增加
    // --------------------------------------------------------

    const bool longCloseWorsening =
        trend.enoughHistory &&

        (
            trend.longCloseRecentDelta > 0.0f ||

            trend.strongLongCloseRecentDelta > 0.0f
        );


    // --------------------------------------------------------
    // Yawn
    // --------------------------------------------------------

    const bool yawnPresent =
        stats.yawnCount > 0;


    const bool yawnWorsening =
        trend.enoughHistory &&
        trend.yawnRecentDelta > 0.0f;


    if (yawnPresent)
    {
        reasons |=
            FATIGUE_REASON_YAWN;
    }


    // ========================================================
    // FATIGUE B
    //
    // PERCLOS明显持续恶化
    // +
    // ECD / LongClose / Yawn / Blink 至少一个同步恶化
    // ========================================================

    const bool secondaryTrendEvidence =
        ecdWorsening ||
        longCloseWorsening ||
        yawnWorsening ||
        blinkDurationWorsening;


    const bool fatigueByTrend =
        perclosTrendWorsening &&
        secondaryTrendEvidence;


    // ========================================================
    // FATIGUE C
    //
    // 最近60秒出现 LongClose / StrongLongClose
    // +
    // 另外一个疲劳证据
    // ========================================================

    const bool auxPerclosTrend =
        trend.enoughHistory &&
        trend.perclosRecentDelta >=
            config_.perclosAuxTrendDelta;


    const bool currentPerclosElevated =
        stats.perclos >=
        config_.fatiguePerclosThreshold;


    const bool currentEcdAbnormal =
        baseline_.medianEcdMs > 0.0f &&
        stats.medianEcdMs > 0.0f &&
        stats.medianEcdMs >=
            baseline_.medianEcdMs *
            config_.ecdIncreaseRatio;


    const bool currentBlinkDurationAbnormal =
        baseline_.blinkDurationMs > 0.0f &&
        stats.medianBlinkDurationMs > 0.0f &&
        stats.medianBlinkDurationMs >=
            baseline_.blinkDurationMs *
            config_.blinkDurationIncreaseRatio;


    const bool otherEvidenceForLongClose =
        currentPerclosElevated ||
        auxPerclosTrend ||
        currentEcdAbnormal ||
        currentBlinkDurationAbnormal ||
        yawnPresent;


    const bool fatigueByLongClose =
        longCloseFamilyPresent &&
        otherEvidenceForLongClose;


    // ========================================================
    // DEEP A
    //
    // recent3 PERCLOS mean >= 15%
    // ========================================================

    const bool deepByPerclosMean =
        trend.recentGroupReady &&
        trend.recentPerclosMean >=
            config_.deepPerclosThreshold;


    // ========================================================
    // DEEP B
    //
    // 最近3轮至少2轮 PERCLOS >= 15%
    // ========================================================

    const bool deepByRepeatedHighPerclos =
        trend.recentGroupReady &&
        trend.recentHighPerclosCount >= 2;


    if (
        deepByPerclosMean ||
        deepByRepeatedHighPerclos
    )
    {
        reasons |=
            FATIGUE_REASON_PERCLOS_DEEP;
    }


    // ========================================================
    // DEEP C
    //
    // StrongLongClose 反复出现
    // +
    // PERCLOS / ECD / Blink 异常
    // ========================================================

    const bool repeatedStrongLongClose =
        stats.strongLongCloseCount >=
        config_.repeatedStrongLongCloseCount;


    const bool deepSupportingAbnormality =
        currentPerclosElevated ||
        perclosTrendWorsening ||
        currentEcdAbnormal ||
        ecdWorsening ||
        currentBlinkDurationAbnormal ||
        blinkDurationWorsening;


    const bool deepByStrongClose =
        repeatedStrongLongClose &&
        deepSupportingAbnormality;


    // ========================================================
    // DEEP D
    //
    // 高PERCLOS
    // +
    // LongClose / StrongLongClose
    // +
    // Yawn / Blink / ECD
    // ========================================================

    bool highPerclos =
        stats.perclos >=
        config_.highPerclosForMultiEvidence;


    if (
        trend.recentGroupReady &&
        trend.recentPerclosMean >=
            config_.highPerclosForMultiEvidence
    )
    {
        highPerclos = true;
    }


    const bool thirdEvidence =
        yawnPresent ||
        currentBlinkDurationAbnormal ||
        currentEcdAbnormal;


    const bool deepByMultiEvidence =
        highPerclos &&
        longCloseFamilyPresent &&
        thirdEvidence;


    if (deepByMultiEvidence)
    {
        reasons |=
            FATIGUE_REASON_MULTI_EVIDENCE;
    }


    // ========================================================
    // Candidate Level
    // ========================================================

    FatigueLevel candidate =
        FatigueLevel::NORMAL;


    // --------------------------------------------------------
    // FATIGUE
    // --------------------------------------------------------

    if (
        fatiguePerclos ||
        fatigueByTrend ||
        fatigueByLongClose
    )
    {
        candidate =
            FatigueLevel::FATIGUE;
    }


    // --------------------------------------------------------
    // DEEP_FATIGUE
    //
    // 深度疲劳覆盖普通疲劳。
    // --------------------------------------------------------

    if (
        deepByPerclosMean ||
        deepByRepeatedHighPerclos ||
        deepByStrongClose ||
        deepByMultiEvidence
    )
    {
        candidate =
            FatigueLevel::DEEP_FATIGUE;
    }


    // ========================================================
    // Stateful hysteresis
    // ========================================================

    applyCandidateLevel(
        candidate
    );


    return makeResult(
        FatigueDecisionPhase::MONITORING,

        true,

        candidate,

        reasons,

        trend
    );
}


// ============================================================
// Level name
// ============================================================

const char* fatigueLevelName(
    FatigueLevel level
)
{
    switch (level)
    {
        case FatigueLevel::NORMAL:
            return "NORMAL";


        case FatigueLevel::FATIGUE:
            return "FATIGUE";


        case FatigueLevel::DEEP_FATIGUE:
            return "DEEP_FATIGUE";


        case FatigueLevel::UNKNOWN:
        default:
            return "UNKNOWN";
    }
}


// ============================================================
// Phase name
// ============================================================

const char* fatigueDecisionPhaseName(
    FatigueDecisionPhase phase
)
{
    switch (phase)
    {
        case FatigueDecisionPhase::MONITORING:
            return "MONITORING";


        case FatigueDecisionPhase::LEARNING_BASELINE:
        default:
            return "LEARNING_BASELINE";
    }
}