#include "FatigueService.h"

#include <chrono>
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include "FatigueWindowAnalyzer.h"
#include <algorithm>
#include <QMetaObject>


namespace
{

const char* eyeStateName(
    EyeState state
)
{
    switch (state)
    {
        case EyeState::UNKNOWN:
            return "UNKNOWN";

        case EyeState::OPEN:
            return "OPEN";

        case EyeState::CLOSING:
            return "CLOSING";

        case EyeState::CLOSED:
            return "CLOSED";

        case EyeState::OPENING:
            return "OPENING";
    }

    return "UNKNOWN";
}


const char* eyeClosureTypeName(
    EyeClosureType type
)
{
    switch (type)
    {
        case EyeClosureType::NONE:
            return "NONE";

        case EyeClosureType::BLINK:
            return "BLINK";

        case EyeClosureType::SLOW_CLOSURE:
            return "SLOW_CLOSURE";

        case EyeClosureType::LONG_CLOSE:
            return "LONG_CLOSE";

        case EyeClosureType::STRONG_LONG_CLOSE:
            return "STRONG_LONG_CLOSE";
    }

    return "NONE";
}

}

// ============================================================
// Constructor
// ============================================================

FatigueService::FatigueService(
    CameraService* camera,
    QObject* parent
)
    :
      QObject(parent),
      camera_(camera)
{
}


// ============================================================
// Destructor
// ============================================================

FatigueService::~FatigueService()
{
    std::cout
        << "[FatigueService] destructor begin"
        << std::endl;

    stop();

    std::cout
        << "[FatigueService] destructor end"
        << std::endl;
}


// ============================================================
// Initialize
// ============================================================

bool FatigueService::initialize()
{
    if (initialized_)
    {
        return true;
    }


    if (camera_ == nullptr)
    {
        std::cerr
            << "[FatigueService] CameraService is null."
            << std::endl;

        return false;
    }


    if (!vision_.initialize())
    {
        std::cerr
            << "[FatigueService] FatigueVision initialization failed."
            << std::endl;

        return false;
    }


    initialized_ = true;


    std::cout
        << "[FatigueService] Initialization SUCCESS."
        << std::endl;


    return true;
}


// ============================================================
// Start
// ============================================================

bool FatigueService::start()
{
    if (running_)
    {
        return true;
    }


    if (!initialized_)
    {
        if (!initialize())
        {
            return false;
        }
    }


    // ========================================================
    // 每次重新开启疲劳检测，
    // 都重新建立当前驾驶员的眼睛 Baseline。
    // ========================================================

    baseline_.resetSession();

    eventTracker_.reset();


    // Decision / WindowAnalyzer 在 processLoop()
    // 中重新创建，因此重新启动就是一个新的检测 session。


    setFatigueLevel(
        FatigueLevel::UNKNOWN
    );

    setEyeWarningActive(
        false
    );
    running_ = true;


    emit monitoringEnabledChanged();


    workerThread_ =
        std::thread(
            &FatigueService::processLoop,
            this
        );


    std::cout
        << "[FatigueService] Started."
        << std::endl;


    return true;
}


// ============================================================
// Stop
// ============================================================

void FatigueService::stop()
{
    const bool wasRunning =
        running_.exchange(false);


    if (workerThread_.joinable())
    {
        std::cout
            << "[FatigueService] joining worker thread"
            << std::endl;


        workerThread_.join();


        std::cout
            << "[FatigueService] worker thread joined"
            << std::endl;
    }


    // 用户关闭疲劳检测以后，
    // UI不应该继续显示上一次 NORMAL / FATIGUE。
    setFatigueLevel(
        FatigueLevel::UNKNOWN
    );
    setEyeWarningActive(
        false
    );

    if (wasRunning)
    {
        emit monitoringEnabledChanged();
    }
}

// ============================================================
// State
// ============================================================

bool FatigueService::isRunning() const
{
    return running_;
}

