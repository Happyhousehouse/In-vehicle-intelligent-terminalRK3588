#include "TTSService.h"
#include "MatchaRknnEngine.h"

#include <QByteArray>
#include <QDebug>

#include <algorithm>
#include <cmath>
#include <cstring>


TTSService::TTSService(
    QObject *parent
)
    :
    QObject(parent)
{
}


TTSService::~TTSService()
{
    qDebug()
        << "[TTSService]"
        << "destructor";


    stopRequested_.store(
        true
    );


    {
        std::lock_guard<std::mutex> lock(
            mutex_
        );

        shuttingDown_ =
            true;

        speechQueue_.clear();

        pcmQueue_.clear();
    }


    speechCv_.notify_all();

    pcmCv_.notify_all();


    if (synthThread_.joinable())
    {
        synthThread_.join();
    }


    if (playbackThread_.joinable())
    {
        playbackThread_.join();
    }


    release();
}


bool TTSService::init(
    const std::string &modelDir
)
{
    if (initialized_.load())
    {
        return true;
    }


    release();

    std::string initError;

    if (!matchaRknnInitialize(
            modelDir,
            &initError))
    {
        const QString error =
            QString::fromStdString(initError);


        emit errorOccurred(
            error
        );


        return false;
    }

    modelDir_ = modelDir;
    sampleRate_ = 16000;


    initialized_.store(
        true
    );


    stopRequested_.store(
        false
    );


    {
        std::lock_guard<std::mutex> lock(
            mutex_
        );

        shuttingDown_ =
            false;

        inputFinished_ =
            false;

        turnActive_ =
            false;

        synthBusy_ =
            false;

        playbackBusy_ =
            false;

        speechStartedEmitted_ =
            false;

        speechQueue_.clear();

        pcmQueue_.clear();
    }


    synthThread_ =
        std::thread(
            &TTSService::synthLoop,
            this
        );


    playbackThread_ =
        std::thread(
            &TTSService::playbackLoop,
            this
        );


    qDebug()
        << "[TTSService]"
        << "Matcha RKNN TTS initialized on NPU core 2"
        << "sampleRate ="
        << sampleRate_;


    return true;
}


/*
 * ============================================================
 * float PCM → S16 PCM Queue
 * ============================================================
 */
bool TTSService::enqueueFloatPcm(
    const float *samples,
    int sampleCount,
    const QString &text,
    bool startOfText
)
{
    if (
        !samples
        ||
        sampleCount <= 0
    )
    {
        return true;
    }


    if (stopRequested_.load())
    {
        return false;
    }


    PcmPacket packet;

    packet.text =
        text;

    packet.sampleRate =
        sampleRate_;

    packet.startOfText =
        startOfText;


    packet.pcm.resize(
        static_cast<std::size_t>(
            sampleCount
        )
    );


    for (
        int i = 0;
        i < sampleCount;
        ++i
    )
    {
        float value =
            samples[i];


        value =
            std::max(
                -1.0f,
                std::min(
                    1.0f,
                    value
                )
            );


        packet.pcm[
            static_cast<std::size_t>(
                i
            )
        ] =
            static_cast<int16_t>(
                std::lrint(
                    value
                    *
                    32767.0f
                )
            );
    }


    {
        std::lock_guard<std::mutex> lock(
            mutex_
        );


        if (
            shuttingDown_
            ||
            stopRequested_.load()
        )
        {
            return false;
        }


        pcmQueue_.push_back(
            std::move(
                packet
            )
        );
    }


    /*
     * PCM一生成出来，
     * 立即唤醒播放线程。
     */
    pcmCv_.notify_one();


    return true;
}


/*
 * ============================================================
 * 单段文字的Matcha RKNN生成
 * ============================================================
 */
