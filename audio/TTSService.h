#pragma once

#include <QObject>
#include <QString>

#include <alsa/asoundlib.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class TTSService : public QObject
{
    Q_OBJECT

public:

    explicit TTSService(
        QObject *parent = nullptr
    );

    ~TTSService() override;


    bool init(
        const std::string &modelDir
    );


    bool isInitialized() const;

    bool isBusy() const;


public slots:

    /*
     * 加入一段要朗读的文字。
     */
    void speak(
        const QString &text
    );

    void speak(
        const QString &text,
        int pauseAfterMs
    );


    /*
     * 上层告诉TTS：
     *
     * 当前AI回答已经不会再产生新文字。
     */
    void finishInput();


    /*
     * 立即停止当前TTS。
     *
     * 后面实现“停止回答”会直接使用。
     */
    void stop();


signals:

    /*
     * 整轮TTS第一次真正开始出声。
     */
    void speechStarted();


    /*
     * 某一个文字块开始播放。
     */
    void speechChunkStarted(
        QString text
    );


    /*
     * 某一个文字块已经写完。
     */
    void speechChunkFinished(
        QString text
    );


    /*
     * 整轮语音真正播放完成。
     */
    void speechFinished();


    /*
     * 当前语音被主动停止。
     */
    void speechStopped();


    void errorOccurred(
        QString error
    );


private:
    struct SpeechItem
    {
        QString text;
        int pauseAfterMs = 0;
    };


    /*
     * ========================================================
     * PCM小块
     * ========================================================
     *
     * 不再保存整句话的PCM。
     *
     * Matcha每生成一个语音块，
     * 就形成一个PcmPacket。
     */
    struct PcmPacket
    {
        QString text;

        std::vector<int16_t> pcm;

        int sampleRate =
            0;

        bool startOfText =
            false;

        bool endOfText =
            false;
    };


private:
    /*
     * 文本 → sherpa流式生成。
     */
    void synthLoop();


    /*
     * PCM Queue → ALSA。
     */
    void playbackLoop();


    /* 对一段文字执行一次Matcha RKNN推理。 */
    bool generateStreaming(
        const SpeechItem &item,
        QString *error
    );


    /*
     * float PCM转成S16_LE并进入播放队列。
     */
    bool enqueueFloatPcm(
        const float *samples,
        int sampleCount,
        const QString &text,
        bool startOfText
    );


    /*
     * 将PCM真正写入ALSA。
     */
    bool writePcm(
        snd_pcm_t *handle,
        const int16_t *samples,
        int sampleCount,
        QString *error
    );


    /*
     * 必须持有mutex_调用。
     */
    bool markTurnFinishedIfReadyLocked();


    void release();


private:

    std::string modelDir_;

    std::atomic<bool> initialized_{
        false
    };

    std::atomic<bool> busy_{
        false
    };


    /*
     * stop()之后，Matcha在各RKNN阶段之间检查它；正在执行的
     * rknn_run()返回后会丢弃结果。
     */
    std::atomic<bool> stopRequested_{
        false
    };


    std::thread synthThread_;

    std::thread playbackThread_;


    /*
     * LLM分好的文字队列。
     */
    std::deque<SpeechItem> speechQueue_;


    /*
     * 真正的流式PCM队列。
     */
    std::deque<PcmPacket> pcmQueue_;


    std::mutex mutex_;

    std::condition_variable speechCv_;

    std::condition_variable pcmCv_;


    bool shuttingDown_ =
        false;


    bool inputFinished_ =
        false;


    bool turnActive_ =
        false;


    bool synthBusy_ =
        false;


    bool playbackBusy_ =
        false;


    bool speechStartedEmitted_ =
        false;


    int sampleRate_ =
        16000;


    static constexpr const char *kPlaybackDevice =
        "plughw:CARD=rockchipes8388,DEV=0";

};
