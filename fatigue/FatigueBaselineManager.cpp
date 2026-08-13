#include "FatigueBaselineManager.h"

#include <algorithm>
#include <cmath>


// ============================================================
// Constructor
// ============================================================

FatigueBaselineManager::FatigueBaselineManager(
    const FatigueBaselineConfig& config
)
    : config_(config)
{
}


// ============================================================
// Reset current session
// ============================================================

void FatigueBaselineManager::resetSession()
{
    eyeBaseline_ =
        EyeBaseline();


    eyeCalibrationStarted_ = false;

    eyeCalibrationStartMs_ = 0;

    lastTimestampMs_ = 0;
    eyeValidAccumulatedMs_ = 0;

    previousValidTimestampMs_ = 0;

    leftEyeSamples_.clear();

    rightEyeSamples_.clear();
}


float FatigueBaselineManager::percentile(
    std::vector<float> values,
    float q
)
{
    if (values.empty())
    {
        return 0.0f;
    }


    q = std::clamp(
        q,
        0.0f,
        1.0f
    );


    std::sort(
        values.begin(),
        values.end()
    );


    const float position =
        q *
        static_cast<float>(
            values.size() - 1
        );


    const std::size_t lower =
        static_cast<std::size_t>(
            std::floor(position)
        );

    const std::size_t upper =
        static_cast<std::size_t>(
            std::ceil(position)
        );


    if (lower == upper)
    {
        return values[lower];
    }


    const float fraction =
        position -
        static_cast<float>(lower);


    return
        values[lower] *
            (1.0f - fraction)
        +
        values[upper] *
            fraction;
}

// ============================================================
// Finalize baseline
// ============================================================

void FatigueBaselineManager::finalizeEyeBaseline()
{
    if (leftEyeSamples_.size() <
            config_.minEyeSamples ||
        rightEyeSamples_.size() <
            config_.minEyeSamples)
    {
        return;
    }


    const float leftOpen =
        percentile(
            leftEyeSamples_,
            config_.eyeOpenPercentile
        );


    const float rightOpen =
        percentile(
            rightEyeSamples_,
            config_.eyeOpenPercentile
        );

    if (!std::isfinite(leftOpen) ||
        !std::isfinite(rightOpen) ||
        leftOpen <= 1e-6f ||
        rightOpen <= 1e-6f)
    {
        return;
    }


    eyeBaseline_.leftOpenRatio =
        leftOpen;

    eyeBaseline_.rightOpenRatio =
        rightOpen;

    eyeBaseline_.sampleCount =
        std::min(
            leftEyeSamples_.size(),
            rightEyeSamples_.size()
        );

    eyeBaseline_.ready = true;
}


// ============================================================
// Update calibration
// ============================================================

void FatigueBaselineManager::updateEyeCalibration(
    float leftEyeRatio,
    float rightEyeRatio,
    int64_t timestampMs
)
{
    // 基线已经完成，不再继续修改。
    //
    // 防止驾驶员后面疲劳后，
    // 基线慢慢漂移成“疲劳状态”。
    if (eyeBaseline_.ready)
    {
        return;
    }


    // --------------------------------------------------------
    // 基础有效性检查
    //
    // 注意：
    // 这里只检查数据是否合理，
    // 不使用固定 OPEN/CLOSED 阈值。
    // --------------------------------------------------------

    if (!std::isfinite(leftEyeRatio) ||
        !std::isfinite(rightEyeRatio))
    {
        return;
    }

    

    if (leftEyeRatio <= 0.0f ||
        rightEyeRatio <= 0.0f)
    {
        return;
    }


    if (timestampMs <= 0)
    {
        return;
    }


    // --------------------------------------------------------
    // 时间倒退，重新开始当前 calibration
    // --------------------------------------------------------

    if (lastTimestampMs_ > 0 &&
        timestampMs < lastTimestampMs_)
    {
        resetSession();
    }


    lastTimestampMs_ =
        timestampMs;


    // --------------------------------------------------------
    // 第一帧有效观测开始计时
    // --------------------------------------------------------

    if (!eyeCalibrationStarted_)
    {
        eyeCalibrationStarted_ = true;

        eyeCalibrationStartMs_ =
            timestampMs;
    }


    // --------------------------------------------------------
    // 保存连续观测
    //
    // 使用中位数而不是均值，
    // 普通短暂眨眼不会严重拉低正常睁眼基线。
    // --------------------------------------------------------

    leftEyeSamples_.push_back(
        leftEyeRatio
    );

    rightEyeSamples_.push_back(
        rightEyeRatio
    );


    if (!eyeCalibrationStarted_)
    {
        eyeCalibrationStarted_ = true;

        eyeCalibrationStartMs_ =
            timestampMs;

        previousValidTimestampMs_ =
            timestampMs;
    }
    else
    {
        const int64_t delta =
            timestampMs -
            previousValidTimestampMs_;


        // 只有连续有效观测才累计时间。
        //
        // 如果中间很久没有 Face，
        // 那段时间不进入 calibration。
        if (delta > 0 &&
            delta <= config_.maxValidGapMs)
        {
            eyeValidAccumulatedMs_ +=
                delta;
        }


        previousValidTimestampMs_ =
            timestampMs;
    }


    // --------------------------------------------------------
    // 同时满足：
    //
    // 时间达到
    // +
    // 样本数量足够
    //
    // 才完成 calibration。
    // --------------------------------------------------------

    if (eyeValidAccumulatedMs_ >=
            config_.eyeCalibrationDurationMs &&
        leftEyeSamples_.size() >=
            config_.minEyeSamples &&
        rightEyeSamples_.size() >=
            config_.minEyeSamples)
    {
        finalizeEyeBaseline();
    }

    const float eyeMean =
    (
        leftEyeRatio +
        rightEyeRatio
    ) * 0.5f;


    if (eyeMean <= 1e-6f)
    {
        return;
    }


    const float eyeAsymmetry =
        std::fabs(
            leftEyeRatio -
            rightEyeRatio
        )
        /
        eyeMean;


    if (eyeAsymmetry >
        config_.maxEyeAsymmetry)
    {
        // 两只眼当前差距过大，
        // 可能是侧脸、关键点异常、遮挡等。
        //
        // 当前帧不用于个人 Open Baseline。
        return;
    }
}