bool TTSService::generateStreaming(
    const SpeechItem &item,
    QString *error
)
{
    if (modelDir_.empty())
    {
        if (error)
        {
            *error = QStringLiteral("Matcha model directory is empty");
        }

        return false;
    }

    qDebug()
        << "[TTSService]"
        << "Matcha generating:"
        << item.text
        << "pauseAfterMs ="
        << item.pauseAfterMs;

    std::vector<float> audio;
    std::string matchaError;
    const QByteArray utf8 = item.text.toUtf8();

    const bool ok = matchaRknnSynthesize(
        modelDir_,
        std::string(utf8.constData(), static_cast<size_t>(utf8.size())),
        &audio,
        &stopRequested_,
        &matchaError);

    if (stopRequested_.load())
        return true;

    if (!ok || audio.empty())
    {
        if (error)
        {
            *error = QString::fromStdString(
                matchaError.empty()
                    ? "Matcha RKNN generation failed"
                    : matchaError);
        }

        return false;
    }

    // Preserve the same per-chunk peak normalization as the validated
    // standalone WAV path. This is part of the known-good sound quality.
    float peak = 0.0f;
    for (const float sample : audio)
    {
        if (std::isfinite(sample))
            peak = std::max(peak, std::fabs(sample));
    }

    const float gain = peak > 1.0e-8f
        ? 0.95f / peak
        : 1.0f;

    for (float &sample : audio)
        sample = std::isfinite(sample) ? sample * gain : 0.0f;

    const int safePauseMs =
        std::max(0, std::min(item.pauseAfterMs, 1000));
    const size_t pauseSamples =
        static_cast<size_t>(sampleRate_) *
        static_cast<size_t>(safePauseMs) / 1000U;
    audio.insert(audio.end(), pauseSamples, 0.0f);

    if (!enqueueFloatPcm(
            audio.data(),
            static_cast<int>(audio.size()),
            item.text,
            true))
    {
        return true;
    }


    /*
     * 插入一个“文字块结束标记”。
     *
     * 它没有PCM。
     */
    {
        std::lock_guard<std::mutex> lock(
            mutex_
        );


        if (
            !shuttingDown_
            &&
            !stopRequested_.load()
        )
        {
            PcmPacket marker;

            marker.text =
                item.text;

            marker.endOfText =
                true;


            pcmQueue_.push_back(
                std::move(
                    marker
                )
            );
        }
    }


    pcmCv_.notify_one();


    qDebug()
        << "[TTSService]"
        << "Matcha generation completed:"
        << item.text;


    return true;
}


/*
 * ============================================================
 * 文本生成线程
 * ============================================================
 */
void TTSService::synthLoop()
{
    qDebug()
        << "[TTSService]"
        << "stream synth worker started";


    while (true)
    {
        SpeechItem item;


        {
            std::unique_lock<std::mutex> lock(
                mutex_
            );


            speechCv_.wait(
                lock,

                [this]()
                {
                    return
                        shuttingDown_
                        ||
                        !speechQueue_.empty();
                }
            );


            if (shuttingDown_)
            {
                break;
            }


            item =
                speechQueue_.front();


            speechQueue_.pop_front();


            synthBusy_ =
                true;
        }


        QString error;


        const bool ok =
            generateStreaming(
                item,
                &error
            );


        {
            std::lock_guard<std::mutex> lock(
                mutex_
            );

            synthBusy_ =
                false;
        }


        /*
         * stop属于正常控制流程，
         * 不打印TTS错误。
         */
        if (
            !ok
            &&
            !stopRequested_.load()
        )
        {
            qWarning()
                << "[TTSService]"
                << error;


            emit errorOccurred(
                error
            );
        }


        /*
         * 可能此时已经满足整轮结束条件，
         * 唤醒播放线程检查。
         */
        pcmCv_.notify_all();
    }


    qDebug()
        << "[TTSService]"
        << "stream synth worker stopped";
}


/*
 * ============================================================
 * ALSA播放线程
 * ============================================================
 */