QString FatigueService::fatigueLevel() const
{
    const FatigueLevel level =
        static_cast<FatigueLevel>(
            fatigueLevelCode_.load()
        );


    switch (level)
    {
        case FatigueLevel::NORMAL:
            return QStringLiteral(
                "NORMAL"
            );


        case FatigueLevel::FATIGUE:
            return QStringLiteral(
                "FATIGUE"
            );


        case FatigueLevel::DEEP_FATIGUE:
            return QStringLiteral(
                "DEEP_FATIGUE"
            );


        case FatigueLevel::UNKNOWN:
        default:
            return QStringLiteral(
                "UNKNOWN"
            );
    }
}

bool FatigueService::eyeWarningActive() const
{
    return eyeWarningActive_.load();
}

void FatigueService::setEyeWarningActive(
    bool active
)
{
    const bool oldValue =
        eyeWarningActive_.exchange(
            active
        );


    if (oldValue == active)
    {
        return;
    }


    QMetaObject::invokeMethod(
        this,

        [this]()
        {
            emit eyeWarningActiveChanged();
        },

        Qt::QueuedConnection
    );
}


bool FatigueService::setMonitoringEnabled(
    bool enabled
)
{
    if (enabled)
    {
        return start();
    }


    stop();

    return true;
}


void FatigueService::setFatigueLevel(
    FatigueLevel level
)
{
    const int newCode =
        static_cast<int>(
            level
        );


    const int oldCode =
        fatigueLevelCode_.exchange(
            newCode
        );


    // 等级没变就不通知UI。
    if (oldCode == newCode)
    {
        return;
    }


    // processLoop运行在std::thread中。
    // 不直接操作QML。
    //
    // 把通知投递到FatigueService所属的Qt主线程。
    QMetaObject::invokeMethod(
        this,

        [this]()
        {
            emit fatigueLevelChanged();
        },

        Qt::QueuedConnection
    );
}
// ============================================================
// Processing thread
// ============================================================

