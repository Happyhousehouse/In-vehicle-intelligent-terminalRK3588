#include "AudioInputService.h"

#include <QDebug>
#include <QElapsedTimer>

#include <algorithm>
#include <cmath>
#include <vector>


AudioInputService::AudioInputService(
    QObject *parent
)
    :
    QObject(parent)
{
}


AudioInputService::~AudioInputService()
{
    qDebug()
        << "[AudioInputService] destructor";

    stop();

    close();
}


/*
 * ============================================================
 * 打开麦克风
 * ============================================================
 */

bool AudioInputService::open(
    const std::string &device,
    unsigned int sampleRate,
    unsigned int channels
)
{
    /*
     * 正在采集时不能重新打开。
     */
    if (running_.load())
    {
        qWarning()
            << "[AudioInputService]"
            << "cannot open while running";

        return false;
    }


    /*
     * 如果以前打开过，
     * 先关闭旧设备。
     */
    close();


    device_ =
        device;

    sampleRate_ =
        sampleRate;

    channels_ =
        channels;


    qDebug()
        << "[AudioInputService]"
        << "opening device:"
        << QString::fromStdString(device_);


    /*
     * ========================================================
     * 1. 打开 PCM Capture
     * ========================================================
     */

    int ret =
        snd_pcm_open(
            &pcmHandle_,
            device_.c_str(),
            SND_PCM_STREAM_CAPTURE,
            0
        );


    if (ret < 0)
    {
        const QString error =
            QStringLiteral(
                "snd_pcm_open failed: %1"
            )
            .arg(
                snd_strerror(ret)
            );


        qWarning()
            << "[AudioInputService]"
            << error;


        emit errorOccurred(
            error
        );


        pcmHandle_ =
            nullptr;


        return false;
    }


    /*
     * ========================================================
     * 2. 创建硬件参数
     * ========================================================
     */

    snd_pcm_hw_params_t *params =
        nullptr;


    snd_pcm_hw_params_alloca(
        &params
    );


    ret =
        snd_pcm_hw_params_any(
            pcmHandle_,
            params
        );


    if (ret < 0)
    {
        qWarning()
            << "[AudioInputService]"
            << "snd_pcm_hw_params_any failed:"
            << snd_strerror(ret);

        close();

        return false;
    }


    /*
     * ========================================================
     * 3. Interleaved
     *
     * 单声道时其实影响不大，
     * 但以后如果改为多声道，
     * 数据排列方式就是：
     *
     * L R L R L R ...
     * ========================================================
     */

    ret =
        snd_pcm_hw_params_set_access(
            pcmHandle_,
            params,
            SND_PCM_ACCESS_RW_INTERLEAVED
        );


    if (ret < 0)
    {
        qWarning()
            << "[AudioInputService]"
            << "set access failed:"
            << snd_strerror(ret);

        close();

        return false;
    }


    /*
     * ========================================================
     * 4. 16-bit signed little endian
     * ========================================================
     */

    ret =
        snd_pcm_hw_params_set_format(
            pcmHandle_,
            params,
            SND_PCM_FORMAT_S16_LE
        );


    if (ret < 0)
    {
        qWarning()
            << "[AudioInputService]"
            << "set format failed:"
            << snd_strerror(ret);

        close();

        return false;
    }


    /*
     * ========================================================
     * 5. 声道
     * ========================================================
     */

    ret =
        snd_pcm_hw_params_set_channels(
            pcmHandle_,
            params,
            channels_
        );


    if (ret < 0)
    {
        qWarning()
            << "[AudioInputService]"
            << "set channels failed:"
            << snd_strerror(ret);

        close();

        return false;
    }


    /*
     * ========================================================
     * 6. 采样率
     * ========================================================
     */

    unsigned int actualRate =
        sampleRate_;


    ret =
        snd_pcm_hw_params_set_rate_near(
            pcmHandle_,
            params,
            &actualRate,
            nullptr
        );


    if (ret < 0)
    {
        qWarning()
            << "[AudioInputService]"
            << "set sample rate failed:"
            << snd_strerror(ret);

        close();

        return false;
    }


    /*
     * ASR 后面要求稳定 16kHz。
     *
     * 如果设备实际给的不是我们要求的频率，
     * 这里直接拒绝。
     */
    if (actualRate != sampleRate_)
    {
        qWarning()
            << "[AudioInputService]"
            << "sample rate mismatch:"
            << actualRate
            << "requested:"
            << sampleRate_;

        close();

        return false;
    }


    /*
     * ========================================================
     * 7. 请求约 20ms period
     * ========================================================
     */

    snd_pcm_uframes_t periodSize =
        framesPerRead_;


    ret =
        snd_pcm_hw_params_set_period_size_near(
            pcmHandle_,
            params,
            &periodSize,
            nullptr
        );


    if (ret < 0)
    {
        qWarning()
            << "[AudioInputService]"
            << "set period size failed:"
            << snd_strerror(ret);

        close();

        return false;
    }


    /*
     * 注意：
     *
     * ALSA 硬件最终 period 不一定就是 320，
     * 所以这里只打印出来观察。
     *
     * 我们仍然每次请求读取 320 frames。
     */
    qDebug()
        << "[AudioInputService]"
        << "ALSA period size ="
        << static_cast<qulonglong>(
               periodSize
           );


    /*
     * ========================================================
     * 8. 正式应用参数
     * ========================================================
     */

    ret =
        snd_pcm_hw_params(
            pcmHandle_,
            params
        );


    if (ret < 0)
    {
        qWarning()
            << "[AudioInputService]"
            << "apply hw params failed:"
            << snd_strerror(ret);

        close();

        return false;
    }


    /*
     * ========================================================
     * 9. Prepare
     * ========================================================
     */

    ret =
        snd_pcm_prepare(
            pcmHandle_
        );


    if (ret < 0)
    {
        qWarning()
            << "[AudioInputService]"
            << "snd_pcm_prepare failed:"
            << snd_strerror(ret);

        close();

        return false;
    }


    qDebug()
        << "[AudioInputService]"
        << "open success"
        << "device ="
        << QString::fromStdString(device_)
        << "rate ="
        << sampleRate_
        << "channels ="
        << channels_
        << "format = S16_LE";


    return true;
}