void TTSService::playbackLoop()
{
    qDebug()
        << "[TTSService]"
        << "stream playback worker started";


    snd_pcm_t *handle =
        nullptr;


    int currentSampleRate =
        0;


    while (true)
    {
        PcmPacket packet;

        bool havePacket =
            false;


        {
            std::unique_lock<std::mutex> lock(
                mutex_
            );


            pcmCv_.wait(
                lock,

                [this]()
                {
                    return
                        shuttingDown_
                        ||
                        stopRequested_.load()
                        ||
                        !pcmQueue_.empty()
                        ||
                        (
                            turnActive_
                            &&
                            inputFinished_
                            &&
                            speechQueue_.empty()
                            &&
                            !synthBusy_
                        );
                }
            );


            if (shuttingDown_)
            {
                break;
            }


            /*
             * ====================================================
             * stop()
             * ====================================================
             */
            if (stopRequested_.load())
            {
                pcmQueue_.clear();


                playbackBusy_ =
                    false;


                /*
                 * 先解锁，
                 * 处理ALSA。
                 */
                lock.unlock();


                if (handle)
                {
                    /*
                     * 丢弃ALSA中还没真正播放的缓存。
                     */
                    snd_pcm_drop(
                        handle
                    );


                    snd_pcm_close(
                        handle
                    );


                    handle =
                        nullptr;


                    currentSampleRate =
                        0;
                }


                /*
                 * 必须等正在进行的sherpa callback真正退出。
                 *
                 * 期间stopRequested_保持true，
                 * callback会持续return 0。
                 */
                lock.lock();


                pcmCv_.wait(
                    lock,

                    [this]()
                    {
                        return
                            shuttingDown_
                            ||
                            !synthBusy_;
                    }
                );


                if (shuttingDown_)
                {
                    break;
                }


                const bool hadTurn =
                    turnActive_;


                turnActive_ =
                    false;


                inputFinished_ =
                    false;


                speechStartedEmitted_ =
                    false;


                busy_.store(
                    false
                );


                stopRequested_.store(
                    false
                );


                lock.unlock();


                if (hadTurn)
                {
                    qDebug()
                        << "[TTSService]"
                        << "speech stopped";


                    emit speechStopped();


                    /*
                     * VoiceController仍然可以按照
                     * “本轮语音结束”继续状态机。
                     */
                    emit speechFinished();
                }


                continue;
            }


            /*
             * 有真正PCM/marker。
             */
            if (!pcmQueue_.empty())
            {
                packet =
                    std::move(
                        pcmQueue_.front()
                    );


                pcmQueue_.pop_front();


                playbackBusy_ =
                    true;


                havePacket =
                    true;
            }
        }


        /*
         * ========================================================
         * 真正处理PCM packet
         * ========================================================
         */
        if (havePacket)
        {
            /*
             * 只是文字结束marker。
             */
            if (packet.endOfText)
            {
                emit speechChunkFinished(
                    packet.text
                );


                {
                    std::lock_guard<std::mutex> lock(
                        mutex_
                    );

                    playbackBusy_ =
                        false;
                }


                pcmCv_.notify_all();

                continue;
            }


            if (packet.pcm.empty())
            {
                {
                    std::lock_guard<std::mutex> lock(
                        mutex_
                    );

                    playbackBusy_ =
                        false;
                }

                continue;
            }


            /*
             * ====================================================
             * ALSA设备
             * ====================================================
             */
            if (
                !handle
                ||
                currentSampleRate
                    !=
                    packet.sampleRate
            )
            {
                if (handle)
                {
                    snd_pcm_close(
                        handle
                    );

                    handle =
                        nullptr;
                }


                int ret =
                    snd_pcm_open(
                        &handle,
                        kPlaybackDevice,
                        SND_PCM_STREAM_PLAYBACK,
                        0
                    );


                if (ret < 0)
                {
                    const QString error =
                        QStringLiteral(
                            "Open ALSA playback failed: %1"
                        )
                        .arg(
                            QString::fromLocal8Bit(
                                snd_strerror(
                                    ret
                                )
                            )
                        );


                    emit errorOccurred(
                        error
                    );


                    {
                        std::lock_guard<std::mutex> lock(
                            mutex_
                        );

                        playbackBusy_ =
                            false;
                    }


                    continue;
                }


                ret =
                    snd_pcm_set_params(
                        handle,
                        SND_PCM_FORMAT_S16_LE,
                        SND_PCM_ACCESS_RW_INTERLEAVED,
                        1,
                        static_cast<unsigned int>(
                            packet.sampleRate
                        ),
                        1,

                        /*
                         * 100ms ALSA buffer。
                         */
                        100000
                    );


                if (ret < 0)
                {
                    const QString error =
                        QStringLiteral(
                            "Set ALSA parameters failed: %1"
                        )
                        .arg(
                            QString::fromLocal8Bit(
                                snd_strerror(
                                    ret
                                )
                            )
                        );


                    emit errorOccurred(
                        error
                    );


                    snd_pcm_close(
                        handle
                    );


                    handle =
                        nullptr;


                    {
                        std::lock_guard<std::mutex> lock(
                            mutex_
                        );

                        playbackBusy_ =
                            false;
                    }


                    continue;
                }


                currentSampleRate =
                    packet.sampleRate;


                /*
                 * =================================================
                 * ES8388启动预热
                 *
                 * 只在整轮第一次open声卡时出现。
                 *
                 * 防止“我是……”前几个字被吃掉。
                 * =================================================
                 */
                constexpr int kWarmupMs =
                    120;


                const int warmupSamples =
                    packet.sampleRate
                    *
                    kWarmupMs
                    /
                    1000;


                std::vector<int16_t> silence(
                    static_cast<std::size_t>(
                        warmupSamples
                    ),
                    0
                );


                QString warmupError;


                writePcm(
                    handle,
                    silence.data(),
                    static_cast<int>(
                        silence.size()
                    ),
                    &warmupError
                );
            }


            /*
             * stop刚好在open ALSA期间发生。
             */
            if (stopRequested_.load())
            {
                {
                    std::lock_guard<std::mutex> lock(
                        mutex_
                    );

                    playbackBusy_ =
                        false;
                }

                pcmCv_.notify_all();

                continue;
            }


            bool emitStart =
                false;


            {
                std::lock_guard<std::mutex> lock(
                    mutex_
                );


                if (!speechStartedEmitted_)
                {
                    speechStartedEmitted_ =
                        true;

                    emitStart =
                        true;
                }
            }


            if (emitStart)
            {
                emit speechStarted();
            }


            if (packet.startOfText)
            {
                qDebug()
                    << "[TTSService]"
                    << "stream playing:"
                    << packet.text;


                emit speechChunkStarted(
                    packet.text
                );
            }


            QString error;


            const bool ok =
                writePcm(
                    handle,
                    packet.pcm.data(),
                    static_cast<int>(
                        packet.pcm.size()
                    ),
                    &error
                );


            {
                std::lock_guard<std::mutex> lock(
                    mutex_
                );

                playbackBusy_ =
                    false;
            }


            if (
                !ok
                &&
                !stopRequested_.load()
            )
            {
                emit errorOccurred(
                    error
                );
            }


            pcmCv_.notify_all();

            continue;
        }


        /*
         * ========================================================
         * 没有PCM了：
         *
         * 检查整轮是否结束。
         * ========================================================
         */
        bool needDrain =
            false;


        {
            std::lock_guard<std::mutex> lock(
                mutex_
            );


            if (
                turnActive_
                &&
                inputFinished_
                &&
                speechQueue_.empty()
                &&
                pcmQueue_.empty()
                &&
                !synthBusy_
                &&
                !playbackBusy_
            )
            {
                playbackBusy_ =
                    true;

                needDrain =
                    true;
            }
        }


        if (!needDrain)
        {
            continue;
        }


        /*
         * ALSA里面可能还有最后几十毫秒PCM。
         *
         * 必须真正播完后，
         * 才能发speechFinished。
         */
        if (handle)
        {
            snd_pcm_drain(
                handle
            );


            snd_pcm_close(
                handle
            );


            handle =
                nullptr;


            currentSampleRate =
                0;
        }


        bool finished =
            false;


        {
            std::lock_guard<std::mutex> lock(
                mutex_
            );


            playbackBusy_ =
                false;


            finished =
                markTurnFinishedIfReadyLocked();
        }


        if (finished)
        {
            qDebug()
                << "[TTSService]"
                << "speech turn finished";


            emit speechFinished();
        }
    }


    if (handle)
    {
        snd_pcm_drop(
            handle
        );


        snd_pcm_close(
            handle
        );
    }


    qDebug()
        << "[TTSService]"
        << "stream playback worker stopped";
}


