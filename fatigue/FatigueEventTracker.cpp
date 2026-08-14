#include "FatigueEventTracker.h"

#include <algorithm>


// ============================================================
// Constructor
// ============================================================

FatigueEventTracker::FatigueEventTracker(
    const FatigueEventConfig& config
)
    : config_(config)
{
}


// ============================================================
// Config
// ============================================================

void FatigueEventTracker::setConfig(
    const FatigueEventConfig& config
)
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
    // --------------------------------------------------------
    // Eye EMA
    // --------------------------------------------------------

    eyeEmaInitialized_ = false;

    prolongedLowStartMs_ = -1;
    prolongedLowEmergencyLatched_ = false;

    smoothedLeftOpenness_ = 0.0f;
    smoothedRightOpenness_ = 0.0f;

    previousCombinedOpenness_ = 0.0f;
    previousValidTimestampMs_ = -1;


    // --------------------------------------------------------
    // Eye state
    // --------------------------------------------------------

    eyeState_ = EyeState::UNKNOWN;

    eyeStateStartMs_ = -1;


    // --------------------------------------------------------
    // Eye episode
    // --------------------------------------------------------

    eyeEpisodeActive_ = false;

    eyeEpisodeStartMs_ = -1;

    eyeClosedStartMs_ = -1;
    eyeClosedEndMs_ = -1;

    closedCandidateStartMs_ = -1;
    openCandidateStartMs_ = -1;

    minLeftOpenness_ = 0.0f;
    minRightOpenness_ = 0.0f;

    emergencyLatched_ = false;


    // --------------------------------------------------------
    // Invalid
    // --------------------------------------------------------

    invalidStartMs_ = -1;


    // --------------------------------------------------------
    // Mouth
    // --------------------------------------------------------

    resetMouthTracking();


    // --------------------------------------------------------
    // Timestamp
    // --------------------------------------------------------

    lastTimestampMs_ = -1;
}


// ============================================================
// Closure classification
// ============================================================

EyeClosureType FatigueEventTracker::classifyClosure(
    int64_t closedDurationMs,
    int64_t totalDurationMs) const
{
    // ========================================================
    // 1. 太短：关键点抖动 / 无效 closure
    // ========================================================

    if (closedDurationMs <
        config_.minClosureMs)
    {
        return EyeClosureType::NONE;
    }


    // ========================================================
    // 2. 真正 CLOSED >= 1000ms
    //
    // ECD 已经足够严重时，
    // totalDuration 不再影响它的类别。
    // ========================================================

    if (closedDurationMs >=
        config_.strongLongCloseMs)
    {
        return
            EyeClosureType::
                STRONG_LONG_CLOSE;
    }


    // ========================================================
    // 3. 真正 CLOSED >= 500ms
    // ========================================================

    if (closedDurationMs >=
        config_.longCloseMs)
    {
        return
            EyeClosureType::
                LONG_CLOSE;
    }


    // ========================================================
    // 到这里意味着：
    //
    // minClosureMs
    // <= ECD
    // < longCloseMs
    //
    // 以前这里全部直接认为 BLINK。
    //
    // 现在还必须检查完整动作时间。
    // ========================================================

    if (totalDurationMs >
        config_.maxBlinkTotalMs)
    {
        return
            EyeClosureType::
                SLOW_CLOSURE;
    }


    // ========================================================
    // ECD 短，并且完整动作也足够快，
    // 才认为是真正普通 Blink。
    // ========================================================

    return EyeClosureType::BLINK;
}


// ============================================================
// Begin eye closure episode
//
// OPEN
//   ↓
// CLOSING
// ============================================================

void FatigueEventTracker::beginEyeEpisode(
    int64_t timestampMs,
    float leftOpenness,
    float rightOpenness
)
{
    eyeEpisodeActive_ = true;

    eyeEpisodeStartMs_ =
        timestampMs;

    eyeClosedStartMs_ = -1;
    eyeClosedEndMs_ = -1;

    closedCandidateStartMs_ = -1;
    openCandidateStartMs_ = -1;

    minLeftOpenness_ =
        leftOpenness;

    minRightOpenness_ =
        rightOpenness;

    emergencyLatched_ = false;
}


