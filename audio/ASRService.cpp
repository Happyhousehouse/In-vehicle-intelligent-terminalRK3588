#include "ASRService.h"

#include <QDebug>

#include <cstdint>
#include <cstring>
#include <vector>


ASRService::ASRService(QObject *parent)
    :
    QObject(parent)
{
}


ASRService::~ASRService()
{
    {
        std::lock_guard<std::mutex> lock(workerMutex_);

        if (workerThread_.joinable())
        {
            workerThread_.join();
        }
    }

    release();
}


bool ASRService::init(
    const std::string &modelDir
)
{
    if (initialized_.load())
    {
        return true;
    }

    release();


    const std::string modelPath =
        modelDir + "/model.int8.onnx";

    const std::string tokensPath =
        modelDir + "/tokens.txt";


    qDebug()
        << "[ASRService] initializing";

    qDebug()
        << "[ASRService] model ="
        << QString::fromStdString(modelPath);


    SherpaOnnxOfflineRecognizerConfig config;

    std::memset(
        &config,
        0,
        sizeof(config)
    );


    /*
     * ========================================================
     * 输入特征
     * ========================================================
     */

    config.feat_config.sample_rate =
        kSampleRate;

    config.feat_config.feature_dim =
        80;


    /*
     * ========================================================
     * SenseVoice
     * ========================================================
     */

    config.model_config.sense_voice.model =
        modelPath.c_str();

    config.model_config.sense_voice.language =
        "zh";

    config.model_config.sense_voice.use_itn =
        1;


    config.model_config.tokens =
        tokensPath.c_str();


    /*
     * 和刚才命令行验证参数保持一致
     */
    config.model_config.num_threads =
        2;

    config.model_config.provider =
        "cpu";

    config.model_config.debug =
        0;


    config.decoding_method =
        "greedy_search";


    /*
     * ========================================================
     * 模型只加载一次
     * ========================================================
     */

    recognizer_ =
        SherpaOnnxCreateOfflineRecognizer(
            &config
        );


    if (!recognizer_)
    {
        const QString error =
            QStringLiteral(
                "Create SenseVoice recognizer failed"
            );

        qWarning()
            << "[ASRService]"
            << error;

        emit errorOccurred(error);

        return false;
    }


    initialized_.store(true);


    qDebug()
        << "[ASRService]"
        << "initialized successfully";


    return true;
}


void ASRService::recognize(
    const QByteArray &pcm
)
{
    if (
        !initialized_.load()
        ||
        !recognizer_
    )
    {
        qWarning()
            << "[ASRService]"
            << "not initialized";

        return;
    }


    if (pcm.isEmpty())
    {
        return;
    }


    /*
     * 防止两句话同时进入同一个 ASR。
     */
    bool expected = false;

    if (
        !busy_.compare_exchange_strong(
            expected,
            true
        )
    )
    {
        qWarning()
            << "[ASRService]"
            << "recognizer is busy";

        return;
    }


    /*
     * 上一条 worker 已经结束的话，
     * 把 thread 对象回收。
     */
    {
        std::lock_guard<std::mutex> lock(
            workerMutex_
        );

        if (workerThread_.joinable())
        {
            workerThread_.join();
        }


        /*
         * QByteArray 按值复制。
         *
         * VAD 后面即使释放原始数据，
         * ASR worker 仍然拥有自己的 PCM。
         */
        const QByteArray pcmCopy = pcm;


        workerThread_ =
            std::thread(
                [this, pcmCopy]()
                {
                    qDebug()
                        << "[ASRService]"
                        << "Recognizing...";

                    emit recognitionStarted();


                    QString error;

                    const QString text =
                        recognizeImpl(
                            pcmCopy,
                            &error
                        );


                    busy_.store(false);


                    if (!error.isEmpty())
                    {
                        qWarning()
                            << "[ASRService]"
                            << error;

                        emit errorOccurred(
                            error
                        );

                        return;
                    }


                    qDebug()
                        << "[ASRService]"
                        << "Result:"
                        << text;


                    emit recognitionFinished(
                        text
                    );
                }
            );
    }
}


QString ASRService::recognizeImpl(
    const QByteArray &pcm,
    QString *error
)
{
    if (
        pcm.size()
        % static_cast<int>(
            sizeof(int16_t)
        )
        != 0
    )
    {
        if (error)
        {
            *error =
                QStringLiteral(
                    "Invalid S16_LE PCM size"
                );
        }

        return {};
    }


    const int sampleCount =
        pcm.size()
        /
        static_cast<int>(
            sizeof(int16_t)
        );


    const int16_t *pcm16 =
        reinterpret_cast<const int16_t *>(
            pcm.constData()
        );


    /*
     * ========================================================
     * S16_LE
     *
     * -32768 ~ 32767
     *
     * ↓
     *
     * float
     * -1.0 ~ +1.0
     * ========================================================
     */

    std::vector<float> samples(
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
        samples[
            static_cast<std::size_t>(i)
        ] =
            static_cast<float>(
                pcm16[i]
            )
            /
            32768.0f;
    }


    /*
     * 每句话创建一个新的 stream。
     */
    const SherpaOnnxOfflineStream *stream =
        SherpaOnnxCreateOfflineStream(
            recognizer_
        );


    if (!stream)
    {
        if (error)
        {
            *error =
                QStringLiteral(
                    "Create offline ASR stream failed"
                );
        }

        return {};
    }


    /*
     * Offline ASR：
     *
     * 一次性送入整句话。
     */
    SherpaOnnxAcceptWaveformOffline(
        stream,
        kSampleRate,
        samples.data(),
        sampleCount
    );


    /*
     * 真正进行 SenseVoice 推理。
     */
    SherpaOnnxDecodeOfflineStream(
        recognizer_,
        stream
    );


    const SherpaOnnxOfflineRecognizerResult *result =
        SherpaOnnxGetOfflineStreamResult(
            stream
        );


    if (!result)
    {
        SherpaOnnxDestroyOfflineStream(
            stream
        );

        if (error)
        {
            *error =
                QStringLiteral(
                    "Get ASR result failed"
                );
        }

        return {};
    }


    QString text;


    if (result->text)
    {
        text =
            QString::fromUtf8(
                result->text
            ).trimmed();
    }


    SherpaOnnxDestroyOfflineRecognizerResult(
        result
    );


    SherpaOnnxDestroyOfflineStream(
        stream
    );


    return text;
}


bool ASRService::isInitialized() const
{
    return initialized_.load();
}


bool ASRService::isBusy() const
{
    return busy_.load();
}


void ASRService::release()
{
    initialized_.store(false);

    if (recognizer_)
    {
        SherpaOnnxDestroyOfflineRecognizer(
            recognizer_
        );

        recognizer_ =
            nullptr;
    }
}