/*
 * ============================================================
 * ALSA写PCM
 * ============================================================
 */
bool TTSService::writePcm(
    snd_pcm_t *handle,
    const int16_t *samples,
    int sampleCount,
    QString *error
)
{
    if (
        !handle
        ||
        !samples
        ||
        sampleCount <= 0
    )
    {
        if (error)
        {
            *error =
                QStringLiteral(
                    "Invalid ALSA PCM data"
                );
        }

        return false;
    }


    snd_pcm_sframes_t offset =
        0;


    while (
        offset
        <
        sampleCount
    )
    {
        /*
         * stop时尽快离开。
         */
        if (stopRequested_.load())
        {
            return true;
        }


        const snd_pcm_sframes_t remaining =
            sampleCount
            -
            offset;


        /*
         * 一次最多写1024 samples。
         *
         * 这样以后stop()不会因为一个超大的
         * snd_pcm_writei阻塞太久。
         */
        const snd_pcm_sframes_t frames =
            remaining > 1024
            ?
                1024
            :
                remaining;


        snd_pcm_sframes_t written =
            snd_pcm_writei(
                handle,
                samples + offset,
                frames
            );


        if (written < 0)
        {
            written =
                snd_pcm_recover(
                    handle,
                    static_cast<int>(
                        written
                    ),
                    1
                );


            if (written < 0)
            {
                if (error)
                {
                    *error =
                        QStringLiteral(
                            "ALSA playback failed: %1"
                        )
                        .arg(
                            QString::fromLocal8Bit(
                                snd_strerror(
                                    static_cast<int>(
                                        written
                                    )
                                )
                            )
                        );
                }


                return false;
            }


            continue;
        }


        offset +=
            written;
    }


    return true;
}


