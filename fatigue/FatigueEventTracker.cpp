#include "FatigueEventTracker.h"

#include <algorithm>

// ============================================================
// Constructor
// ============================================================

FatigueEventTracker::FatigueEventTracker(
    const FatigueEventConfig& config)
    : config_(config)
{
}

// ============================================================
// Config
// ============================================================

void FatigueEventTracker::setConfig(
    const FatigueEventConfig& config)
{
    config_ = config;
}

const FatigueEventConfig&
FatigueEventTracker::config() const
{
    return config_;
}

// ============================================================
// Reset
// ============================================================

void FatigueEventTracker::reset()
{
    eyeState_ = EyeState::UNKNOWN;

    eyeClosureActive_ = false;
    eyeCloseStartMs_ = 0;

    mouthState_ = MouthState::UNKNOWN;

    mouthOpenActive_ = false;
    mouthOpenStartMs_ = 0;
    mouthPeakRatio_ = 0.0f;

    lastTimestampMs_ = -1;
}

// ============================================================
// Face / PFLD 当前帧无效
//
// 第一版采用保守策略：
// 跟踪失败时直接中断当前事件。
//
// 不能出现：
// “摄像头没看到眼睛”
//       ↓
// 被统计成 2 秒长闭眼
// ============================================================

void FatigueEventTracker::invalidateTracking()
{
    eyeState_ = EyeState::UNKNOWN;

    eyeClosureActive_ = false;
    eyeCloseStartMs_ = 0;

    mouthState_ = MouthState::UNKNOWN;

    mouthOpenActive_ = false;
    mouthOpenStartMs_ = 0;
    mouthPeakRatio_ = 0.0f;
}

// ============================================================
// Eye state
// ============================================================

EyeState FatigueEventTracker::resolveEyeState(
    float leftEyeRatio,
    float rightEyeRatio) const
{
    // --------------------------------------------------------
    // 只有双眼都达到闭眼条件，
    // 才认为当前属于“双眼闭合”。
    // --------------------------------------------------------

    const bool bothClosed =
        leftEyeRatio <= config_.eyeCloseThreshold &&
        rightEyeRatio <= config_.eyeCloseThreshold;

    if (bothClosed)
    {
        return EyeState::CLOSED;
    }

    // --------------------------------------------------------
    // 至少一只眼明显睁开，
    // 就不属于双眼闭眼。
    //
    // 这样可以过滤一部分：
    // 单眼眨眼
    // 单眼揉眼
    // 单眼遮挡
    // --------------------------------------------------------

    const bool atLeastOneClearlyOpen =
        leftEyeRatio >= config_.eyeOpenThreshold ||
        rightEyeRatio >= config_.eyeOpenThreshold;

    if (atLeastOneClearlyOpen)
    {
        return EyeState::OPEN;
    }

    // --------------------------------------------------------
    // 中间滞回区
    //
    // 例如：
    // closeThreshold = 0.18
    // openThreshold  = 0.22
    //
    // 0.18 ~ 0.22 之间不立即切换状态，
    // 保持之前状态，防止阈值附近抖动。
    // --------------------------------------------------------

    return eyeState_;
}

// ============================================================
// Mouth state
// ============================================================

MouthState FatigueEventTracker::resolveMouthState(
    float mouthRatio) const
{
    if (mouthRatio >= config_.mouthOpenThreshold)
    {
        return MouthState::OPEN;
    }

    if (mouthRatio <= config_.mouthCloseThreshold)
    {
        return MouthState::CLOSED;
    }

    // 滞回区
    return mouthState_;
}

// ============================================================
// Closure classification
// ============================================================

EyeClosureType FatigueEventTracker::classifyClosure(
    int64_t durationMs) const
{
    if (durationMs < config_.minClosureMs)
    {
        return EyeClosureType::NONE;
    }

    if (durationMs >= config_.strongLongCloseMs)
    {
        return EyeClosureType::STRONG_LONG_CLOSE;
    }

    if (durationMs >= config_.longCloseMs)
    {
        return EyeClosureType::LONG_CLOSE;
    }

    return EyeClosureType::BLINK;
}

// ============================================================
// Update
// ============================================================

