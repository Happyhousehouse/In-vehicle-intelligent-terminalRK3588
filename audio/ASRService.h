#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#include "sherpa-onnx/c-api/c-api.h"


class ASRService : public QObject
{
    Q_OBJECT

public:
    explicit ASRService(QObject *parent = nullptr);

    ~ASRService();

    bool init(const std::string &modelDir);

    bool isInitialized() const;
    bool isBusy() const;


public slots:
    // 输入：
    // 16000 Hz
    // mono
    // S16_LE
    // 一整句话
    void recognize(const QByteArray &pcm);


signals:
    void recognitionStarted();

    void recognitionFinished(QString text);

    void errorOccurred(QString error);


private:
    QString recognizeImpl(
        const QByteArray &pcm,
        QString *error
    );

    void release();


private:
    const SherpaOnnxOfflineRecognizer *recognizer_ =
        nullptr;

    std::atomic<bool> initialized_{false};
    std::atomic<bool> busy_{false};

    std::thread workerThread_;
    std::mutex workerMutex_;

    static constexpr int kSampleRate = 16000;
};