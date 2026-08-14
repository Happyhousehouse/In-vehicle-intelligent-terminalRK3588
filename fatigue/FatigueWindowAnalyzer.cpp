#include "FatigueWindowAnalyzer.h"

#include <algorithm>


// ============================================================
// Constructor
// ============================================================

FatigueWindowAnalyzer::FatigueWindowAnalyzer(
    const FatigueWindowAnalyzerConfig& config
)
    : config_(config)
{
}


// ============================================================
// Config
// ============================================================

void FatigueWindowAnalyzer::setConfig(
    const FatigueWindowAnalyzerConfig& config
)
{
    config_ = config;

    reset();
}


const FatigueWindowAnalyzerConfig&
FatigueWindowAnalyzer::config() const
{
    return config_;
}


// ============================================================
// Reset
// ============================================================

void FatigueWindowAnalyzer::reset()
{
    observationIntervals_.clear();

    eyeEvents_.clear();

    yawnEvents_.clear();


    previousFrameInitialized_ = false;

    previousTimestampMs_ = -1;

    previousObservationValid_ = false;

    previousEyeState_ =
        EyeState::UNKNOWN;


    sessionStartMs_ = -1;

    nextReportMs_ = -1;
}


// ============================================================
// Prune old data
//
// 只需要保留最近 windowMs 的数据。
// ============================================================

void FatigueWindowAnalyzer::pruneOldData(
    int64_t timestampMs
)
{
    const int64_t cutoffMs =
        timestampMs -
        config_.windowMs;


    // --------------------------------------------------------
    // Observation intervals
    //
    // 如果整个 interval 都已经位于窗口之前，
    // 就可以删除。
    //
    // 跨越 cutoff 的 interval 暂时保留，
    // buildStats() 时会做 overlap 裁剪。
    // --------------------------------------------------------

    while (!observationIntervals_.empty() &&
           observationIntervals_.front().endMs <=
               cutoffMs)
    {
        observationIntervals_.pop_front();
    }


    // --------------------------------------------------------
    // Eye events
    // --------------------------------------------------------

    while (!eyeEvents_.empty() &&
           eyeEvents_.front().timestampMs <
               cutoffMs)
    {
        eyeEvents_.pop_front();
    }


    // --------------------------------------------------------
    // Yawn events
    // --------------------------------------------------------

    while (!yawnEvents_.empty() &&
           yawnEvents_.front().timestampMs <
               cutoffMs)
    {
        yawnEvents_.pop_front();
    }
}


// ============================================================
// Build current window stats
// ============================================================

