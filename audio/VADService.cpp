#include "VADService.h"

#include <QDebug>

#include <algorithm>
#include <cstring>


VADService::VADService(
    QObject *parent
)
    :
    QObject(parent)
{
}


VADService::~VADService()
{
    release();
}


bool VADService::init(
    const std::string &modelPath
)
{
    if (initialized_.load())
    {
        return true;
    }


    release();


    SherpaOnnxVadModelConfig config;

    std::memset(
        &config,
        0,
        sizeof(config)
    );


    /*
     * ========================================================
     * Silero VAD
     *
     * 参数完全按照刚才板子上验证成功的配置。
     * ========================================================
     */

    config.silero_vad.model =
        modelPath.c_str();

    config.silero_vad.threshold =
        0.5f;

    config.silero_vad.min_silence_duration =
        0.8f;

    config.silero_vad.min_speech_duration =
        0.25f;

    config.silero_vad.max_speech_duration =
        10.0f;

    config.silero_vad.window_size =
        512;


    config.sample_rate =
        kSampleRate;

    config.num_threads =
        1;

    config.provider =
        "cpu";

    config.debug =
        0;


    /*
     * 内部最多保存 15 秒左右的语音。
     *
     * 我们当前单次语音命令最多10秒，
     * 留一点余量。
     */
    vad_ =
        SherpaOnnxCreateVoiceActivityDetector(
            &config,
            15.0f
        );


    if (!vad_)
    {
        qWarning()
            << "[VADService] create VAD failed";

        emit errorOccurred(
            QStringLiteral(
                "Create sherpa-onnx VAD failed"
            )
        );

        return false;
    }


    initialized_.store(
        true
    );


    qDebug()
        << "[VADService]"
        << "initialized successfully";


    return true;
}


void VADService::startListening()
{
    if (
        !initialized_.load()
        ||
        !vad_
    )
    {
        return;
    }

    std::lock_guard<std::mutex> lock(
        vadMutex_
    );

    if (!initialized_.load() || !vad_)
    {
        return;
    }


    /*
     * 清掉上一轮残留状态。
     */
    SherpaOnnxVoiceActivityDetectorReset(
        vad_
    );


    speechActive_ =
        false;

    listeningPcmHistory_.clear();


    listening_.store(
        true
    );


    qDebug()
        << "[VADService]"
        << "Listening started";


    emit listeningStarted();
}


void VADService::stopListening()
{
    std::lock_guard<std::mutex> lock(
        vadMutex_
    );

    if (!listening_.exchange(false))
    {
        return;
    }


    speechActive_ =
        false;

    listeningPcmHistory_.clear();


    if (vad_)
    {
        SherpaOnnxVoiceActivityDetectorReset(
            vad_
        );
    }


    qDebug()
        << "[VADService]"
        << "Listening stopped";


    emit listeningStopped();
}


