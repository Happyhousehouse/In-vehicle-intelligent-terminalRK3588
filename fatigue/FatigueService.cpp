#include "FatigueService.h"

#include <chrono>
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

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


    int printCounter = 0;

    // 临时调试：只保存一张关键点图
    bool landmarkDebugSaved = false;

    while (running_)
    {
        // ====================================================
        // 1. 从现有 CameraService 获取最新帧
        // ====================================================

        cv::Mat frame =
            camera_->getFrame();


        if (frame.empty())
        {
            std::this_thread::sleep_for(
                milliseconds(50)
            );

            continue;
        }


        // ====================================================
        // 2. FatigueVision
        // ====================================================

        FatigueVisionResult result =
            vision_.process(frame);

        const int64_t timestampMs =
            duration_cast<milliseconds>(
                steady_clock::now()
                    .time_since_epoch()
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

                if (result.valid)
                {
                    const NormalizedEyeObservation normalized =
                        baseline_.normalizeEyes(
                            result.leftEyeRatio,
                            result.rightEyeRatio
                        );


                    if (normalized.valid)
                    {
                        std::cout
                            << " leftState="
                            << normalized.leftOpenness
                            << " rightState="
                            << normalized.rightOpenness;
                    }
                    else
                    {
                        std::cout
                            << " observation=INVALID";
                    }
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
        }


        // ====================================================
        // 4. 当前疲劳检测先跑约 10 FPS
        //
        // 没必要跟 QML 的 30 FPS 完全同步。
        // 后面根据 Face + PFLD 实际性能再调整。
        // ====================================================

        std::this_thread::sleep_for(
            milliseconds(100)
        );
    }


    std::cout
        << "[FatigueService] Stopped."
        << std::endl;
}