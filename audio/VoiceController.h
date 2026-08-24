#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QTimer>


class WakeWordService;
class VADService;
class ASRService;
class VehicleAssistant;
class TTSService;


/*
 * ============================================================
 * VoiceController
 *
 * 负责整个语音会话状态管理：
 *
 * WakeWord
 *   ↓
 * VAD
 *   ↓
 * ASR
 *   ↓
 * VehicleAssistant
 *
 *
 * 会话规则：
 *
 * 1. 检测到“你好小车”以后：
 *    最多等待5秒用户开口。
 *
 * 2. 如果5秒无人讲话：
 *    认为误唤醒，退出语音模式。
 *
 * 3. 如果已经完成至少一轮正式交流：
 *    AI回答结束以后等待30秒。
 *
 * 4. 30秒内用户再次讲话：
 *    继续当前语音会话。
 *
 * 5. 用户一开始讲话：
 *    立即停止5秒/30秒Timer。
 *
 * 6. ASR、VehicleAssistant、RKLLM处理期间：
 *    不计入等待时间。
 *
 * 7. 用户说“退出语音助手”等明确命令：
 *    立即退出语音Session。
 * ============================================================
 */
class VoiceController : public QObject
{
    Q_OBJECT


public:

    explicit VoiceController(
        WakeWordService *wakeWordService,
        VADService *vadService,
        ASRService *asrService,
        VehicleAssistant *assistant,
        TTSService *ttsService,
        QObject *parent = nullptr
    );

    ~VoiceController() override = default;


private:

    /*
     * ========================================================
     * Voice Session状态
     * ========================================================
     */
    enum class State
    {
        /*
        * 等待唤醒词。
        */
        WAIT_WAKE = 0,


        /*
        * 唤醒后等待第一句话。
        *
        * 5秒超时。
        */
        WAIT_FIRST_COMMAND,


        /*
        * 用户正在说正常命令。
        */
        LISTENING,


        /*
        * ASR正在识别正常命令。
        */
        RECOGNIZING,


        /*
        * AI正在处理 / 回答。
        *
        * 这个阶段会同时开启VAD，
        * 专门监听“停止回答”。
        */
        ANSWERING,


        /*
        * AI回答期间，
        * 检测到用户开始讲话。
        */
        ANSWER_INTERRUPT_LISTENING,


        /*
        * AI回答期间，
        * 正在识别用户刚才说的话。
        *
        * 主要判断是不是：
        *
        * “停止回答”
        */
        ANSWER_INTERRUPT_RECOGNIZING,


        /*
        * AI已经完成一轮回答，
        * 等待下一条命令。
        *
        * 30秒超时。
        */
        WAIT_FOLLOWUP
    };


private slots:

    /*
     * 检测到：
     *
     * “你好小车”
     */
    void onWakeWordDetected(
        const QString &keyword
    );


    /*
     * VAD发现用户真正开始讲话。
     *
     * 用于立即停止5秒/30秒Timer。
     */
    void onSpeechStarted();


    /*
     * VAD已经拿到一整句话。
     */
    void onSpeechSegmentReady(
        const QByteArray &pcm
    );


    /*
     * ASR识别完成。
     */
    void onRecognitionFinished(
        const QString &text
    );


    /*
     * ASR失败。
     */
    void onAsrError(
        const QString &error
    );

    /*
    * VehicleAssistant生成出一段可以朗读的文本。
    */
    void onAssistantSpeechChunkReady(
        const QString &text,
        int pauseAfterMs
    );


    /*
    * TTS真正开始播放。
    *
    * 从这一刻开始关闭VAD，
    * 避免麦克风收到扬声器自己的声音。
    */
    void onTtsSpeechStarted();


    /*
    * 整轮TTS真正播放结束。
    */
    void onTtsSpeechFinished();


    /*
    * 用户主动打断后，TTS队列和播放已经停止。
    */
    void onTtsSpeechStopped();
    /*
     * VehicleAssistant一轮回答真正结束。
     */
    void onAssistantAnswerFinished();


    /*
     * VehicleAssistant拒绝本轮请求。
     */
    void onChatRejected(
        const QString &reason
    );


