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
    // ========================================================
    // 1. Baseline 已经完成
    //
    // 完成后不再继续学习，防止 fatigue drift。
    // ========================================================

    if (eyeBaseline_.ready)
    {
        return;
    }


    // ========================================================
    // 2. 基础有效性检查
    // ========================================================

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


    // ========================================================
    // 3. 时间倒退
    // ========================================================

    if (lastTimestampMs_ > 0 &&
        timestampMs < lastTimestampMs_)
    {
        resetSession();
    }


    lastTimestampMs_ =
        timestampMs;


    // ========================================================
    // 4. 样本质量检查
    //
    // 注意：
    // 必须在 push_back() 之前。
    //
    // 两眼差距过大可能来自：
    // - 侧脸
    // - 单眼遮挡
    // - PFLD 抖动
    // - 关键点异常
    //
    // 这种帧不进入 Open Baseline。
    // ========================================================

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
        return;
    }


    // ========================================================
    // 5. 第一帧有效 calibration observation
    // ========================================================

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
        // ====================================================
        // 6. 有效观测时间累计
        //
        // 只有相邻有效 calibration frame
        // 间隔没有超过 maxValidGapMs 才累计。
        // ====================================================

        const int64_t delta =
            timestampMs -
            previousValidTimestampMs_;


        if (delta > 0 &&
            delta <= config_.maxValidGapMs)
        {
            eyeValidAccumulatedMs_ +=
                delta;
        }


        previousValidTimestampMs_ =
            timestampMs;
    }


    // ========================================================
    // 7. 保存有效 Eye Ratio 样本
    // ========================================================

    leftEyeSamples_.push_back(
        leftEyeRatio
    );


    rightEyeSamples_.push_back(
        rightEyeRatio
    );


    // ========================================================
    // 8. 完成条件
    //
    // 有效观测时间
    // +
    // 有效样本数量
    // ========================================================

    if (eyeValidAccumulatedMs_ >=
            config_.eyeCalibrationDurationMs &&
        leftEyeSamples_.size() >=
            config_.minEyeSamples &&
        rightEyeSamples_.size() >=
            config_.minEyeSamples)
    {
        finalizeEyeBaseline();
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