FatigueWindowStats
FatigueWindowAnalyzer::buildStats(
    int64_t timestampMs
) const
{
    FatigueWindowStats stats;


    if (sessionStartMs_ < 0)
    {
        return stats;
    }


    // ========================================================
    // 1. Window range
    // ========================================================

    const int64_t wantedStartMs =
        timestampMs -
        config_.windowMs;


    const int64_t windowStartMs =
        std::max(
            sessionStartMs_,
            wantedStartMs
        );


    stats.startMs =
        windowStartMs;

    stats.endMs =
        timestampMs;


    if (stats.endMs >
        stats.startMs)
    {
        stats.durationMs =
            stats.endMs -
            stats.startMs;
    }


    stats.fullWindow =
        stats.durationMs >=
        config_.windowMs;


    // ========================================================
    // 2. Valid observation time + CLOSED time
    // ========================================================

    for (const ObservationInterval& interval :
         observationIntervals_)
    {
        // ----------------------------------------------------
        // 求 interval 与当前 Window 的交集。
        // ----------------------------------------------------

        const int64_t overlapStartMs =
            std::max(
                interval.startMs,
                windowStartMs
            );


        const int64_t overlapEndMs =
            std::min(
                interval.endMs,
                timestampMs
            );


        if (overlapEndMs <=
            overlapStartMs)
        {
            continue;
        }


        const int64_t overlapMs =
            overlapEndMs -
            overlapStartMs;


        // ----------------------------------------------------
        // INVALID 不进入有效观测时间。
        // ----------------------------------------------------

        if (!interval.valid)
        {
            continue;
        }


        stats.validObservationMs +=
            overlapMs;


        // ----------------------------------------------------
        // 只有 EyeState == CLOSED
        // 才进入 PERCLOS 的 closed time。
        // ----------------------------------------------------

        if (interval.closed)
        {
            stats.closedObservationMs +=
                overlapMs;
        }
    }


    // ========================================================
    // 3. Valid observation ratio
    // ========================================================

    if (stats.durationMs > 0)
    {
        stats.validObservationRatio =
            static_cast<float>(
                stats.validObservationMs
            )
            /
            static_cast<float>(
                stats.durationMs
            );
    }


    // ========================================================
    // 4. PERCLOS
    //
    // CLOSED有效时间 / 有效Eye观测时间
    //
    // INVALID 不进入分母。
    // ========================================================

    if (stats.validObservationMs > 0)
    {
        stats.perclos =
            static_cast<float>(
                stats.closedObservationMs
            )
            /
            static_cast<float>(
                stats.validObservationMs
            );
    }


    // ========================================================
    // 5. Eye events
    // ========================================================

    std::vector<int64_t>
        ecdValues;
    std::vector<int64_t> blinkDurationValues;

    for (const EyeEventRecord& event :
         eyeEvents_)
    {
        if (event.timestampMs <
            windowStartMs)
        {
            continue;
        }


        if (event.timestampMs >
            timestampMs)
        {
            continue;
        }


        // ----------------------------------------------------
        // Event count
        // ----------------------------------------------------

        switch (event.type)
        {
            case EyeClosureType::BLINK:
            {
                ++stats.blinkCount;


                // ====================================================
                // Blink Duration
                //
                // 普通 blink 可能根本没有进入 CLOSED，
                // 因此这里必须使用：
                //
                // totalDurationMs
                //
                // 而不是 closedDurationMs。
                // ====================================================

                if (
                    event.totalDurationMs > 0 &&
                    event.totalDurationMs <=
                        config_.maxBlinkDurationForStatsMs
                )
                {
                    blinkDurationValues.push_back(
                        event.totalDurationMs
                    );
                }

                break;
            }


            case EyeClosureType::LONG_CLOSE:
            {
                ++stats.longCloseCount;
                break;
            }


            case EyeClosureType::STRONG_LONG_CLOSE:
            {
                ++stats.strongLongCloseCount;
                break;
            }


            // 当前旧版仍然可能存在 SLOW_CLOSURE。
            //
            // 第一版 WindowAnalyzer 暂时不把它计入
            // BlinkRate / LongClose。
            case EyeClosureType::SLOW_CLOSURE:
            case EyeClosureType::NONE:
            default:
            {
                break;
            }
        }


        // ----------------------------------------------------
        // ECD
        //
        // direct:
        //
        // OPEN -> CLOSING -> OPEN
        //
        // 这种 Blink：
        //
        // closedDurationMs == 0
        //
        // 可以计入 BlinkCount，
        // 但不能拿 0 去污染 ECD。
        // ----------------------------------------------------

        if (event.closedDurationMs > 0)
        {
            ecdValues.push_back(
                event.closedDurationMs
            );


            stats.maxEcdMs =
                std::max(
                    stats.maxEcdMs,
                    event.closedDurationMs
                );
        }
    }


    // ========================================================
    // 6. Blink rate
    //
    // BlinkCount
    // ------------------------- × 60s
    // valid observation time
    //
    // No Face 不进入分母。
    // ========================================================

    if (stats.validObservationMs > 0)
    {
        stats.blinkRatePerMinute =
            static_cast<float>(
                stats.blinkCount
            )
            *
            60000.0f
            /
            static_cast<float>(
                stats.validObservationMs
            );
    }


    // ========================================================
    // Blink Duration
    // ========================================================

    stats.blinkDurationSampleCount =
        static_cast<int>(
            blinkDurationValues.size()
        );


    if (!blinkDurationValues.empty())
    {
        std::sort(
            blinkDurationValues.begin(),
            blinkDurationValues.end()
        );


        const std::size_t count =
            blinkDurationValues.size();

        const std::size_t middle =
            count / 2;


        if ((count % 2) != 0)
        {
            stats.medianBlinkDurationMs =
                static_cast<float>(
                    blinkDurationValues[middle]
                );
        }
        else
        {
            stats.medianBlinkDurationMs =
                (
                    static_cast<float>(
                        blinkDurationValues[middle - 1]
                    )
                    +
                    static_cast<float>(
                        blinkDurationValues[middle]
                    )
                ) * 0.5f;
        }
    }
    else
    {
        stats.medianBlinkDurationMs = 0.0f;
    }

    // ========================================================
    // 7. Median ECD
    // ========================================================

    if (!ecdValues.empty())
    {
        std::sort(
            ecdValues.begin(),
            ecdValues.end()
        );


        const std::size_t count =
            ecdValues.size();


        if ((count % 2) == 1)
        {
            stats.medianEcdMs =
                static_cast<float>(
                    ecdValues[
                        count / 2
                    ]
                );
        }
        else
        {
            const int64_t a =
                ecdValues[
                    count / 2 - 1
                ];

            const int64_t b =
                ecdValues[
                    count / 2
                ];


            stats.medianEcdMs =
                static_cast<float>(
                    a + b
                )
                /
                2.0f;
        }
    }


    // ========================================================
    // 8. Yawn
    // ========================================================

    for (const YawnRecord& event :
         yawnEvents_)
    {
        if (event.timestampMs >=
                windowStartMs
            &&
            event.timestampMs <=
                timestampMs)
        {
            ++stats.yawnCount;
        }
    }


    return stats;
}