/*
 * ============================================================
 * 开始采集
 * ============================================================
 */

bool AudioInputService::start()
{
    if (!pcmHandle_)
    {
        qWarning()
            << "[AudioInputService]"
            << "start failed: device not open";

        emit errorOccurred(
            QStringLiteral(
                "麦克风设备尚未打开。"
            )
        );


        return false;
    }


    /*
     * 已经运行。
     */
    if (running_.load())
    {
        return true;
    }


    /*
     * 重新 prepare。
     *
     * 后面 stop/start 时也比较安全。
     */
    const int ret =
        snd_pcm_prepare(
            pcmHandle_
        );


    if (ret < 0)
    {
        qWarning()
            << "[AudioInputService]"
            << "prepare before start failed:"
            << snd_strerror(ret);

        return false;
    }


    running_.store(
        true
    );


    captureThread_ =
        std::thread(
            &AudioInputService::captureLoop,
            this
        );


    emit runningChanged();


    qDebug()
        << "[AudioInputService]"
        << "capture started";


    return true;
}


/*
 * ============================================================
 * 后台采集线程
 * ============================================================
 */

void AudioInputService::captureLoop()
{
    qDebug()
        << "[AudioInputService]"
        << "capture thread started";


    /*
     * 320 frames × channels
     */
    std::vector<int16_t> buffer(
        static_cast<std::size_t>(
            framesPerRead_
        )
        *
        channels_
    );


    /*
     * 只用于控制调试日志频率。
     *
     * PCM 仍然每 20ms 输出，
     * 但终端没必要每秒打印 50 次。
     */
    QElapsedTimer debugTimer;

    debugTimer.start();


    while (running_.load())
    {
        /*
         * ====================================================
         * 从 ALSA 读取 PCM
         * ====================================================
         */

        snd_pcm_sframes_t framesRead =
            snd_pcm_readi(
                pcmHandle_,
                buffer.data(),
                framesPerRead_
            );


        /*
         * ====================================================
         * ALSA 错误恢复
         *
         * 常见：
         *
         * -EPIPE
         * XRUN / overrun
         * ====================================================
         */

        if (framesRead < 0)
        {
            const int recoverResult =
                snd_pcm_recover(
                    pcmHandle_,
                    static_cast<int>(
                        framesRead
                    ),
                    1
                );


            if (recoverResult < 0)
            {
                const QString error =
                    QStringLiteral(
                        "ALSA capture failed: %1"
                    )
                    .arg(
                        snd_strerror(
                            recoverResult
                        )
                    );


                qWarning()
                    << "[AudioInputService]"
                    << error;


                emit errorOccurred(
                    error
                );


                break;
            }


            continue;
        }


        if (framesRead == 0)
        {
            continue;
        }


        /*
         * ====================================================
         * 实际 sample 数量
         * ====================================================
         */

        const std::size_t sampleCount =
            static_cast<std::size_t>(
                framesRead
            )
            *
            channels_;


        /*
         * ====================================================
         * Peak / RMS
         * ====================================================
         */

        updateLevel(
            buffer.data(),
            sampleCount
        );


        /*
         * ====================================================
         * PCM 数据输出
         *
         * S16_LE：
         *
         * 每个 sample = 2 bytes
         * ====================================================
         */

        const int byteCount =
            static_cast<int>(
                sampleCount
                *
                sizeof(int16_t)
            );


        QByteArray pcm(
            reinterpret_cast<
                const char *
            >(
                buffer.data()
            ),
            byteCount
        );


        emit pcmDataReady(
            pcm
        );


        /*
         * ====================================================
         * 每 500ms 打印一次音量
         * ====================================================
         */

        if (
            debugTimer.elapsed()
            >=
            500
        )
        {
            qDebug()
                << "[AudioInputService]"
                << "Peak ="
                << peakLevel_.load() * 100.0
                << "%"
                << "RMS ="
                << rmsLevel_.load() * 100.0
                << "%";


            debugTimer.restart();
        }
    }


    /*
     * 如果不是 stop() 主动退出，
     * 而是 ALSA 错误导致线程结束，
     * 要修正 running 状态。
     */
    const bool wasRunning =
        running_.exchange(
            false
        );


    if (wasRunning)
    {
        emit runningChanged();
    }


    qDebug()
        << "[AudioInputService]"
        << "capture thread finished";
}


