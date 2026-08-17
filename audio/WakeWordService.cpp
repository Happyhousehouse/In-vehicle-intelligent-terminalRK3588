#include "WakeWordService.h"

#include <QDebug>

#include <cstring>


WakeWordService::WakeWordService(
    QObject *parent
)
    :
    QObject(parent)
{
}


WakeWordService::~WakeWordService()
{
    qDebug()
        << "[WakeWordService] destructor";

    /*
     * 注意：
     *
     * 程序退出时必须先停止 AudioInputService，
     * 确保 processPcm() 不再进入，
     * 然后才能释放这里的 stream / spotter。
     */
    release();
}


/*
 * ============================================================
 * 初始化
 * ============================================================
 */

bool WakeWordService::init(
    const std::string &modelDir
)
{
    /*
     * 已经初始化。
     */
    if (initialized_.load())
    {
        return true;
    }


    release();


    /*
     * ========================================================
     * 模型路径
     *
     * 使用我们已经验证成功的 INT8 模型。
     * ========================================================
     */

    const std::string encoder =
        modelDir
        +
        "/encoder-epoch-12-avg-2-chunk-16-left-64.int8.onnx";


    const std::string decoder =
        modelDir
        +
        "/decoder-epoch-12-avg-2-chunk-16-left-64.int8.onnx";


    const std::string joiner =
        modelDir
        +
        "/joiner-epoch-12-avg-2-chunk-16-left-64.int8.onnx";


    const std::string tokens =
        modelDir
        +
        "/tokens.txt";


    const std::string keywords =
        modelDir
        +
        "/keywords_vehicle.txt";


    qDebug()
        << "[WakeWordService]"
        << "initializing";


    qDebug()
        << "[WakeWordService]"
        << "encoder ="
        << QString::fromStdString(
               encoder
           );


    qDebug()
        << "[WakeWordService]"
        << "keywords ="
        << QString::fromStdString(
               keywords
           );


    /*
     * ========================================================
     * sherpa-onnx KWS config
     * ========================================================
     */

    SherpaOnnxKeywordSpotterConfig config;

    std::memset(
        &config,
        0,
        sizeof(config)
    );


    /*
     * Feature
     */
    config.feat_config.sample_rate =
        kSampleRate;

    config.feat_config.feature_dim =
        80;


    /*
     * Transducer model
     */
    config.model_config.transducer.encoder =
        encoder.c_str();

    config.model_config.transducer.decoder =
        decoder.c_str();

    config.model_config.transducer.joiner =
        joiner.c_str();


    config.model_config.tokens =
        tokens.c_str();


    /*
     * CPU推理。
     *
     * 前面已经实测：
     *
     * RTF ≈ 0.056
     */
    config.model_config.provider =
        "cpu";

    config.model_config.num_threads =
        2;

    config.model_config.debug =
        0;


    /*
     * ========================================================
     * KWS参数
     *
     * 先完全保持我们命令行验证成功时的参数：
     *
     * max_active_paths      = 4
     * trailing blanks       = 1
     * score                 = 1
     * threshold             = 0.25
     * ========================================================
     */

    config.max_active_paths =
        4;

    config.num_trailing_blanks =
        1;

    config.keywords_score =
        1.0f;

    config.keywords_threshold =
        0.25f;


    config.keywords_file =
        keywords.c_str();


    /*
     * ========================================================
     * 创建 KeywordSpotter
     * ========================================================
     */

    spotter_ =
        SherpaOnnxCreateKeywordSpotter(
            &config
        );


    if (!spotter_)
    {
        const QString error =
            QStringLiteral(
                "创建 sherpa-onnx KeywordSpotter 失败。"
            );


        qWarning()
            << "[WakeWordService]"
            << error;


        emit errorOccurred(
            error
        );


        return false;
    }


    /*
     * ========================================================
     * 创建持续 KWS stream
     * ========================================================
     */

    stream_ =
        SherpaOnnxCreateKeywordStream(
            spotter_
        );


    if (!stream_)
    {
        const QString error =
            QStringLiteral(
                "创建 sherpa-onnx keyword stream 失败。"
            );


        qWarning()
            << "[WakeWordService]"
            << error;


        SherpaOnnxDestroyKeywordSpotter(
            spotter_
        );


        spotter_ =
            nullptr;


        emit errorOccurred(
            error
        );


        return false;
    }


    initialized_.store(
        true
    );


    emit initializedChanged();


    qDebug()
        << "[WakeWordService]"
        << "initialized successfully";


    return true;
}