    /*
     * 5秒 / 30秒等待Timer超时。
     */
    void onWaitTimeout();


private:

    /*
     * 开始：
     *
     * 第一次命令5秒等待。
     */
    void startFirstCommandWait();


    /*
     * 开始：
     *
     * 正式会话30秒等待。
     */
    void startFollowupWait();


    /*
     * ASR异常等情况下，
     * 根据当前会话阶段恢复等待。
     */
    void resumeCurrentWait();

    /*
    * AI开始回答以后，
    * 打开VAD监听“停止回答”。
    *
    * 注意：
    * 这里没有5秒/30秒Timer。
    */
    void startAnswerInterruptListening();


    /*
    * 判断是不是停止当前AI回答的命令。
    */
    bool isStopAnswerCommand(
        const QString &text
    ) const;


    /*
    * 一轮AI回答真正结束以后，
    * 统一进入30秒连续会话。
    */
    void finishAssistantTurn();
    /*
     * 完全退出当前语音Session。
     *
     * 最终：
     *
     * VAD OFF
     * WakeWord ON
     * State = WAIT_WAKE
     */
    void exitVoiceSession(
        const QString &reason
    );


    /*
     * 判断是不是：
     *
     * “退出语音助手”
     * “结束语音助手”
     * 等退出命令。
     */
    bool isExitCommand(
        const QString &text
    ) const;


    /*
     * 对ASR文字做简单标准化。
     *
     * 主要去：
     *
     * 句号
     * 问号
     * 感叹号
     * 空格
     */
    QString normalizeCommand(
        const QString &text
    ) const;


    /*
     * 修改状态。
     */
    void setState(
        State state
    );


    /*
     * 调试日志使用。
     */
    const char *stateName(
        State state
    ) const;


private:

    /*
     * ========================================================
     * 外部服务
     *
     * VoiceController不拥有这些对象。
     *
     * 生命周期仍然由main.cpp负责。
     * ========================================================
     */

    WakeWordService *wakeWordService_ =
        nullptr;

    VADService *vadService_ =
        nullptr;

    ASRService *asrService_ =
        nullptr;

    VehicleAssistant *assistant_ =
        nullptr;

    TTSService *ttsService_ =
        nullptr;


    /*
    * 当前扬声器是否正在播放AI语音。
    *
    * 用于屏蔽可能已经排进Qt事件队列的
    * VAD speechStarted / speechSegmentReady。
    */
    bool ttsSpeaking_ =
        false;
    /*
     * 当前Voice状态。
     */
    State state_ =
        State::WAIT_WAKE;


    /*
     * ========================================================
     * 当前语音Session是否已经正式建立交流
     * ========================================================
     *
     * false：
     *
     * “你好小车”
     *    ↓
     * 仍然处于第一次命令阶段
     * 使用5秒。
     *
     *
     * true：
     *
     * 已经至少完成：
     *
     * 用户问题
     *    ↓
     * AI回答
     *
     * 后面使用30秒。
     */
    bool conversationEstablished_ =
        false;


    /*
     * ========================================================
     * 当前语音Session是否已经创建新的Conversation
     * ========================================================
     *
     * 注意：
     *
     * 不是一听到“你好小车”就newConversation。
     *
     * 而是第一次真正识别到有效用户问题以后，
     * 才创建Conversation。
     *
     * 防止误唤醒产生大量空历史会话。
     */
    bool sessionConversationStarted_ =
        false;

    /*
    * AI回答期间如果用户正在说“停止回答”，
    * 恰好此时AI自己已经回答结束，
    * 用这个变量记录。
    */
    bool assistantFinishedDuringInterrupt_ =
        false;
    /*
     * 统一等待Timer。
     *
     * 根据不同状态：
     *
     * WAIT_FIRST_COMMAND
     *     → 5000ms
     *
     * WAIT_FOLLOWUP
     *     → 30000ms
     */
    QTimer waitTimer_;


    /*
     * 误唤醒等待时间。
     */
    static constexpr int kFirstCommandTimeoutMs =
        5000;


    /*
     * 已经正式交流后的等待时间。
     */
    static constexpr int kFollowupTimeoutMs =
        30000;
};