/*
 * ============================================================
 * speak
 * ============================================================
 */
void TTSService::speak(
    const QString &text
)
{
    speak(text, 0);
}


void TTSService::speak(
    const QString &text,
    int pauseAfterMs
)
{
    if (!initialized_.load())
    {
        return;
    }


    const QString cleanText =
        text.trimmed();


    if (cleanText.isEmpty())
    {
        return;
    }


    {
        std::lock_guard<std::mutex> lock(
            mutex_
        );


        if (shuttingDown_)
        {
            return;
        }


        /*
         * 上一次stop已经完全结束以后，
         * 新的一轮可以重新使用。
         */
        if (!turnActive_)
        {
            stopRequested_.store(
                false
            );


            speechStartedEmitted_ =
                false;
        }


        /*
         * 当前正在执行stop，
         * 不接受旧回答残余文本。
         */
        if (stopRequested_.load())
        {
            return;
        }


        turnActive_ =
            true;


        inputFinished_ =
            false;


        busy_.store(
            true
        );


        SpeechItem item;
        item.text = cleanText;
        item.pauseAfterMs =
            std::max(0, std::min(pauseAfterMs, 1000));

        speechQueue_.push_back(std::move(item));


        qDebug()
            << "[TTSService]"
            << "text queued:"
            << cleanText
            << "pauseAfterMs ="
            << pauseAfterMs
            << "queue ="
            << speechQueue_.size();
    }


    speechCv_.notify_one();
}


/*
 * ============================================================
 * VehicleAssistant回答文本结束
 * ============================================================
 */
void TTSService::finishInput()
{
    {
        std::lock_guard<std::mutex> lock(
            mutex_
        );


        if (!turnActive_)
        {
            return;
        }


        inputFinished_ =
            true;


        qDebug()
            << "[TTSService]"
            << "input finished";
    }


    /*
     * 让播放线程负责：
     *
     * PCM全部播放
     * → drain
     * → speechFinished
     */
    speechCv_.notify_all();

    pcmCv_.notify_all();
}


/*
 * ============================================================
 * stop
 * ============================================================
 */
void TTSService::stop()
{
    {
        std::lock_guard<std::mutex> lock(
            mutex_
        );


        if (!turnActive_)
        {
            return;
        }


        qDebug()
            << "[TTSService]"
            << "stop requested";


        stopRequested_.store(
            true
        );


        /*
         * 后面尚未开始合成的文字全部丢弃。
         */
        speechQueue_.clear();


        /*
         * 已经生成但还没写进ALSA的PCM全部丢弃。
         */
        pcmQueue_.clear();


        inputFinished_ =
            true;
    }


    /*
     * Matcha会在RKNN阶段之间检查stopRequested。
     */
    speechCv_.notify_all();

    pcmCv_.notify_all();
}


bool TTSService::markTurnFinishedIfReadyLocked()
{
    if (
        !turnActive_
        ||
        !inputFinished_
        ||
        !speechQueue_.empty()
        ||
        !pcmQueue_.empty()
        ||
        synthBusy_
        ||
        playbackBusy_
    )
    {
        return false;
    }


    turnActive_ =
        false;


    inputFinished_ =
        false;


    speechStartedEmitted_ =
        false;


    busy_.store(
        false
    );


    return true;
}


bool TTSService::isInitialized() const
{
    return initialized_.load();
}


bool TTSService::isBusy() const
{
    return busy_.load();
}


void TTSService::release()
{
    initialized_.store(
        false
    );


    modelDir_.clear();
}