// ============================================================
// Update episode minimum openness
// ============================================================

void FatigueEventTracker::updateEyeEpisodeMinimum(
    float leftOpenness,
    float rightOpenness
)
{
    if (!eyeEpisodeActive_)
    {
        return;
    }

    minLeftOpenness_ =
        std::min(
            minLeftOpenness_,
            leftOpenness
        );

    minRightOpenness_ =
        std::min(
            minRightOpenness_,
            rightOpenness
        );
}


// ============================================================
// Cancel current eye episode
// ============================================================

void FatigueEventTracker::cancelEyeEpisode()
{
    eyeEpisodeActive_ = false;

    eyeEpisodeStartMs_ = -1;

    eyeClosedStartMs_ = -1;
    eyeClosedEndMs_ = -1;

    closedCandidateStartMs_ = -1;
    openCandidateStartMs_ = -1;

    minLeftOpenness_ = 0.0f;
    minRightOpenness_ = 0.0f;

    emergencyLatched_ = false;
}


// ============================================================
// Long invalid observation
//
// No Face / PFLD failure > maxInvalidGapMs
//
// 当前 episode 作废。
// 下一次有效观测重新从 UNKNOWN 建立状态。
// ============================================================

void FatigueEventTracker::invalidateEyeTracking()
{
    eyeState_ =
        EyeState::UNKNOWN;

    eyeStateStartMs_ = -1;

    cancelEyeEpisode();


    // 长时间丢脸后 EMA 也重新建立。
    //
    // 否则重新出现人脸时，新的 openness
    // 会和几百毫秒甚至更早的数据混合。
    eyeEmaInitialized_ = false;

    smoothedLeftOpenness_ = 0.0f;
    smoothedRightOpenness_ = 0.0f;

    previousCombinedOpenness_ = 0.0f;
    previousValidTimestampMs_ = -1;

    prolongedLowStartMs_ = -1;
    prolongedLowEmergencyLatched_ = false;
}


// ============================================================
// Mouth state
//
// 当前仍保留简单 hysteresis。
// 完整 Yawn state machine 后面单独实现。
// ============================================================

MouthState FatigueEventTracker::resolveMouthState(
    float mouthRatio
) const
{
    if (mouthRatio >=
        config_.mouthOpenThreshold)
    {
        return MouthState::OPEN;
    }

    if (mouthRatio <=
        config_.mouthCloseThreshold)
    {
        return MouthState::CLOSED;
    }

    // hysteresis zone
    return mouthState_;
}


// ============================================================
// Reset mouth tracking
// ============================================================

void FatigueEventTracker::resetMouthTracking()
{
    mouthState_ =
        MouthState::UNKNOWN;

    mouthOpenActive_ = false;

    mouthOpenStartMs_ = -1;

    mouthOpenAccumMs_ = 0;

    mouthLastTimestampMs_ = -1;

    mouthWasOpen_ = false;

    mouthPeakRatio_ = 0.0f;

    yawnLatched_ = false;
}


// ============================================================
// Update
// ============================================================

