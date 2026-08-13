#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>


// ============================================================
// Baseline 配置
// ============================================================

struct FatigueBaselineConfig
{
    // 正式以后恢复 45000
    int64_t eyeCalibrationDurationMs = 15000;

    // 调试阶段
    std::size_t minEyeSamples = 50;


    // 正常睁眼基线采用样本分布的 70% 分位点
    float eyeOpenPercentile = 0.70f;


    // 两眼差距太离谱时，不用于校准。
    //
    // 这是“样本质量过滤”，
    // 不是 OPEN/CLOSED 阈值。
    float maxEyeAsymmetry = 0.45f;


    // 连续有效帧最大允许间隔。
    //
    // 超过这个值表示中间发生了：
    // No Face / PFLD failure 等。
    //
    // 这段时间不算有效校准时间。
    int64_t maxValidGapMs = 300;
};


// ============================================================
// 当前驾驶员眼睛基线
// ============================================================

struct EyeBaseline
{
    bool ready = false;

    // 正常睁眼时左右眼 Eye Ratio
    float leftOpenRatio = 0.0f;
    float rightOpenRatio = 0.0f;

    std::size_t sampleCount = 0;
};


// ============================================================
// 归一化后的连续眼睛状态
//
// 这里不是 OPEN / CLOSED。
//
// 正常睁眼通常约为：
// 1.0
//
// 比正常状态明显变窄：
// < 1.0
//
// 比校准时睁得更大：
// > 1.0
// ============================================================

struct NormalizedEyeObservation
{
    bool valid = false;

    float leftOpenness = 0.0f;
    float rightOpenness = 0.0f;
};


// ============================================================
// FatigueBaselineManager
//
// 当前第一阶段职责：
//
// Eye Ratio
//     ↓
// 45 秒个人校准
//     ↓
// Left / Right Open Baseline
//     ↓
// Normalized Eye Openness
//
// 后续再扩展：
//
// blinkRate baseline
// median ECD baseline
// PERCLOS baseline
// 长期 Driver Profile
// ============================================================

class FatigueBaselineManager
{
public:

    explicit FatigueBaselineManager(
        const FatigueBaselineConfig& config =
            FatigueBaselineConfig()
    );


    // 清除当前 session 校准
    void resetSession();

    // 输入当前有效帧的左右眼 Ratio
    void updateEyeCalibration(
        float leftEyeRatio,
        float rightEyeRatio,
        int64_t timestampMs
    );


    bool eyeBaselineReady() const;


    // 0.0 ~ 1.0
    float eyeCalibrationProgress() const;


    const EyeBaseline& eyeBaseline() const;


    // 将原始 Eye Ratio 转换成
    // 相对于个人正常状态的连续量
    NormalizedEyeObservation normalizeEyes(
        float leftEyeRatio,
        float rightEyeRatio
    ) const;


private:

    static float percentile(
        std::vector<float> values,
        float q
    );


    void finalizeEyeBaseline();


private:

    FatigueBaselineConfig config_;


    EyeBaseline eyeBaseline_;


    bool eyeCalibrationStarted_ = false;

    int64_t eyeCalibrationStartMs_ = 0;

    int64_t lastTimestampMs_ = 0;

    int64_t eyeValidAccumulatedMs_ = 0;

    int64_t previousValidTimestampMs_ = 0;

    std::vector<float> leftEyeSamples_;

    std::vector<float> rightEyeSamples_;
};