/*
 * ============================================================
 * PCM输入
 * ============================================================
 */

void WakeWordService::processPcm(
    const QByteArray &pcm
)
{
    if (
        !initialized_.load()
        ||
        !enabled_.load()
        ||
        !spotter_
        ||
        !stream_
    )
    {
        return;
    }


    /*
     * S16_LE：
     *
     * 每个 sample 2 bytes。
     */
    if (
        pcm.isEmpty()
        ||
        (pcm.size() % sizeof(int16_t)) != 0
    )
    {
        return;
    }


    const int16_t *samples16 =
        reinterpret_cast<
            const int16_t *
        >(
            pcm.constData()
        );


    const int sampleCount =
        pcm.size()
        /
        static_cast<int>(
            sizeof(int16_t)
        );


    /*
     * ========================================================
     * int16 PCM
     *
     * -32768 ~ 32767
     *
     * 转换成 sherpa-onnx 要求的：
     *
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
                samples16[i]
            )
            /
            32768.0f;
    }


    /*
     * ========================================================
     * PCM送入 streaming KWS
     * ========================================================
     */

    SherpaOnnxOnlineStreamAcceptWaveform(
        stream_,
        kSampleRate,
        samples.data(),
        sampleCount
    );


    /*
     * ========================================================
     * 只要内部积累的数据足够，
     * 就立即进行 decode。
     * ========================================================
     */

    while (
        SherpaOnnxIsKeywordStreamReady(
            spotter_,
            stream_
        )
    )
    {
        SherpaOnnxDecodeKeywordStream(
            spotter_,
            stream_
        );


        const SherpaOnnxKeywordResult *result =
            SherpaOnnxGetKeywordResult(
                spotter_,
                stream_
            );


        if (!result)
        {
            continue;
        }


        /*
         * 先复制出来。
         *
         * result Destroy 后里面指针就无效了。
         */
        QString keyword;


        if (
            result->keyword
            &&
            result->keyword[0] != '\0'
        )
        {
            keyword =
                QString::fromUtf8(
                    result->keyword
                );
        }


        SherpaOnnxDestroyKeywordResult(
            result
        );


        /*
         * ====================================================
         * 没检测到关键词。
         * ====================================================
         */

        if (keyword.isEmpty())
        {
            continue;
        }


        /*
         * ====================================================
         * 检测成功。
         *
         * 官方要求：
         * 一旦检测到关键词，应立即 reset stream。
         * ====================================================
         */

        SherpaOnnxResetKeywordStream(
            spotter_,
            stream_
        );


        qDebug()
            << "[WakeWordService]"
            << "★★★★★ WAKE WORD DETECTED:"
            << keyword;


        emit wakeWordDetected(
            keyword
        );


        /*
         * 一次 processPcm()
         * 最多向上层报告一次唤醒。
         */
        break;
    }
}


/*
 * ============================================================
 * Enable / Disable
 * ============================================================
 */

bool WakeWordService::isEnabled() const
{
    return enabled_.load();
}


void WakeWordService::setEnabled(
    bool enabled
)
{
    const bool old =
        enabled_.exchange(
            enabled
        );


    if (old == enabled)
    {
        return;
    }


    qDebug()
        << "[WakeWordService]"
        << "enabled ="
        << enabled;


    /*
     * 从 disabled 重新开启时，
     * 把之前残留的 KWS 状态清掉。
     */
    if (
        enabled
        &&
        spotter_
        &&
        stream_
    )
    {
        SherpaOnnxResetKeywordStream(
            spotter_,
            stream_
        );
    }


    emit enabledChanged();
}


bool WakeWordService::isInitialized() const
{
    return initialized_.load();
}


/*
 * ============================================================
 * Release
 * ============================================================
 */

void WakeWordService::release()
{
    initialized_.store(
        false
    );


    if (stream_)
    {
        SherpaOnnxDestroyOnlineStream(
            stream_
        );


        stream_ =
            nullptr;
    }


    if (spotter_)
    {
        SherpaOnnxDestroyKeywordSpotter(
            spotter_
        );


        spotter_ =
            nullptr;
    }
}