FatigueEventResult FatigueEventTracker::update(
    const FatigueObservation& observation
)
{
    FatigueEventResult result;

    const int64_t timestampMs =
        observation.timestampMs;


    // ========================================================
    // 1. Timestamp validation
    // ========================================================

    if (lastTimestampMs_ >= 0 &&
        timestampMs < lastTimestampMs_)
    {
        // monotonic clock 不应该倒退。
        //
        // 如果发生，所有旧的时序 episode
        // 都不能继续使用。
        reset();
    }

    lastTimestampMs_ =
        timestampMs;


    // ========================================================
    // 2. Invalid observation
    //
    // 短暂丢失：
    //
    // <= maxInvalidGapMs
    // 保留 episode
    //
    // 长时间丢失：
    //
    // > maxInvalidGapMs
    // 作废并回 UNKNOWN
    // ========================================================

    if (!observation.valid)
    {
        if (invalidStartMs_ < 0)
        {
            invalidStartMs_ =
                timestampMs;
        }


        const int64_t invalidDurationMs =
            timestampMs -
            invalidStartMs_;


        if (invalidDurationMs >
            config_.maxInvalidGapMs)
        {
            // Eye：
            // INVALID 超过 maxInvalidGapMs 后，
            // 当前眼睛 episode 不能继续拼接，
            // 所以眼睛回 UNKNOWN。
            invalidateEyeTracking();


            // Mouth：
            // 不调用 resetMouthTracking()。
            //
            // Yawn 使用固定 5 秒真实时间窗口：
            //
            // INVALID 期间：
            //   - 5 秒窗口继续走
            //   - 不增加 mouthOpenAccumMs_
            //
            // 所以不能因为 INVALID > 500ms
            // 就把当前 Mouth 窗口清除。
        }


        result.observationValid = false;

        result.eyeState =
            eyeState_;

        result.mouthState =
            mouthState_;

        result.smoothedLeftOpenness =
            smoothedLeftOpenness_;

        result.smoothedRightOpenness =
            smoothedRightOpenness_;

        result.combinedOpenness =
            std::max(
                smoothedLeftOpenness_,
                smoothedRightOpenness_
            );


        if (eyeStateStartMs_ >= 0)
        {
            result.stateDurationMs =
                timestampMs -
                eyeStateStartMs_;
        }


        // 当前帧没有有效眼睛证据，
        // 不允许在 invalid frame 上继续触发
        // continuous close / emergency。
        result.currentClosedDurationMs = 0;

        result.currentMouthOpenMs = 0;


        // --------------------------------------------------------
        // Invalid 时间不能被算进“张嘴时间”。
        //
        // 5 秒窗口本身继续走真实时间，
        // 但是 No Face 这段时间不能算 mouthRatio >= 0.80。
        // --------------------------------------------------------

        mouthLastTimestampMs_ =
            timestampMs;

        mouthWasOpen_ = false;


        return result;
    }


    // 当前重新获得有效视觉观测
    const bool resumedAfterInvalid =
        invalidStartMs_ >= 0;

    invalidStartMs_ = -1;

    result.observationValid = true;


    // ========================================================
    // 3. Eye EMA
    // ========================================================

    const float rawLeft =
        observation.leftEyeOpenness;

    const float rawRight =
        observation.rightEyeOpenness;

    const float rawCombined =
        std::max(
            rawLeft,
            rawRight
        );

    if (!eyeEmaInitialized_)
    {
        smoothedLeftOpenness_ =
            rawLeft;

        smoothedRightOpenness_ =
            rawRight;

        eyeEmaInitialized_ = true;
    }
    else
    {
        const float alpha =
            std::clamp(
                config_.emaAlpha,
                0.0f,
                1.0f
            );


        smoothedLeftOpenness_ =
            alpha * rawLeft
            +
            (1.0f - alpha) *
                smoothedLeftOpenness_;


        smoothedRightOpenness_ =
            alpha * rawRight
            +
            (1.0f - alpha) *
                smoothedRightOpenness_;
    }


    // ========================================================
    // 4. Combined eye openness
    //
    // 注意：
    //
    // 这里不是平均值。
    //
    // combined = max(left, right)
    //
    // 只有左右眼都低时，
    // combined 才真正低。
    // ========================================================

    const float combinedOpenness =
        std::max(
            smoothedLeftOpenness_,
            smoothedRightOpenness_
        );


    result.smoothedLeftOpenness =
        smoothedLeftOpenness_;

    result.smoothedRightOpenness =
        smoothedRightOpenness_;

    result.combinedOpenness =
        combinedOpenness;

    // ============================================================
    // Prolonged Low Openness Emergency
    //
    // 与严格 CLOSED 状态机完全独立。
    //
    // 使用 EMA 后的 combinedOpenness：
    //     目的不是识别快速 Blink，
    //     而是识别持续 2 秒以上的低开合状态，
    //     因此 EMA 在这里反而可以帮助过滤关键点抖动。
    //
    // combinedOpenness 使用左右眼 max，
    // 所以仍然要求“两只眼都低”，避免单眼遮挡直接报警。
    // ============================================================

    if (combinedOpenness <=
        config_.prolongedLowEnterRatio)
    {
        // --------------------------------------------------------
        // 第一次进入低开合区域
        // --------------------------------------------------------

        if (prolongedLowStartMs_ < 0)
        {
            prolongedLowStartMs_ =
                timestampMs;

            prolongedLowEmergencyLatched_ =
                false;
        }


        result.currentLowOpennessDurationMs =
            timestampMs -
            prolongedLowStartMs_;


        // --------------------------------------------------------
        // 持续低开合 >= 2 秒：
        // 立即 Emergency。
        //
        // one-shot：
        // 同一个 low episode 只触发一次。

    }
    else if (
        combinedOpenness >=
        config_.prolongedLowExitRatio)
    {
        // --------------------------------------------------------
        // 已真正恢复睁眼。
        //
        // 使用 0.75 而不是 0.65：
        // 形成 hysteresis（滞回），
        // 避免 0.64 / 0.66 / 0.63 这种边界抖动
        // 不断重启计时器。
        // --------------------------------------------------------

        prolongedLowStartMs_ =
            -1;

        prolongedLowEmergencyLatched_ =
            false;

        result.currentLowOpennessDurationMs =
            0;
    }
    else
    {
        // --------------------------------------------------------
        // 0.65 ~ 0.75 滞回区
        //
        // 如果之前已经进入 Low，则继续同一个 episode；
        // 如果之前没有进入，则什么都不做。
        // --------------------------------------------------------

        if (prolongedLowStartMs_ >= 0)
        {
            result.currentLowOpennessDurationMs =
                timestampMs -
                prolongedLowStartMs_;
        }
    }
    // ========================================================
    // 5. Openness slope
    //
    // 单位：
    // openness / second
    // ========================================================

    float slopePerSecond = 0.0f;


    if (!resumedAfterInvalid &&
        previousValidTimestampMs_ >= 0)
    {
        const int64_t deltaMs =
            timestampMs -
            previousValidTimestampMs_;


        if (deltaMs > 0)
        {
            slopePerSecond =
                (
                    combinedOpenness -
                    previousCombinedOpenness_
                )
                *
                1000.0f
                /
                static_cast<float>(
                    deltaMs
                );
        }
    }


    result.opennessSlopePerSecond =
        slopePerSecond;


    previousCombinedOpenness_ =
        combinedOpenness;

    previousValidTimestampMs_ =
        timestampMs;
    
    // ============================================================
    // Eye episode warning
    //
    // episode 从：
    // OPEN -> CLOSING
    //
    // 开始计时。
    //
    // 规则：
    //
    // 1. 如果一直只有 CLOSING：
    //      >= 4000ms 警告
    //
    // 2. 如果本次 episode 已经出现过 CLOSED：
    //      门槛缩短为 2000ms
    //
    // 注意：
    // CLOSED 后进入 OPENING，仍属于同一个 episode。
    // ============================================================

    if (eyeEpisodeActive_ &&
        eyeEpisodeStartMs_ >= 0)
    {
        const int64_t episodeDurationMs =
            timestampMs -
            eyeEpisodeStartMs_;


        result.currentEyeEpisodeDurationMs =
            episodeDurationMs;


        // eyeClosedStartMs_ >= 0
        // 表示当前 episode 曾经真正进入过 CLOSED。
        const bool hasEnteredClosed =
            eyeClosedStartMs_ >= 0;


        const int64_t warningThresholdMs =
            hasEnteredClosed
                ? config_.closedWarningMs
                : config_.closingWarningMs;


        if (!emergencyLatched_ &&
            episodeDurationMs >=
                warningThresholdMs)
        {
            result.emergencyTriggered =
                true;

            emergencyLatched_ =
                true;
        }
    }
    // ========================================================
    // 6. Eye State Machine
    // ========================================================

    auto enterEyeState =
        [this, timestampMs](
            EyeState state)
        {
            eyeState_ = state;

            eyeStateStartMs_ =
                timestampMs;
        };


    // --------------------------------------------------------
    // UNKNOWN
    //
    // 第一次必须先看到一个明确 OPEN，
    // 才正式建立 tracking。
    //
    // 这样程序启动时的一帧异常低值
    // 不会直接生成假闭眼事件。
    // --------------------------------------------------------

    if (eyeState_ == EyeState::UNKNOWN)
    {
        if (combinedOpenness >=
            config_.openReturnRatio)
        {
            enterEyeState(
                EyeState::OPEN
            );
        }
    }


    // --------------------------------------------------------
    // OPEN
    //
    // 开始向关闭方向移动：
    //
    // 1. 已经非常低
    //
    // OR
    //
    // 2. 低于 openExitRatio
    //    并且明显向下变化
    // --------------------------------------------------------

    else if (eyeState_ == EyeState::OPEN)
    {
        const bool stronglyLow =
            combinedOpenness <=
            config_.closedEnterRatio;


        const bool clearlyClosing =
            combinedOpenness <
                config_.openExitRatio
            &&
            slopePerSecond <=
                config_.closingSlopePerSecond;


        if (stronglyLow ||
            clearlyClosing)
        {
            beginEyeEpisode(
                timestampMs,
                smoothedLeftOpenness_,
                smoothedRightOpenness_
            );


            // --------------------------------------------------------
            // 进入 CLOSING 的这一帧如果 raw 已经真正进入闭眼区域，
            // 必须从这一帧就开始 CLOSED confirmation。
            //
            // 否则会平白丢掉第一个低值采样点。
            // --------------------------------------------------------

            if (rawCombined <=
                config_.closedEnterRatio)
            {
                closedCandidateStartMs_ =
                    timestampMs;
            }


            enterEyeState(
                EyeState::CLOSING
            );
        }
    }


    // --------------------------------------------------------
    // CLOSING
    // --------------------------------------------------------

    else if (
        eyeState_ ==
        EyeState::CLOSING)
    {
        updateEyeEpisodeMinimum(
            smoothedLeftOpenness_,
            smoothedRightOpenness_
        );


        // ----------------------------------------------------
        // 还没有真正闭合，就重新睁开。
        //
        // 本次 episode 取消。
        // ----------------------------------------------------

        const bool clearlyRecovered =
            combinedOpenness >=
                config_.openReturnRatio
            ||
            (
                combinedOpenness >=
                    config_.openExitRatio
                &&
                slopePerSecond >=
                    config_.openingSlopePerSecond
            );


        if (clearlyRecovered)
        {
            // ========================================================
            // 普通眨眼
            //
            // 实际测试发现正常 Blink 很可能是：
            //
            // OPEN
            //   ↓
            // CLOSING
            //   ↓
            // OPEN
            //
            // 并不一定真正进入 CLOSED。
            //
            // 因此：
            // 只要一次有效 CLOSING 最终恢复 OPEN，
            // 并且期间没有进入过 CLOSED，
            // 就完成一次普通 BLINK。
            // ========================================================

            if (eyeEpisodeActive_ &&
                eyeEpisodeStartMs_ >= 0 &&
                eyeClosedStartMs_ < 0 &&
                (timestampMs - eyeEpisodeStartMs_) <
                    config_.closingWarningMs)
            {
                result.eyeClosure.completed =
                    true;

                result.eyeClosure.type =
                    EyeClosureType::BLINK;

                result.eyeClosure.startMs =
                    eyeEpisodeStartMs_;

                result.eyeClosure.closedStartMs =
                    0;

                result.eyeClosure.closedEndMs =
                    0;

                result.eyeClosure.endMs =
                    timestampMs;

                result.eyeClosure.closedDurationMs =
                    0;

                result.eyeClosure.totalDurationMs =
                    timestampMs -
                    eyeEpisodeStartMs_;

                result.eyeClosure.minLeftOpenness =
                    minLeftOpenness_;

                result.eyeClosure.minRightOpenness =
                    minRightOpenness_;
            }


            cancelEyeEpisode();


            enterEyeState(
                EyeState::OPEN
            );
        }
        else
        {
            // ------------------------------------------------
            // CLOSED candidate
            // ------------------------------------------------

            if (rawCombined <=
                config_.closedEnterRatio)
            {
                if (closedCandidateStartMs_ < 0)
                {
                    closedCandidateStartMs_ =
                        timestampMs;
                }


                const int64_t confirmMs =
                    timestampMs -
                    closedCandidateStartMs_;

 
                if (confirmMs >=
                    config_.closedConfirmMs)
                {
                    // 闭眼真正起点使用：
                    //
                    // 第一次跨过 closedEnterRatio 的时间，
                    // 而不是确认完成后的当前时间。
                    eyeClosedStartMs_ =
                        closedCandidateStartMs_;


                    closedCandidateStartMs_ =
                        -1;


                    enterEyeState(
                        EyeState::CLOSED
                    );
                }
            }
            else
            {
                // 没有连续保持 CLOSED，
                // confirmation 重新开始。
                closedCandidateStartMs_ =
                    -1;
            }
        }
    }


    // --------------------------------------------------------
    // CLOSED
    // --------------------------------------------------------

    else if (
        eyeState_ ==
        EyeState::CLOSED)
    {
        updateEyeEpisodeMinimum(
            smoothedLeftOpenness_,
            smoothedRightOpenness_
        );


        // ----------------------------------------------------
        // CLOSED -> OPENING
        // ----------------------------------------------------

        const bool clearlyOpening =
            combinedOpenness >=
                config_.openReturnRatio
            ||
            (
                combinedOpenness >=
                    config_.closedExitRatio
                &&
                slopePerSecond >=
                    config_.openingSlopePerSecond
            );


        if (clearlyOpening)
        {
            eyeClosedEndMs_ =
                timestampMs;

            openCandidateStartMs_ =
                -1;

            enterEyeState(
                EyeState::OPENING
            );
        }
    }


    // --------------------------------------------------------
    // OPENING
    // --------------------------------------------------------

    else if (
        eyeState_ ==
        EyeState::OPENING)
    {
        updateEyeEpisodeMinimum(
            smoothedLeftOpenness_,
            smoothedRightOpenness_
        );


        // ----------------------------------------------------
        // OPENING 又掉回 CLOSED
        //
        // 继续同一个 episode。
        //
        // 不创建第二次闭眼事件。
        // ----------------------------------------------------

        if (combinedOpenness <=
            config_.closedEnterRatio)
        {
            eyeClosedEndMs_ = -1;

            openCandidateStartMs_ = -1;

            enterEyeState(
                EyeState::CLOSED
            );
        }

        else
        {
            // ------------------------------------------------
            // OPEN confirmation
            // ------------------------------------------------

            if (combinedOpenness >=
                config_.openReturnRatio)
            {
                if (openCandidateStartMs_ < 0)
                {
                    openCandidateStartMs_ =
                        timestampMs;
                }


                const int64_t confirmMs =
                    timestampMs -
                    openCandidateStartMs_;


                if (confirmMs >=
                    config_.openConfirmMs)
                {
                    // ----------------------------------------
                    // 完整 episode：
                    //
                    // CLOSING
                    // CLOSED
                    // OPENING
                    // OPEN
                    // ----------------------------------------

                    if (eyeEpisodeActive_ &&
                        eyeEpisodeStartMs_ >= 0 &&
                        eyeClosedStartMs_ >= 0)
                    {
                        if (eyeClosedEndMs_ < 0)
                        {
                            eyeClosedEndMs_ =
                                timestampMs;
                        }


                        const int64_t closedDurationMs =
                            eyeClosedEndMs_
                            -
                            eyeClosedStartMs_;


                        const int64_t totalDurationMs =
                            timestampMs
                            -
                            eyeEpisodeStartMs_;


                        const EyeClosureType type =
                            classifyClosure(
                                closedDurationMs,
                                totalDurationMs
                            );


                        // < minClosureMs 的抖动不输出事件。
                        if (type !=
                            EyeClosureType::NONE)
                        {
                            result.eyeClosure.completed =
                                true;

                            result.eyeClosure.type =
                                type;

                            result.eyeClosure.startMs =
                                eyeEpisodeStartMs_;

                            result.eyeClosure.closedStartMs =
                                eyeClosedStartMs_;

                            result.eyeClosure.closedEndMs =
                                eyeClosedEndMs_;

                            result.eyeClosure.endMs =
                                timestampMs;

                            result.eyeClosure.closedDurationMs =
                                closedDurationMs;

                            result.eyeClosure.totalDurationMs =
                                totalDurationMs;

                            result.eyeClosure.minLeftOpenness =
                                minLeftOpenness_;

                            result.eyeClosure.minRightOpenness =
                                minRightOpenness_;
                        }
                    }


                    cancelEyeEpisode();

                    enterEyeState(
                        EyeState::OPEN
                    );
                }
            }
            else
            {
                // OPEN 没有连续保持，
                // open confirmation 重新计时。
                openCandidateStartMs_ =
                    -1;
            }
        }
    }


    // ========================================================
    // 7. Eye realtime output
    // ========================================================

    result.eyeState =
        eyeState_;


    if (eyeStateStartMs_ >= 0)
    {
        result.stateDurationMs =
            timestampMs -
            eyeStateStartMs_;
    }


    // ========================================================
    // Current CLOSED duration + Emergency
    //
    // Emergency 只允许在当前真正处于 CLOSED 时触发。
    // ========================================================

    if (eyeState_ == EyeState::CLOSED &&
        eyeClosedStartMs_ >= 0)
    {
        // CLOSED duration 仍然保留，
        // 用于 ECD / LongClose / UI / WindowAnalyzer。
        result.currentClosedDurationMs =
            timestampMs -
            eyeClosedStartMs_;
    }


    // ========================================================
    // 8. Mouth / Yawn
    //
    // 当前基础哈欠规则：
    //
    // 1. mouthRatio >= 0.80
    //    -> 开启一个 5 秒检测窗口
    //
    // 2. 5 秒窗口内：
    //    mouthRatio >= 0.80 的累计时间 >= 70%
    //    即 >= 3500ms
    //    -> YAWN
    //
    // 3. 中途 mouthRatio < 0.80：
    //    不累计这段时间，
    //    但不会立即取消当前 5 秒窗口。
    //
    // 4. 同一次长时间张嘴只触发一次。
    // ========================================================

    const bool mouthOpenNow =
        observation.mouthRatio >=
            config_.mouthOpenThreshold;


    // 当前帧 MouthState 只反映嘴巴此刻是否超过 0.80。
    mouthState_ =
        mouthOpenNow
            ? MouthState::OPEN
            : MouthState::CLOSED;


    // ========================================================
    // 已经触发过一次 YAWN
    //
    // 嘴巴没有真正回落到阈值以下之前，
    // 不允许重新开启下一次 Yawn 窗口。
    // ========================================================

    if (yawnLatched_)
    {
        result.currentMouthOpenMs = 0;

        mouthLastTimestampMs_ =
            timestampMs;

        mouthWasOpen_ =
            mouthOpenNow;


        // 嘴巴终于回到阈值以下，
        // 允许下一次 Yawn 检测。
        if (!mouthOpenNow)
        {
            yawnLatched_ = false;

            mouthOpenActive_ = false;

            mouthOpenStartMs_ = -1;

            mouthOpenAccumMs_ = 0;

            mouthPeakRatio_ = 0.0f;
        }
    }


    // ========================================================
    // 当前还没有 Yawn 锁存
    // ========================================================

    else
    {
        // ----------------------------------------------------
        // 当前还没有 5 秒检测窗口
        // ----------------------------------------------------

        if (!mouthOpenActive_)
        {
            // mouthRatio 第一次 >= 0.80
            // 从这里开启新的 5 秒检测窗口。
            if (mouthOpenNow)
            {
                mouthOpenActive_ = true;

                mouthOpenStartMs_ =
                    timestampMs;

                mouthOpenAccumMs_ = 0;

                mouthLastTimestampMs_ =
                    timestampMs;

                mouthWasOpen_ = true;

                mouthPeakRatio_ =
                    observation.mouthRatio;
            }

            else
            {
                mouthLastTimestampMs_ =
                    timestampMs;

                mouthWasOpen_ = false;

                result.currentMouthOpenMs = 0;
            }
        }


        // ----------------------------------------------------
        // 已经存在一个 5 秒检测窗口
        // ----------------------------------------------------

        else
        {
            // ------------------------------------------------
            // 计算上一帧到这一帧经过多少时间。
            // ------------------------------------------------

            int64_t deltaMs = 0;

            if (mouthLastTimestampMs_ >= 0 &&
                timestampMs >=
                    mouthLastTimestampMs_)
            {
                deltaMs =
                    timestampMs -
                    mouthLastTimestampMs_;
            }


            // ------------------------------------------------
            // 上一个时间段嘴巴如果 >= 0.80，
            // 就把这一段时间累计进张嘴时间。
            // ------------------------------------------------

            if (mouthWasOpen_)
            {
                mouthOpenAccumMs_ +=
                    deltaMs;
            }


            // ------------------------------------------------
            // 更新当前帧信息。
            // ------------------------------------------------

            mouthLastTimestampMs_ =
                timestampMs;

            mouthWasOpen_ =
                mouthOpenNow;


            if (mouthOpenNow)
            {
                mouthPeakRatio_ =
                    std::max(
                        mouthPeakRatio_,
                        observation.mouthRatio
                    );
            }


            // ------------------------------------------------
            // 当前 5 秒窗口已经经过多久。
            // ------------------------------------------------

            const int64_t windowElapsedMs =
                timestampMs -
                mouthOpenStartMs_;


            // ------------------------------------------------
            // 5 秒 × 70%
            //
            // 默认：
            //
            // 5000 × 0.70 = 3500ms
            // ------------------------------------------------

            const int64_t requiredOpenMs =
                static_cast<int64_t>(
                    static_cast<double>(
                        config_.yawnWindowMs
                    )
                    *
                    static_cast<double>(
                        config_.yawnOpenFraction
                    )
                );


            // 当前调试输出中的 openMs，
            // 从现在开始表示：
            //
            // “当前 5 秒窗口内累计张嘴时间”
            //
            // 不再表示连续张嘴时间。
            result.currentMouthOpenMs =
                mouthOpenAccumMs_;


            // =================================================
            // 成功：
            //
            // 5 秒内已经累计张嘴 >= 3500ms
            //
            // 不需要非得等到第 5000ms 才触发。
            // 一旦已经达到 3500ms，
            // 即使后面剩余时间全部闭嘴，
            // 这一窗口最终也至少是 70%。
            // =================================================

            if (mouthOpenAccumMs_ >=
                requiredOpenMs)
            {
                result.yawn.detected =
                    true;

                result.yawn.startMs =
                    mouthOpenStartMs_;

                // durationMs 现在记录：
                // 实际累计张嘴时间。
                result.yawn.durationMs =
                    mouthOpenAccumMs_;

                result.yawn.peakMouthRatio =
                    mouthPeakRatio_;


                // 同一次张嘴只允许触发一次。
                yawnLatched_ = true;

                mouthOpenActive_ = false;

                mouthOpenStartMs_ = -1;

                mouthOpenAccumMs_ = 0;

                mouthPeakRatio_ = 0.0f;
            }


            // =================================================
            // 失败：
            //
            // 5 秒窗口已经结束，
            // 但累计张嘴仍不足 3500ms。
            // =================================================

            else if (windowElapsedMs >=
                    config_.yawnWindowMs)
            {
                mouthOpenActive_ = false;

                mouthOpenStartMs_ = -1;

                mouthOpenAccumMs_ = 0;

                mouthPeakRatio_ = 0.0f;

                result.currentMouthOpenMs = 0;


                // 如果这一帧嘴巴仍然 >= 0.80，
                // 下一有效帧可以重新开启一个新的 5 秒窗口。
            }
        }
    }


    result.mouthState =
        mouthState_;


    return result;
}