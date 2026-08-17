#pragma once

#include <QObject>
#include <QByteArray>

#include <atomic>
#include <cstdint>
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


    static constexpr int kSampleRate =
        16000;
};