// ============================================================
// Current stats
// ============================================================

FatigueWindowStats
FatigueWindowAnalyzer::currentStats(
    int64_t timestampMs
) const
{
    return buildStats(
        timestampMs
    );
}


// ============================================================
// Update
// ============================================================

bool FatigueWindowAnalyzer::update(
    const FatigueEventResult& result,
    int64_t timestampMs,
    FatigueWindowStats& outStats
)
{
    // ========================================================
    // 1. Timestamp rollback protection
    // ========================================================

    if (previousFrameInitialized_ &&
        timestampMs <
            previousTimestampMs_)
    {
        reset();
    }


    // ========================================================
    // 2. First frame
    // ========================================================

    if (!previousFrameInitialized_)
    {
        previousFrameInitialized_ =
            true;

        previousTimestampMs_ =
            timestampMs;

        previousObservationValid_ =
            result.observationValid;

        previousEyeState_ =
            result.eyeState;


        sessionStartMs_ =
            timestampMs;


        nextReportMs_ =
            timestampMs +
            config_.reportIntervalMs;


        // ----------------------------------------------------
        // 第一帧也允许接收 Event，
        // 虽然正常运行时第一帧几乎不会产生事件。
        // ----------------------------------------------------

        if (result.eyeClosure.completed)
        {
            EyeEventRecord record;

            record.timestampMs =
                result.eyeClosure.endMs > 0
                    ? result.eyeClosure.endMs
                    : timestampMs;

            record.type =
                result.eyeClosure.type;

            record.closedDurationMs =
                result.eyeClosure.closedDurationMs;
            
            record.totalDurationMs =
                result.eyeClosure.totalDurationMs;


            eyeEvents_.push_back(
                record
            );
        }


        if (result.yawn.detected)
        {
            YawnRecord record;

            record.timestampMs =
                timestampMs;

            yawnEvents_.push_back(
                record
            );
        }


        return false;
    }


    // ========================================================
    // 3. Build interval from previous frame to current frame
    //
    // 例如：
    //
    // previous = CLOSED / valid
    // previousTimestamp = 1000
    //
    // currentTimestamp = 1033
    //
    // 那么：
    //
    // [1000,1033)
    //
    // 计入：
    //
    // validObservationMs += 33
    // closedObservationMs += 33
    // ========================================================

    if (timestampMs >
        previousTimestampMs_)
    {
        ObservationInterval interval;

        interval.startMs =
            previousTimestampMs_;

        interval.endMs =
            timestampMs;

        interval.valid =
            previousObservationValid_;

        interval.closed =
            previousObservationValid_
            &&
            previousEyeState_ ==
                EyeState::CLOSED;


        observationIntervals_.push_back(
            interval
        );
    }


    // ========================================================
    // 4. Completed Eye Event
    // ========================================================

    if (result.eyeClosure.completed)
    {
        EyeEventRecord record;


        record.timestampMs =
            result.eyeClosure.endMs > 0
                ? result.eyeClosure.endMs
                : timestampMs;


        record.type =
            result.eyeClosure.type;


        record.closedDurationMs =
            result.eyeClosure.closedDurationMs;
        
        record.totalDurationMs =
            result.eyeClosure.totalDurationMs;


        eyeEvents_.push_back(
            record
        );
    }


    // ========================================================
    // 5. Yawn Event
    //
    // Yawn Tracker 已经保证一次长张嘴只触发一次。
    //
    // WindowAnalyzer 这里只做：
    //
    // yawnCount += 1
    // ========================================================

    if (result.yawn.detected)
    {
        YawnRecord record;

        record.timestampMs =
            timestampMs;


        yawnEvents_.push_back(
            record
        );
    }


    // ========================================================
    // 6. Save current frame for next interval
    // ========================================================

    previousTimestampMs_ =
        timestampMs;

    previousObservationValid_ =
        result.observationValid;

    previousEyeState_ =
        result.eyeState;


    // ========================================================
    // 7. Remove data older than 60s
    // ========================================================

    pruneOldData(
        timestampMs
    );


    // ========================================================
    // 8. 10-second report
    // ========================================================

    if (nextReportMs_ >= 0 &&
        timestampMs >=
            nextReportMs_)
    {
        outStats =
            buildStats(
                timestampMs
            );


        // ----------------------------------------------------
        // 正常情况下每次只跨一个 report interval。
        //
        // while 用来处理：
        //
        // 程序卡顿后一次跳过多个 10 秒周期。
        //
        // 避免 nextReportMs_ 永远落在过去。
        // ----------------------------------------------------

        while (nextReportMs_ <=
               timestampMs)
        {
            nextReportMs_ +=
                config_.reportIntervalMs;
        }


        return true;
    }


    return false;
}