#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>

#include <alsa/asoundlib.h>

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>


class AudioInputService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(
        bool running
        READ isRunning
        NOTIFY runningChanged
    )

    Q_PROPERTY(
        double peakLevel
        READ peakLevel
        NOTIFY levelChanged
    )

    Q_PROPERTY(
        double rmsLevel
        READ rmsLevel
        NOTIFY levelChanged
    )


public:

    explicit AudioInputService(
        QObject *parent = nullptr
    );

    ~AudioInputService();


    /*
     * 打开 ALSA 麦克风。
     *
     * 当前摄像头麦克风：
     *
     * hw:CARD=Camera,DEV=0
     */
    bool open(
        const std::string &device,
        unsigned int sampleRate = 16000,
        unsigned int channels = 1
    );


    /*
     * 开始后台持续采集。
     */
    Q_INVOKABLE
    bool start();


    /*
     * 停止采集。
     */
    Q_INVOKABLE
    void stop();


    /*
     * 关闭 ALSA 设备。
     */
    void close();


    bool isRunning() const;

    double peakLevel() const;

    double rmsLevel() const;


signals:

    /*
     * 采集运行状态改变。
     */
    void runningChanged();


    /*
     * Peak / RMS 更新。
     */
    void levelChanged();


    /*
     * 每次输出一块 PCM。
     *
     * 当前目标：
     *
     * 16000 Hz
     * mono
     * S16_LE
     * 20ms
     *
     * 即：
     *
     * 320 samples
     * 640 bytes
     */
    void pcmDataReady(
        QByteArray pcm
    );


    void errorOccurred(
        QString error
    );


private:

    void captureLoop();


    void updateLevel(
        const int16_t *samples,
        std::size_t count
    );


private:

    snd_pcm_t *pcmHandle_ =
        nullptr;


    std::string device_;


    unsigned int sampleRate_ =
        16000;


    unsigned int channels_ =
        1;


    /*
     * 16000 Hz × 20 ms
     *
     * = 320 frames
     */
    snd_pcm_uframes_t framesPerRead_ =
        320;


    std::thread captureThread_;


    std::atomic<bool> running_{
        false
    };


    /*
     * 范围：
     *
     * 0.0 ~ 1.0
     */
    std::atomic<double> peakLevel_{
        0.0
    };


    std::atomic<double> rmsLevel_{
        0.0
    };
};