void VADService::processPcm(
    const QByteArray &pcm
)
{
    if (
        !initialized_.load()
        ||
        !listening_.load()
        ||
        !vad_
    )
    {
        return;
    }

    std::lock_guard<std::mutex> lock(
        vadMutex_
    );

    if (
        !initialized_.load()
        ||
        !listening_.load()
        ||
        !vad_
    )
    {
        return;
    }


    if (
        pcm.isEmpty()
        ||
        pcm.size() % sizeof(int16_t) != 0
    )
    {
        return;
    }


    const int16_t *src =
        reinterpret_cast<const int16_t *>(
            pcm.constData()
        );


    const int count =
        pcm.size()
        /
        static_cast<int>(
            sizeof(int16_t)
        );

    // Preserve the exact waveform timeline accepted by VAD. The buffer is
    // reset for every listening turn and is small (about 0.96 MB even for a
    // complete 30-second follow-up window).
    listeningPcmHistory_.append(pcm);


    /*
     * int16
     *
     * ↓
     *
     * float [-1, +1]
     */
    std::vector<float> samples(
        static_cast<std::size_t>(
            count
        )
    );


    for (
        int i = 0;
        i < count;
        ++i
    )
    {
        samples[
            static_cast<std::size_t>(i)
        ] =
            static_cast<float>(
                src[i]
            )
            /
            32768.0f;
    }


    /*
     * sherpa VAD 自己有内部缓存，
     * 因此这里可以直接送你的 320 samples。
     *
     * 不需要我们自己强行拼成512。
     */
    SherpaOnnxVoiceActivityDetectorAcceptWaveform(
        vad_,
        samples.data(),
        count
    );


    /*
     * 当前正在讲话？
     */
    const bool detected =
        SherpaOnnxVoiceActivityDetectorDetected(
            vad_
        ) != 0;


    if (
        detected
        &&
        !speechActive_
    )
    {
        speechActive_ =
            true;


        qDebug()
            << "[VADService]"
            << "Speech started";


        emit speechStarted();
    }


    /*
     * ========================================================
     * 已经产生完整 speech segment
     * ========================================================
     */

    while (
        SherpaOnnxVoiceActivityDetectorEmpty(
            vad_
        ) == 0
    )
    {
        const SherpaOnnxSpeechSegment *segment =
            SherpaOnnxVoiceActivityDetectorFront(
                vad_
            );


        if (!segment)
        {
            break;
        }


        /*
         * segment->samples 是 float [-1,+1]
         *
         * ASR后面其实可以直接吃float，
         * 但为了保持你的 Audio 模块接口统一，
         * 当前先转回 S16_LE QByteArray。
         */
        const int historySamples =
            listeningPcmHistory_.size()
            /
            static_cast<int>(sizeof(int16_t));
        const bool segmentStartInHistory =
            segment->start >= 0
            &&
            segment->start <= historySamples;
        const int segmentStart =
            segmentStartInHistory
                ? segment->start
                : 0;
        const int wantedPreRollSamples =
            kSampleRate * kPreRollMs / 1000;
        const int prefixSamples =
            segmentStartInHistory
                ? std::min(
                    wantedPreRollSamples,
                    segmentStart)
                : 0;
        const int prefixStart =
            segmentStart - prefixSamples;
        const int segmentEnd =
            segmentStartInHistory
                ? segmentStart + segment->n
                : 0;
        const bool segmentEndInHistory =
            segmentStartInHistory
            &&
            segmentEnd >= segmentStart
            &&
            segmentEnd <= historySamples;
        const int wantedPostRollSamples =
            kSampleRate * kPostRollMs / 1000;
        const int suffixSamples =
            segmentEndInHistory
                ? std::min(
                    wantedPostRollSamples,
                    historySamples - segmentEnd)
                : 0;

        QByteArray output;

        output.resize(
            (prefixSamples + segment->n + suffixSamples)
            *
            static_cast<int>(sizeof(int16_t))
        );


        int16_t *dst =
            reinterpret_cast<int16_t *>(
                output.data()
            );

        if (prefixSamples > 0)
        {
            std::memcpy(
                dst,
                listeningPcmHistory_.constData()
                    + prefixStart *
                        static_cast<int>(sizeof(int16_t)),
                static_cast<size_t>(prefixSamples) *
                    sizeof(int16_t));
        }


        for (
            int i = 0;
            i < segment->n;
            ++i
        )
        {
            float v =
                segment->samples[i];


            if (v > 1.0f)
            {
                v = 1.0f;
            }
            else if (v < -1.0f)
            {
                v = -1.0f;
            }


            dst[prefixSamples + i] =
                static_cast<int16_t>(
                    v * 32767.0f
                );
        }

        if (suffixSamples > 0)
        {
            std::memcpy(
                dst + prefixSamples + segment->n,
                listeningPcmHistory_.constData()
                    + segmentEnd *
                        static_cast<int>(sizeof(int16_t)),
                static_cast<size_t>(suffixSamples) *
                    sizeof(int16_t));
        }


        const double duration =
            static_cast<double>(
                segment->n
            )
            /
            kSampleRate;


        qDebug()
            << "[VADService]"
            << "Speech ended,"
            << "duration ="
            << duration
            << "seconds"
            << "segmentStart ="
            << segment->start
            << "preRoll ="
            << static_cast<double>(prefixSamples)
                / kSampleRate
            << "seconds"
            << "postRoll ="
            << static_cast<double>(suffixSamples)
                / kSampleRate
            << "seconds";


        /*
         * Front() 返回的是一个副本，
         * 先释放副本，再 Pop 队列。
         */
        SherpaOnnxDestroySpeechSegment(
            segment
        );


        SherpaOnnxVoiceActivityDetectorPop(
            vad_
        );


        speechActive_ =
            false;


        /*
         * 一次唤醒只取一句命令。
         */
        listening_.store(
            false
        );

        listeningPcmHistory_.clear();


        emit speechSegmentReady(
            output
        );


        emit listeningStopped();


        break;
    }
}


bool VADService::isInitialized() const
{
    return initialized_.load();
}


bool VADService::isListening() const
{
    return listening_.load();
}


void VADService::release()
{
    listening_.store(false);

    initialized_.store(false);


    if (vad_)
    {
        SherpaOnnxDestroyVoiceActivityDetector(
            vad_
        );

        vad_ =
            nullptr;
    }
}