// ============================================================
// Ready
// ============================================================

bool FatigueBaselineManager::eyeBaselineReady() const
{
    return eyeBaseline_.ready;
}


// ============================================================
// Calibration progress
// ============================================================

float FatigueBaselineManager::eyeCalibrationProgress() const
{
    if (eyeBaseline_.ready)
    {
        return 1.0f;
    }


    if (!eyeCalibrationStarted_)
    {
        return 0.0f;
    }


    if (config_.eyeCalibrationDurationMs <= 0)
    {
        return 0.0f;
    }


    const int64_t elapsed =
        eyeValidAccumulatedMs_;


    float timeProgress =
        static_cast<float>(elapsed)
        /
        static_cast<float>(
            config_.eyeCalibrationDurationMs
        );


    timeProgress =
        std::clamp(
            timeProgress,
            0.0f,
            1.0f
        );


    float sampleProgress = 1.0f;


    if (config_.minEyeSamples > 0)
    {
        sampleProgress =
            static_cast<float>(
                leftEyeSamples_.size()
            )
            /
            static_cast<float>(
                config_.minEyeSamples
            );


        sampleProgress =
            std::clamp(
                sampleProgress,
                0.0f,
                1.0f
            );
    }


    // 时间和有效样本两个条件都要满足
    return
        std::min(
            timeProgress,
            sampleProgress
        );
}


// ============================================================
// Baseline
// ============================================================

const EyeBaseline&
FatigueBaselineManager::eyeBaseline() const
{
    return eyeBaseline_;
}


// ============================================================
// Normalize current eye observation
// ============================================================

NormalizedEyeObservation
FatigueBaselineManager::normalizeEyes(
    float leftEyeRatio,
    float rightEyeRatio
) const
{
    NormalizedEyeObservation result;


    if (!eyeBaseline_.ready)
    {
        return result;
    }


    if (!std::isfinite(leftEyeRatio) ||
        !std::isfinite(rightEyeRatio))
    {
        return result;
    }

    // ============================================================
    // 无效视觉观测
    //
    // No Face / PFLD Failure 时 FatigueVisionResult 中
    // EyeRatio 默认可能为 0。
    //
    // 0 不能被解释为“眼睛完全闭合”，
    // 必须作为 INVALID observation。
    // ============================================================

    if (leftEyeRatio <= 0.0f ||
        rightEyeRatio <= 0.0f)
    {
        return result;
    }

    if (eyeBaseline_.leftOpenRatio <=
            1e-6f ||
        eyeBaseline_.rightOpenRatio <=
            1e-6f)
    {
        return result;
    }


    result.leftOpenness =
        leftEyeRatio
        /
        eyeBaseline_.leftOpenRatio;


    result.rightOpenness =
        rightEyeRatio
        /
        eyeBaseline_.rightOpenRatio;


    if (!std::isfinite(
            result.leftOpenness) ||
        !std::isfinite(
            result.rightOpenness))
    {
        return NormalizedEyeObservation();
    }


    // 不 clamp。
    //
    // >1.0 也是有意义的，
    // 表示当前眼睛比 calibration baseline
    // 睁得更大。
    result.valid = true;


    return result;
}