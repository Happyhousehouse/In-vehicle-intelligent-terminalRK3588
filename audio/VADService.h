#pragma once

#include <QObject>
#include <QByteArray>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "sherpa-onnx/c-api/c-api.h"


class VADService : public QObject
{
    Q_OBJECT

public:

    explicit VADService(
        QObject *parent = nullptr
    );

    ~VADService();


    bool init(
        const std::string &modelPath
    );


    bool isInitialized() const;

    bool isListening() const;


public slots:

    // 开始等待用户说命令
    void startListening();

    // 主动停止
    void stopListening();

    // AudioInputService 的 PCM
    void processPcm(
        const QByteArray &pcm
    );


signals:

    void listeningStarted();

    void speechStarted();

    /*
     * 一整句话已经结束。
     *
     * 输出：
     * 16000 Hz
     * mono
     * S16_LE
     */
    void speechSegmentReady(
        QByteArray pcm
    );

    void listeningStopped();

    void errorOccurred(
        QString error
    );


private:

    void release();


private:

    const SherpaOnnxVoiceActivityDetector *vad_ =
        nullptr;


    std::atomic<bool> initialized_{
        false
    };

    std::atomic<bool> listening_{
        false
    };


    bool speechActive_ =
        false;

    // Exact PCM accepted since the latest startListening(). Sherpa exposes
    // segment->start, so we can restore a short prefix that VAD excluded
    // without duplicating samples already present in the segment.
    QByteArray listeningPcmHistory_;

    mutable std::mutex vadMutex_;


    static constexpr int kSampleRate =
        16000;

    static constexpr int kPreRollMs =
        250;

    static constexpr int kPostRollMs =
        300;
};