/*
 * ============================================================
 * 计算音量
 * ============================================================
 */

void AudioInputService::updateLevel(
    const int16_t *samples,
    std::size_t count
)
{
    if (
        !samples
        ||
        count == 0
    )
    {
        peakLevel_.store(
            0.0
        );

        rmsLevel_.store(
            0.0
        );

        emit levelChanged();

        return;
    }


    int peak =
        0;


    double squareSum =
        0.0;


    for (
        std::size_t i = 0;
        i < count;
        ++i
    )
    {
        /*
         * 转 int 防止 abs(INT16_MIN)
         * 的边界问题。
         */
        const int value =
            static_cast<int>(
                samples[i]
            );


        const int amplitude =
            std::abs(
                value
            );


        peak =
            std::max(
                peak,
                amplitude
            );


        const double d =
            static_cast<double>(
                value
            );


        squareSum +=
            d * d;
    }


    const double rms =
        std::sqrt(
            squareSum
            /
            static_cast<double>(
                count
            )
        );


    /*
     * int16 最大幅度：
     *
     * 32768
     *
     * 转成：
     *
     * 0.0 ~ 1.0
     */
    double normalizedPeak =
        static_cast<double>(
            peak
        )
        /
        32768.0;


    double normalizedRms =
        rms
        /
        32768.0;


    normalizedPeak =
        std::min(
            normalizedPeak,
            1.0
        );


    normalizedRms =
        std::min(
            normalizedRms,
            1.0
        );


    peakLevel_.store(
        normalizedPeak
    );


    rmsLevel_.store(
        normalizedRms
    );


    emit levelChanged();
}


/*
 * ============================================================
 * Stop
 * ============================================================
 */

void AudioInputService::stop()
{
    if (!running_.load())
    {
        /*
         * 即便 running 已经被采集线程
         * 因错误改成 false，
         * 仍然需要回收线程。
         */
        if (captureThread_.joinable())
        {
            captureThread_.join();
        }


        return;
    }


    qDebug()
        << "[AudioInputService]"
        << "stopping capture";


    running_.store(
        false
    );


    /*
     * 当前每次 read 只有约 20ms，
     * 所以这里直接等线程自然退出即可。
     */
    if (captureThread_.joinable())
    {
        captureThread_.join();
    }


    peakLevel_.store(
        0.0
    );


    rmsLevel_.store(
        0.0
    );


    emit levelChanged();

    emit runningChanged();


    qDebug()
        << "[AudioInputService]"
        << "capture stopped";
}


/*
 * ============================================================
 * Close
 * ============================================================
 */

void AudioInputService::close()
{
    /*
     * 不允许正在采集时直接关设备。
     */
    stop();


    if (pcmHandle_)
    {
        snd_pcm_close(
            pcmHandle_
        );


        pcmHandle_ =
            nullptr;


        qDebug()
            << "[AudioInputService]"
            << "device closed";
    }
}


/*
 * ============================================================
 * Getter
 * ============================================================
 */

bool AudioInputService::isRunning() const
{
    return running_.load();
}


double AudioInputService::peakLevel() const
{
    return peakLevel_.load();
}


double AudioInputService::rmsLevel() const
{
    return rmsLevel_.load();
}