void FatigueService::processLoop()
{
    using namespace std::chrono;


    // ========================================================
    // 疲劳检测自适应采样率：
    //
    // 检测到人脸或刚刚短暂丢失人脸：30 FPS（约 33ms）
    // 长时间无人脸：                    15 FPS（约 66ms）
    //
    // 注意：处理耗时包含在33/66ms周期内，不是在处理结束后
    // 再额外等待一个完整周期。
    // ========================================================

    constexpr int64_t activeFramePeriodMs = 33;
    constexpr int64_t idleFramePeriodMs = 66;
    constexpr int64_t faceHoldMs = 1500;
    constexpr int64_t periodicLogIntervalMs = 1000;

    FatigueWindowAnalyzer windowAnalyzer;
    FatigueDecision fatigueDecision;
    int64_t targetFramePeriodMs = idleFramePeriodMs;

    bool hasSeenFace = false;
    bool hasLoggedFaceMode = false;
    bool lastLoggedFaceRecentlySeen = false;

    steady_clock::time_point lastFaceSeenAt;
    steady_clock::time_point lastPeriodicLogAt =
        steady_clock::now() -
        milliseconds(periodicLogIntervalMs);


    // 用于打印 EyeState 状态变化
    EyeState lastLoggedEyeState =
        EyeState::UNKNOWN;


    // 临时调试：只保存一张关键点图
    bool landmarkDebugSaved = false;


    while (running_)
    {
        const auto loopStart =
            steady_clock::now();
        // ====================================================
        // 1. 从现有 CameraService 获取最新帧
        // ====================================================

        cv::Mat frame =
            camera_->getFrame();


        if (frame.empty())
        {
            std::this_thread::sleep_until(
                loopStart +
                milliseconds(
                    targetFramePeriodMs
                )
            );

            continue;
        }


        // ====================================================
        // 2. FatigueVision
        // ====================================================

        const auto visionStart =
            steady_clock::now();


        FatigueVisionResult result =
            vision_.process(frame);


        const auto visionEnd =
            steady_clock::now();


        // Keep full-rate processing for a short grace period after one missed
        // detection. This prevents the frame rate from oscillating when a
        // driver turns their head or one detector frame is unstable.
        if (result.faceDetected)
        {
            hasSeenFace = true;
            lastFaceSeenAt = visionEnd;
        }

        const bool faceRecentlySeen =
            hasSeenFace
            &&
            duration_cast<milliseconds>(
                visionEnd - lastFaceSeenAt
            ).count() <= faceHoldMs;

        targetFramePeriodMs =
            faceRecentlySeen
                ? activeFramePeriodMs
                : idleFramePeriodMs;


        const int64_t visionCostMs =
            duration_cast<milliseconds>(
                visionEnd -
                visionStart
            ).count();


        const int64_t timestampMs =
            duration_cast<milliseconds>(
                loopStart.time_since_epoch()
            ).count();

        if (result.valid)
        {
            // ========================================================
            // 当前驾驶员 Eye Baseline Calibration
            // ========================================================

            baseline_.updateEyeCalibration(
                result.leftEyeRatio,
                result.rightEyeRatio,
                timestampMs
            );
        }

        // ============================================================
        // FatigueEventTracker
        //
        // 只有 Eye Baseline READY 以后，
        // 才允许进入归一化 Eye State Space。
        // ============================================================

        FatigueEventResult eventResult;

        bool eventTrackerUpdated = false;

        NormalizedEyeObservation normalized;


        // ------------------------------------------------------------
        // Baseline 已完成
        // ------------------------------------------------------------

        if (baseline_.eyeBaselineReady())
        {
            FatigueObservation observation;

            observation.timestampMs =
                timestampMs;

            observation.mouthRatio =
                result.mouthRatio;


            // --------------------------------------------------------
            // 当前视觉帧有效
            // --------------------------------------------------------

            if (result.valid)
            {
                normalized =
                    baseline_.normalizeEyes(
                        result.leftEyeRatio,
                        result.rightEyeRatio
                    );


                if (normalized.valid)
                {
                    observation.valid = true;

                    observation.leftEyeOpenness =
                        normalized.leftOpenness;

                    observation.rightEyeOpenness =
                        normalized.rightOpenness;
                }
                else
                {
                    observation.valid = false;
                }
            }

            // --------------------------------------------------------
            // No Face / PFLD Failed
            //
            // 仍然必须调用 tracker.update()。
            //
            // 因为 EventTracker 需要知道：
            //
            // invalid 持续了多少 ms。
            // --------------------------------------------------------

            else
            {
                observation.valid = false;
            }


            eventResult =
                eventTracker_.update(
                    observation
                );


            eventTrackerUpdated = true;


            // ========================================================
            // FatigueWindowAnalyzer
            //
            // EventTracker 每更新一帧，
            // WindowAnalyzer 也同步更新一帧。
            //
            // 输入：
            //
            // observationValid
            // eyeState
            // eyeClosure event
            // yawn event
            //
            // 每 10 秒返回一次 WindowStats。
            // ========================================================

            FatigueWindowStats windowStats;


            const bool windowReportReady =
                windowAnalyzer.update(
                    eventResult,
                    timestampMs,
                    windowStats
                );


            if (windowReportReady)
            {
                std::cout
                    << "[WindowDebug]"

                    << " duration="
                    << windowStats.durationMs
                    << "ms"

                    << " full="
                    << (
                        windowStats.fullWindow
                            ? 1
                            : 0
                    )

                    << " valid="
                    << windowStats.validObservationRatio
                    << " (" 
                    << windowStats.validObservationMs
                    << "ms)"

                    << " blink="
                    << windowStats.blinkCount

                    << " blinkRate="
                    << windowStats.blinkRatePerMinute
                    << "/min"

                    << " medianBlink="
                    << windowStats.medianBlinkDurationMs
                    << "ms"

                    << " blinkSamples="
                    << windowStats.blinkDurationSampleCount

                    << " medianECD="
                    << windowStats.medianEcdMs
                    << "ms"

                    << " maxECD="
                    << windowStats.maxEcdMs
                    << "ms"

                    << " perclos="
                    << windowStats.perclos

                    << " closed="
                    << windowStats.closedObservationMs
                    << "ms"

                    << " longClose="
                    << windowStats.longCloseCount

                    << " strongLongClose="
                    << windowStats.strongLongCloseCount

                    << " yawn="
                    << windowStats.yawnCount

                    << std::endl;
                    const FatigueDecisionResult decisionResult =
                        fatigueDecision.evaluate(
                            windowStats
                        );

                    // ========================================================
                    // 发布最终稳定疲劳等级给UI
                    //
                    // 注意：
                    // 用 level，不使用 candidateLevel。
                    // ========================================================

                    setFatigueLevel(
                        decisionResult.level
                    );
                    // ========================================================
                    // Fatigue Decision 状态
                    // ========================================================

                    std::cout
                        << "[FatigueDecision]"

                        << " phase="
                        << fatigueDecisionPhaseName(
                            decisionResult.phase
                        )

                        << " ready="
                        << (
                            decisionResult.ready
                                ? 1
                                : 0
                        )

                        << " baselineReady="
                        << (
                            decisionResult.baselineReady
                                ? 1
                                : 0
                        )

                        << " baselineSamples="
                        << decisionResult.baselineSamplesCollected
                        << "/"
                        << fatigueDecision.config().baselineWindowCount

                        << " history="
                        << decisionResult.historySize
                        << "/"
                        << fatigueDecision.config().historyWindowCount

                        << " candidate="
                        << fatigueLevelName(
                            decisionResult.candidateLevel
                        )

                        << " level="
                        << fatigueLevelName(
                            decisionResult.level
                        )

                        << " worsening="
                        << decisionResult.worseningStreak

                        << " recovering="
                        << decisionResult.recoveringStreak

                        << " reasons="
                        << decisionResult.reasonFlags

                        << std::endl;


                    // ========================================================
                    // Behavioral Baseline
                    // ========================================================

                    if (decisionResult.baselineReady)
                    {
                        const FatigueBehaviorBaseline& baseline =
                            decisionResult.baseline;

                        std::cout
                            << "[DecisionBaseline]"

                            << " samples="
                            << baseline.sampleCount

                            << " perclos="
                            << baseline.perclos

                            << " medianECD="
                            << baseline.medianEcdMs
                            << "ms"

                            << " typicalMaxECD="
                            << baseline.typicalMaxEcdMs
                            << "ms"

                            << " blinkRate="
                            << baseline.blinkRatePerMinute
                            << "/min"

                            << " blinkDuration="
                            << baseline.blinkDurationMs
                            << "ms"

                            << " longClose="
                            << baseline.longCloseCount

                            << " strongLongClose="
                            << baseline.strongLongCloseCount

                            << " yawn="
                            << baseline.yawnCount

                            << std::endl;
                    }


                    // ========================================================
                    // Trend
                    // ========================================================

                    if (
                        decisionResult.baselineReady &&
                        decisionResult.historySize > 0
                    )
                    {
                        const FatigueTrendMetrics& trend =
                            decisionResult.trend;

                        std::cout
                            << "[DecisionTrend]"

                            << " enough="
                            << (
                                trend.enoughHistory
                                    ? 1
                                    : 0
                            )

                            // ------------------------------------------------
                            // 当前窗口 VS 初始基准
                            // ------------------------------------------------

                            << " perclosBaseDelta="
                            << trend.perclosDeltaFromBaseline

                            << " medianEcdBaseDelta="
                            << trend.medianEcdDeltaFromBaselineMs
                            << "ms"

                            << " maxEcdBaseDelta="
                            << trend.maxEcdDeltaFromBaselineMs
                            << "ms"

                            << " blinkRateBaseDelta="
                            << trend.blinkRateDeltaFromBaseline

                            << " blinkDurationBaseDelta="
                            << trend.blinkDurationDeltaFromBaselineMs
                            << "ms"

                            << " longCloseBaseDelta="
                            << trend.longCloseDeltaFromBaseline

                            << " strongCloseBaseDelta="
                            << trend.strongLongCloseDeltaFromBaseline

                            << " yawnBaseDelta="
                            << trend.yawnDeltaFromBaseline

                            << std::endl;


                        if (trend.enoughHistory)
                        {
                            std::cout
                                << "[DecisionTrendCompare]"

                                // --------------------------------------------
                                // PERCLOS
                                // --------------------------------------------

                                << " perclos="
                                << trend.previousPerclosMean
                                << "->"
                                << trend.recentPerclosMean

                                << " delta="
                                << trend.perclosRecentDelta


                                // --------------------------------------------
                                // Median ECD
                                // --------------------------------------------

                                << " medianECD="
                                << trend.previousMedianEcdMean
                                << "->"
                                << trend.recentMedianEcdMean

                                << " delta="
                                << trend.medianEcdRecentDeltaMs
                                << "ms"


                                // --------------------------------------------
                                // Max ECD
                                // --------------------------------------------

                                << " maxECD="
                                << trend.previousMaxEcdMean
                                << "->"
                                << trend.recentMaxEcdMean

                                << " delta="
                                << trend.maxEcdRecentDeltaMs
                                << "ms"


                                // --------------------------------------------
                                // Blink Rate
                                // --------------------------------------------

                                << " blinkRate="
                                << trend.previousBlinkRateMean
                                << "->"
                                << trend.recentBlinkRateMean

                                << " delta="
                                << trend.blinkRateRecentDelta

                                << " blinkDuration="
                                << trend.previousBlinkDurationMean
                                << "->"
                                << trend.recentBlinkDurationMean

                                << " delta="
                                << trend.blinkDurationRecentDeltaMs
                                << "ms"
                                // --------------------------------------------
                                // Long Close
                                // --------------------------------------------

                                << " longClose="
                                << trend.previousLongCloseMean
                                << "->"
                                << trend.recentLongCloseMean

                                << " delta="
                                << trend.longCloseRecentDelta


                                // --------------------------------------------
                                // Strong Long Close
                                // --------------------------------------------

                                << " strongClose="
                                << trend.previousStrongLongCloseMean
                                << "->"
                                << trend.recentStrongLongCloseMean

                                << " delta="
                                << trend.strongLongCloseRecentDelta


                                // --------------------------------------------
                                // Yawn
                                // --------------------------------------------

                                << " yawn="
                                << trend.previousYawnMean
                                << "->"
                                << trend.recentYawnMean

                                << " delta="
                                << trend.yawnRecentDelta

                                << std::endl;
                        }
                    }
            }

            // ============================================================
            // Eye per-frame diagnostic
            //
            // 临时用于普通眨眼调试。
            // Baseline READY 后每一帧都打印：
            //
            // raw:
            //     Baseline 归一化之后、EMA 之前
            //
            // ema:
            //     EventTracker EMA 平滑之后
            //
            // combined:
            //     max(left, right)
            //
            // 调试完成后可以删除或降低打印频率。
            // ============================================================

            if (eventTrackerUpdated)
            {
                if (eventResult.observationValid &&
                    normalized.valid)
                {
                    const float rawCombined =
                        std::max(
                            normalized.leftOpenness,
                            normalized.rightOpenness
                        );


                    std::cout
                        << "[EyeDebug]"
                        << " t=" << timestampMs

                        << " state="
                        << eyeStateName(
                            eventResult.eyeState
                        )

                        // --------------------------------------------
                        // Baseline normalized，EMA 之前
                        // --------------------------------------------
                        << " rawL="
                        << normalized.leftOpenness

                        << " rawR="
                        << normalized.rightOpenness

                        << " rawC="
                        << rawCombined

                        // --------------------------------------------
                        // EMA 之后
                        // --------------------------------------------
                        << " emaL="
                        << eventResult.smoothedLeftOpenness

                        << " emaR="
                        << eventResult.smoothedRightOpenness

                        << " emaC="
                        << eventResult.combinedOpenness

                        // --------------------------------------------
                        // 当前变化速度
                        // --------------------------------------------
                        << " slope="
                        << eventResult.opennessSlopePerSecond

                        // --------------------------------------------
                        // 当前状态持续时间
                        // --------------------------------------------
                        << " stateMs="
                        << eventResult.stateDurationMs

                        // --------------------------------------------
                        // 真正 CLOSED 后持续时间
                        // --------------------------------------------
                        << " closedMs="
                        << eventResult.currentClosedDurationMs

                        << " episodeMs="
                        << eventResult.currentEyeEpisodeDurationMs

                        << " lowMs="
                        << eventResult.currentLowOpennessDurationMs

                        << std::endl;

                }
                else
                {
                    std::cout
                        << "[EyeDebug]"
                        << " t=" << timestampMs
                        << " observation=INVALID"
                        << " state="
                        << eyeStateName(
                            eventResult.eyeState
                        )
                        << std::endl;
                }
            }
        }

        // ============================================================
        // Eye state transition debug
        //
        // 只在状态发生变化时打印。
        // ============================================================

        if (eventTrackerUpdated &&
            eventResult.eyeState !=
                lastLoggedEyeState)
        {
            std::cout
                << "[EyeState] "
                << eyeStateName(
                    lastLoggedEyeState
                )
                << " -> "
                << eyeStateName(
                    eventResult.eyeState
                )
                << " left="
                << eventResult.smoothedLeftOpenness
                << " right="
                << eventResult.smoothedRightOpenness
                << " combined="
                << eventResult.combinedOpenness
                << " slope="
                << eventResult.opennessSlopePerSecond
                << std::endl;


            lastLoggedEyeState =
                eventResult.eyeState;
        }

        // ============================================================
        // Completed Eye Closure Event
        // ============================================================

        if (eventTrackerUpdated &&
            eventResult.eyeClosure.completed)
        {
            const EyeClosureEvent& event =
                eventResult.eyeClosure;


            std::cout
                << "[EyeEvent] type="
                << eyeClosureTypeName(
                    event.type
                )
                << " closed="
                << event.closedDurationMs
                << "ms"
                << " total="
                << event.totalDurationMs
                << "ms"
                << " minLeft="
                << event.minLeftOpenness
                << " minRight="
                << event.minRightOpenness
                << std::endl;
        }

                // ============================================================
        // Mouth diagnostic
        //
        // 用于测试基础哈欠检测：
        // mouthRatio >= 0.80 持续 >= 4000ms
        // ============================================================

        if (eventTrackerUpdated &&
            eventResult.observationValid)
        {
            std::cout
                << "[MouthDebug]"
                << " ratio="
                << result.mouthRatio
                << " openMs="
                << eventResult.currentMouthOpenMs
                << std::endl;
        }

        // ============================================================
        // Yawn event
        // ============================================================

        if (eventTrackerUpdated &&
            eventResult.yawn.detected)
        {
            std::cout
                << "[Yawn]"
                << " duration="
                << eventResult.yawn.durationMs
                << "ms"
                << " peak="
                << eventResult.yawn.peakMouthRatio
                << std::endl;
        }
        // ============================================================
        // Fatigue warning
        //
        // 新规则：
        //
        // 只有 CLOSING：
        //     episode >= 4000ms
        //
        // 当前 episode 曾进入 CLOSED：
        //     episode >= 2000ms
        //
        // emergencyTriggered 这个字段名暂时保留，
        // 后续统一重构时再改名。
        // ============================================================

        // ============================================================
        // 即时驾驶安全警告
        //
        // 当 EventTracker 判断当前眼睛闭合 episode
        // 已经达到警告条件时立即显示。
        //
        // 这个警告不等待 60 秒 Window / FatigueDecision。
        // ============================================================

        if (
            eventTrackerUpdated &&
            eventResult.emergencyTriggered
        )
        {
            setEyeWarningActive(
                true
            );


            std::cout
                << "[Warning]"
                << " episodeMs="
                << eventResult.currentEyeEpisodeDurationMs
                << "ms"
                << " state="
                << eyeStateName(
                    eventResult.eyeState
                )
                << " combined="
                << eventResult.combinedOpenness
                << std::endl;
        }


        // ============================================================
        // 驾驶员重新明确睁眼后解除即时警告。
        //
        // OPENING阶段仍保持警告，
        // 直到真正回到OPEN。
        // ============================================================

        if (
            eventTrackerUpdated &&
            eyeWarningActive_.load() &&
            (
                eventResult.eyeState ==
                    EyeState::OPEN
                ||
                eventResult.eyeState ==
                    EyeState::UNKNOWN
            )
        )
        {
            setEyeWarningActive(
                false
            );
        }
                // ============================================================
        // 临时：保存一张 PFLD 98 点调试图
        // ============================================================

        if (!landmarkDebugSaved &&
            result.faceDetected &&
            result.landmarksValid &&
            result.landmarks.size() == 98)
        {
            cv::Mat debugImage =
                frame.clone();


            // --------------------------------------------------------
            // 画 Face Rect
            // --------------------------------------------------------

            cv::rectangle(
                debugImage,
                result.faceRect,
                cv::Scalar(0, 255, 0),
                2
            );


            // --------------------------------------------------------
            // 画 98 个 landmarks
            // --------------------------------------------------------

            for (int i = 0; i < 98; ++i)
            {
                const cv::Point2f& p =
                    result.landmarks[i];


                cv::circle(
                    debugImage,
                    p,
                    3,
                    cv::Scalar(0, 0, 255),
                    -1
                );


                // 为了方便我们检查点序，
                // 给每个点标上编号。
                cv::putText(
                    debugImage,
                    std::to_string(i),
                    cv::Point(
                        static_cast<int>(p.x) + 3,
                        static_cast<int>(p.y) - 3
                    ),
                    cv::FONT_HERSHEY_SIMPLEX,
                    0.30,
                    cv::Scalar(255, 255, 0),
                    1,
                    cv::LINE_AA
                );
            }


            const std::string debugPath =
                "/home/firefly/Pictures/"
                "fatigue_landmarks_debug.jpg";


            if (cv::imwrite(
                    debugPath,
                    debugImage))
            {
                std::cout
                    << "[FatigueService] Landmark debug image saved: "
                    << debugPath
                    << std::endl;


                landmarkDebugSaved = true;
            }
            else
            {
                std::cerr
                    << "[FatigueService] Failed to save landmark debug image."
                    << std::endl;
            }
        }
        // ====================================================
        // 3. 当前阶段只验证 Face
        // ====================================================

        const bool faceModeChanged =
            !hasLoggedFaceMode
            ||
            faceRecentlySeen != lastLoggedFaceRecentlySeen;

        const bool periodicLogDue =
            duration_cast<milliseconds>(
                visionEnd - lastPeriodicLogAt
            ).count() >= periodicLogIntervalMs;

        const bool shouldPrintStatus =
            faceModeChanged || periodicLogDue;


        // 状态变化立即打印；稳定状态最多每秒打印一次。
        if (shouldPrintStatus)
        {
            lastPeriodicLogAt = visionEnd;
            hasLoggedFaceMode = true;
            lastLoggedFaceRecentlySeen = faceRecentlySeen;


            // ========================================================
            // Baseline debug
            // ========================================================

            if (!baseline_.eyeBaselineReady())
            {
                if (result.valid)
                {
                    std::cout
                        << "[Baseline] calibrating "
                        << baseline_.eyeCalibrationProgress() * 100.0f
                        << "%"
                        << " left="
                        << result.leftEyeRatio
                        << " right="
                        << result.rightEyeRatio
                        << std::endl;
                }
                else
                {
                    std::cout
                        << "[Baseline] waiting valid face"
                        << " progress="
                        << baseline_.eyeCalibrationProgress() * 100.0f
                        << "%"
                        << std::endl;
                }
            }
            else
            {
                const EyeBaseline& base =
                    baseline_.eyeBaseline();


                std::cout
                    << "[Baseline] READY"
                    << " leftBase="
                    << base.leftOpenRatio
                    << " rightBase="
                    << base.rightOpenRatio
                    << " samples="
                    << base.sampleCount;


                // ========================================================
                // 必须有完整有效的 FatigueVision observation
                // 才允许进入 normalized state space。
                // ========================================================

                if (eventTrackerUpdated &&
                    eventResult.observationValid)
                {
                    std::cout
                        << " leftState="
                        << eventResult.smoothedLeftOpenness
                        << " rightState="
                        << eventResult.smoothedRightOpenness
                        << " combined="
                        << eventResult.combinedOpenness;
                }
                else
                {
                    std::cout
                        << " observation=INVALID";
                }


                std::cout
                    << std::endl;
            }


            // ========================================================
            // Face / Vision debug
            // ========================================================

            if (result.faceDetected)
            {
                std::cout
                    << "[FatigueService] Face"
                    << " score="
                    << result.faceScore
                    << " rect=("
                    << result.faceRect.x
                    << ","
                    << result.faceRect.y
                    << ","
                    << result.faceRect.width
                    << ","
                    << result.faceRect.height
                    << ")";


                if (result.valid)
                {
                    std::cout
                        << " rightEye="
                        << result.rightEyeRatio
                        << " leftEye="
                        << result.leftEyeRatio
                        << " mouth="
                        << result.mouthRatio;
                }
                else if (result.landmarksValid)
                {
                    std::cout
                        << " PFLD=98"
                        << " ratios=INVALID";
                }
                else
                {
                    std::cout
                        << " PFLD=FAILED";
                }


                std::cout
                    << std::endl;
            }
            else
            {
                std::cout
                    << "[FatigueService] No face"
                    << std::endl;
            }

            if (eventTrackerUpdated)
            {
                std::cout
                    << "[Tracker]"
                    << " state="
                    << eyeStateName(
                        eventResult.eyeState
                    )
                    << " left="
                    << eventResult.smoothedLeftOpenness
                    << " right="
                    << eventResult.smoothedRightOpenness
                    << " combined="
                    << eventResult.combinedOpenness
                    << " slope="
                    << eventResult.opennessSlopePerSecond
                    << " stateMs="
                    << eventResult.stateDurationMs

                    << " closedMs="
                    << eventResult.currentClosedDurationMs

                    << " episodeMs="
                    << eventResult.currentEyeEpisodeDurationMs

                    << std::endl;
            }

            std::cout
                << "[FatiguePerf]"
                << " vision="
                << visionCostMs
                << "ms"
                << " targetPeriod="
                << targetFramePeriodMs
                << "ms"
                << std::endl;
        }


        // ====================================================
        // ============================================================
        // Adaptive target: 30 FPS with/recently after a face, otherwise 15 FPS.
        //
        // Sleep until the target deadline rather than sleeping after work, so
        // processing time is included in the requested frame period.
        //
        // 如果本帧处理超过目标周期，sleep_until 会立即返回，
        // 不再额外等待。
        // ============================================================

        std::this_thread::sleep_until(
            loopStart +
            milliseconds(
                targetFramePeriodMs
            )
        );
    }


    std::cout
        << "[FatigueService] Stopped."
        << std::endl;
}
