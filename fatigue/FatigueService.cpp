#include "FatigueService.h"

#include <chrono>
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <algorithm>
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
    CameraService* camera
)
    : camera_(camera)
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

    baseline_.resetSession();
    eventTracker_.reset();
    running_ = true;


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
    running_ = false;


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
}


// ============================================================
// State
// ============================================================

bool FatigueService::isRunning() const
{
    return running_;
}


// ============================================================
// Processing thread
// ============================================================

void FatigueService::processLoop()
{
    using namespace std::chrono;


    // ========================================================
    // 疲劳检测目标采样率
    //
    // 20 FPS
    // ≈ 每 50ms 处理一次
    //
    // 注意：
    // 不是“处理完以后再睡 50ms”，
    // 而是一整个循环目标为 50ms。
    // ========================================================

    constexpr int64_t targetFramePeriodMs = 33;


    int printCounter = 0;


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

        if (eventTrackerUpdated &&
            eventResult.emergencyTriggered)
        {
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

        ++printCounter;


        // 大约每 10 次打印一次，避免终端刷屏
        if (printCounter >= 10)
        {
            printCounter = 0;


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
        // Target 20 FPS
        //
        // 一个完整循环目标：50ms。
        //
        // 如果本帧工作只用了 30ms：
        // 再等待约 20ms。
        //
        // 如果本帧已经用了 60ms：
        // sleep_until 会立即返回，
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