FatigueEventResult FatigueEventTracker::update(
    const FatigueFrameInput& frame)
{
    FatigueEventResult result;

    // ========================================================
    // 1. 时间戳检查
    // ========================================================

    if (lastTimestampMs_ >= 0 &&
        frame.timestampMs < lastTimestampMs_)
    {
        // 时间发生倒退，旧的时间事件全部失效
        reset();
    }

    lastTimestampMs_ = frame.timestampMs;

    // ========================================================
    // 2. 当前视觉结果无效
    // ========================================================

    if (!frame.valid)
    {
        invalidateTracking();

        result.valid = false;

        result.eyeState =
            EyeState::UNKNOWN;

        result.mouthState =
            MouthState::UNKNOWN;

        return result;
    }

    result.valid = true;

    // ========================================================
    // 3. Eye
    // ========================================================

    const EyeState newEyeState =
        resolveEyeState(
            frame.leftEyeRatio,
            frame.rightEyeRatio);

    // --------------------------------------------------------
    // 第一次获得有效 EyeState
    // --------------------------------------------------------

    if (eyeState_ == EyeState::UNKNOWN)
    {
        eyeState_ = newEyeState;

        // 程序开始时驾驶员刚好闭眼
        if (eyeState_ == EyeState::CLOSED)
        {
            eyeClosureActive_ = true;
            eyeCloseStartMs_ = frame.timestampMs;
        }
    }

    // --------------------------------------------------------
    // OPEN -> CLOSED
    // 开始一次闭眼事件
    // --------------------------------------------------------

    else if (
        eyeState_ == EyeState::OPEN &&
        newEyeState == EyeState::CLOSED)
    {
        eyeState_ = EyeState::CLOSED;

        eyeClosureActive_ = true;

        eyeCloseStartMs_ =
            frame.timestampMs;
    }

    // --------------------------------------------------------
    // CLOSED -> OPEN
    // 一次闭眼事件结束
    // --------------------------------------------------------

    else if (
        eyeState_ == EyeState::CLOSED &&
        newEyeState == EyeState::OPEN)
    {
        eyeState_ = EyeState::OPEN;

        if (eyeClosureActive_)
        {
            const int64_t durationMs =
                frame.timestampMs -
                eyeCloseStartMs_;

            result.eyeClosure.completed = true;

            result.eyeClosure.startMs =
                eyeCloseStartMs_;

            result.eyeClosure.durationMs =
                durationMs;

            result.eyeClosure.type =
                classifyClosure(durationMs);

            eyeClosureActive_ = false;
            eyeCloseStartMs_ = 0;
        }
    }

    // --------------------------------------------------------
    // 当前 Eye 状态
    // --------------------------------------------------------

    result.eyeState = eyeState_;

    // --------------------------------------------------------
    // 当前还没有睁眼时，实时计算闭眼时间
    //
    // 以后绝对危险阈值：
    //
    // if (currentCloseMs >= 2000)
    //      EMERGENCY
    //
    // 就靠这里。
    // --------------------------------------------------------

    if (
        eyeClosureActive_ &&
        eyeState_ == EyeState::CLOSED)
    {
        result.currentCloseMs =
            frame.timestampMs -
            eyeCloseStartMs_;
    }
    else
    {
        result.currentCloseMs = 0;
    }

    // ========================================================
    // 4. Mouth
    // ========================================================

    const MouthState newMouthState =
        resolveMouthState(
            frame.mouthRatio);

    // --------------------------------------------------------
    // 第一次状态
    // --------------------------------------------------------

    if (mouthState_ == MouthState::UNKNOWN)
    {
        mouthState_ = newMouthState;

        if (mouthState_ == MouthState::OPEN)
        {
            mouthOpenActive_ = true;

            mouthOpenStartMs_ =
                frame.timestampMs;

            mouthPeakRatio_ =
                frame.mouthRatio;
        }
    }

    // --------------------------------------------------------
    // CLOSED -> OPEN
    // --------------------------------------------------------

    else if (
        mouthState_ == MouthState::CLOSED &&
        newMouthState == MouthState::OPEN)
    {
        mouthState_ = MouthState::OPEN;

        mouthOpenActive_ = true;

        mouthOpenStartMs_ =
            frame.timestampMs;

        mouthPeakRatio_ =
            frame.mouthRatio;
    }

    // --------------------------------------------------------
    // OPEN -> OPEN
    // 更新张嘴峰值
    // --------------------------------------------------------

    else if (
        mouthState_ == MouthState::OPEN &&
        newMouthState == MouthState::OPEN)
    {
        if (mouthOpenActive_)
        {
            mouthPeakRatio_ =
                std::max(
                    mouthPeakRatio_,
                    frame.mouthRatio);
        }
    }

    // --------------------------------------------------------
    // OPEN -> CLOSED
    //
    // 完成一次完整张嘴事件。
    // 达到持续时间要求才认为 Yawn。
    // --------------------------------------------------------

    else if (
        mouthState_ == MouthState::OPEN &&
        newMouthState == MouthState::CLOSED)
    {
        mouthState_ = MouthState::CLOSED;

        if (mouthOpenActive_)
        {
            const int64_t durationMs =
                frame.timestampMs -
                mouthOpenStartMs_;

            if (
                durationMs >=
                config_.yawnMinDurationMs)
            {
                result.yawn.detected = true;

                result.yawn.startMs =
                    mouthOpenStartMs_;

                result.yawn.durationMs =
                    durationMs;

                result.yawn.peakMouthRatio =
                    mouthPeakRatio_;
            }

            mouthOpenActive_ = false;
            mouthOpenStartMs_ = 0;
            mouthPeakRatio_ = 0.0f;
        }
    }

    result.mouthState =
        mouthState_;

    // --------------------------------------------------------
    // 实时张嘴时间
    // --------------------------------------------------------

    if (
        mouthOpenActive_ &&
        mouthState_ == MouthState::OPEN)
    {
        result.currentMouthOpenMs =
            frame.timestampMs -
            mouthOpenStartMs_;
    }
    else
    {
        result.currentMouthOpenMs = 0;
    }

    return result;
}