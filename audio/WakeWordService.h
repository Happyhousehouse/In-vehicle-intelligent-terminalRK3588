#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include "sherpa-onnx/c-api/c-api.h"


class WakeWordService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(
        bool initialized
        READ isInitialized
        NOTIFY initializedChanged
    )

    Q_PROPERTY(
        bool enabled
        READ isEnabled
        WRITE setEnabled
        NOTIFY enabledChanged
    )


public:

    explicit WakeWordService(
        QObject *parent = nullptr
    );

    ~WakeWordService();


    /*
     * 初始化 KWS 模型。
     *
     * modelDir:
     *
     * ~/VehicleTerminal/model/kws/
     * sherpa-onnx-kws-zipformer-wenetspeech-3.3M-2024-01-01
     */
    bool init(
        const std::string &modelDir
    );


    bool isInitialized() const;

    bool isEnabled() const;


    Q_INVOKABLE
    void setEnabled(
        bool enabled
    );


public slots:

    /*
     * 接收 AudioInputService 输出的：
     *
     * 16000 Hz
     * mono
     * S16_LE
     *
     * 当前每次约：
     *
     * 320 samples
     * 640 bytes
     * 20 ms
     */
    void processPcm(
        const QByteArray &pcm
    );


signals:

    void initializedChanged();

    void enabledChanged();


    /*
     * 检测到唤醒词。
     *
     * 当前应该输出：
     *
     * "你好小车"
     */
    void wakeWordDetected(
        QString keyword
    );


    void errorOccurred(
        QString error
    );


private:

    void release();


private:

    const SherpaOnnxKeywordSpotter *spotter_ =
        nullptr;

    const SherpaOnnxOnlineStream *stream_ =
        nullptr;


    std::atomic<bool> initialized_{
        false
    };

    std::atomic<bool> enabled_{
        true
    };


    static constexpr int kSampleRate =
        16000;
};