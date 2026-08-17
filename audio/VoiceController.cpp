#include "VoiceController.h"

#include "WakeWordService.h"
#include "VADService.h"
#include "ASRService.h"
#include "VoiceCommandNormalizer.h"

#include "../assistant/VehicleAssistant.h"

#include <QDebug>

// ============================================================
// Constructor
// ============================================================

VoiceController::VoiceController(
    WakeWordService *wakeWordService,
    VADService *vadService,
    ASRService *asrService,
    VehicleAssistant *assistant,
    QObject *parent
)
    :
    QObject(parent),
    wakeWordService_(wakeWordService),
    vadService_(vadService),
    asrService_(asrService),
    assistant_(assistant)
{
    /*
     * ========================================================
     * 等待计时器
     *
     * WAIT_FIRST_COMMAND:
     *      5 秒
     *
     * WAIT_FOLLOWUP:
     *      30 秒
     * ========================================================
     */

    waitTimer_.setSingleShot(
        true
    );

    waitTimer_.setTimerType(
        Qt::PreciseTimer
    );


    connect(
        &waitTimer_,
        &QTimer::timeout,

        this,
        &VoiceController::onWaitTimeout
    );


    /*
     * ========================================================
     * Wake Word
     * ========================================================
     *
     * WakeWordService 当前可能在音频采集线程
     * 里触发 wakeWordDetected。
     *
     * VoiceController 内部使用 QTimer，
     * 所以统一回 Qt 主线程处理。
     * ========================================================
     */

    connect(
        wakeWordService_,
        &WakeWordService::wakeWordDetected,

        this,
        &VoiceController::onWakeWordDetected,

        Qt::QueuedConnection
    );


    /*
     * ========================================================
     * VAD
     * ========================================================
     */

    connect(
        vadService_,
        &VADService::speechStarted,

        this,
        &VoiceController::onSpeechStarted,

        Qt::QueuedConnection
    );


    connect(
        vadService_,
        &VADService::speechSegmentReady,

        this,
        &VoiceController::onSpeechSegmentReady,

        Qt::QueuedConnection
    );


    /*
     * ========================================================
     * ASR
     * ========================================================
     */

    connect(
        asrService_,
        &ASRService::recognitionFinished,

        this,
        &VoiceController::onRecognitionFinished,

        Qt::QueuedConnection
    );


    connect(
        asrService_,
        &ASRService::errorOccurred,

        this,
        &VoiceController::onAsrError,

        Qt::QueuedConnection
    );


    /*
     * ========================================================
     * VehicleAssistant
     * ========================================================
     */

    connect(
        assistant_,
        &VehicleAssistant::answerFinished,

        this,
        &VoiceController::onAssistantAnswerFinished,

        Qt::QueuedConnection
    );


    connect(
        assistant_,
        &VehicleAssistant::chatRejected,

        this,
        &VoiceController::onChatRejected,

        Qt::QueuedConnection
    );


    qDebug()
        << "[VoiceController]"
        << "initialized";
}


// ============================================================
// Wake Word
// ============================================================

void VoiceController::onWakeWordDetected(
    const QString &keyword
)
{
    /*
     * 只有 WAIT_WAKE 状态允许开始新的语音 Session。
     */
    if (
        state_
        !=
        State::WAIT_WAKE
    )
    {
        return;
    }


    /*
     * 如果 AI 此时正在通过键盘等其他入口回答，
     * 暂时不进入语音会话。
     */
    if (
        assistant_
        &&
        assistant_->isGenerating()
    )
    {
        qWarning()
            << "[VoiceController]"
            << "wake ignored:"
            << "assistant is generating";

        return;
    }


    qDebug()
        << "====================================";

    qDebug()
        << "[VoiceController]"
        << "WAKE WORD:"
        << keyword;

    qDebug()
        << "====================================";


    /*
     * 新的一次语音 Session。
     *
     * 此时还不能认为已经正式交流。
     */
    conversationEstablished_ =
        false;


    /*
     * 此时也不创建新的 Conversation。
     *
     * 等第一次真正识别到有效问题以后
     * 再调用 assistant.newConversation()。
     */
    sessionConversationStarted_ =
        false;
    assistantFinishedDuringInterrupt_ =
        false;

    /*
     * 语音 Session 内关闭唤醒词检测。
     */
    if (wakeWordService_)
    {
        wakeWordService_
            ->setEnabled(
                false
            );
    }


    /*
     * 进入首次命令等待阶段。
     *
     * 5秒没有讲话：
     * 判定为误唤醒。
     */
    startFirstCommandWait();
}


// ============================================================
// 第一次命令等待
// ============================================================

void VoiceController::startFirstCommandWait()
{
    setState(
        State::WAIT_FIRST_COMMAND
    );


    /*
     * 开始 VAD 监听。
     */
    if (vadService_)
    {
        vadService_
            ->startListening();
    }


    /*
     * 误唤醒保护：
     *
     * 5秒。
     */
    waitTimer_.start(
        kFirstCommandTimeoutMs
    );


    qDebug()
        << "[VoiceController]"
        << "waiting first command:"
        << kFirstCommandTimeoutMs
        << "ms";
}


// ============================================================
// 后续连续交流等待
// ============================================================

void VoiceController::startFollowupWait()
{
    setState(
        State::WAIT_FOLLOWUP
    );


    /*
     * AI回答真正结束以后，
     * 才重新开始听用户讲话。
     */
    if (vadService_)
    {
        vadService_
            ->startListening();
    }


    /*
     * 已经正式交流以后：
     *
     * 每一轮给用户30秒继续交流。
     */
    waitTimer_.start(
        kFollowupTimeoutMs
    );


    qDebug()
        << "[VoiceController]"
        << "waiting follow-up:"
        << kFollowupTimeoutMs
        << "ms";
}


// ============================================================
// 恢复等待
// ============================================================

void VoiceController::resumeCurrentWait()
{
    /*
     * 如果已经正式完成至少一轮交流，
     * 使用30秒。
     */
    if (conversationEstablished_)
    {
        startFollowupWait();

        return;
    }


    /*
     * 还没有真正完成第一轮，
     * 仍然使用5秒误唤醒策略。
     */
    startFirstCommandWait();
}


// ============================================================
// 用户开始讲话
// ============================================================

void VoiceController::onSpeechStarted()
{
    /*
     * ========================================================
     * 情况1：
     * 正常等待用户命令
     * ========================================================
     */
    if (
        state_ == State::WAIT_FIRST_COMMAND
        ||
        state_ == State::WAIT_FOLLOWUP
    )
    {
        /*
         * 用户一开口，
         * 立即取消5秒 / 30秒等待。
         */
        waitTimer_.stop();


        setState(
            State::LISTENING
        );


        qDebug()
            << "[VoiceController]"
            << "speech started,"
            << "wait timeout cancelled";

        return;
    }


    /*
     * ========================================================
     * 情况2：
     * AI正在回答
     *
     * 用户此时讲话，
     * 进入“回答打断监听”。
     * ========================================================
     */
    if (
        state_
        ==
        State::ANSWERING
    )
    {
        setState(
            State::ANSWER_INTERRUPT_LISTENING
        );


        qDebug()
            << "[VoiceController]"
            << "speech detected during AI answer";

        return;
    }
}


// ============================================================
// 一整句话完成
// ============================================================

void VoiceController::onSpeechSegmentReady(
    const QByteArray &pcm
)
{
    /*
     * ========================================================
     * 情况1：
     * AI回答期间的打断语音
     * ========================================================
     */
    if (
        state_
        ==
        State::ANSWER_INTERRUPT_LISTENING
    )
    {
        qDebug()
            << "[VoiceController]"
            << "answer interrupt segment ready";


        /*
         * 回答期间没有5秒 / 30秒Timer，
         * 这里防御性停止。
         */
        waitTimer_.stop();


        if (
            !asrService_
            ||
            !asrService_->isInitialized()
        )
        {
            qWarning()
                << "[VoiceController]"
                << "ASR unavailable during interrupt";


            /*
             * AI还在回答：
             * 继续监听停止命令。
             */
            /*
            * 只有 RKLLM 真正在生成时，
            * 才需要监听“停止回答”。
            *
            * 打开天气页面、打开相机等本地命令
            * 会同步 emit answerFinished()，
            * 不应该启动回答打断 VAD。
            */
            if (
                state_
                ==
                State::ANSWERING
                &&
                assistant_
                &&
                assistant_->isGenerating()
            )
            {
                startAnswerInterruptListening();
            }
            else
            {
                /*
                 * AI已经自己回答结束。
                 */
                finishAssistantTurn();
            }


            return;
        }


        if (
            asrService_->isBusy()
        )
        {
            qWarning()
                << "[VoiceController]"
                << "ASR busy during interrupt";


            if (
                assistant_
                &&
                assistant_->isGenerating()
            )
            {
                setState(
                    State::ANSWERING
                );


                startAnswerInterruptListening();
            }
            else
            {
                finishAssistantTurn();
            }


            return;
        }


        /*
         * ====================================================
         * 进入专门的“回答打断ASR”状态
         * ====================================================
         */
        setState(
            State::ANSWER_INTERRUPT_RECOGNIZING
        );


        qDebug()
            << "[VoiceController]"
            << "recognizing stop-answer command";


        asrService_
            ->recognize(
                pcm
            );


        return;
    }


    /*
     * ========================================================
     * 情况2：
     * 正常用户语音
     * ========================================================
     */
    if (
        state_
        !=
        State::LISTENING
        &&
        state_
        !=
        State::WAIT_FIRST_COMMAND
        &&
        state_
        !=
        State::WAIT_FOLLOWUP
    )
    {
        qWarning()
            << "[VoiceController]"
            << "unexpected speech segment"
            << "state ="
            << stateName(
                state_
            );


        return;
    }


    /*
     * 用户已经把一句话说完，
     * 停止5秒 / 30秒等待。
     */
    waitTimer_.stop();


    if (
        !asrService_
        ||
        !asrService_->isInitialized()
    )
    {
        qWarning()
            << "[VoiceController]"
            << "ASR unavailable";


        exitVoiceSession(
            QStringLiteral(
                "ASR unavailable"
            )
        );


        return;
    }


    if (
        asrService_->isBusy()
    )
    {
        qWarning()
            << "[VoiceController]"
            << "ASR is busy";


        resumeCurrentWait();


        return;
    }


    /*
     * 正常命令识别。
     */
    setState(
        State::RECOGNIZING
    );


    qDebug()
        << "[VoiceController]"
        << "recognizing normal command";


    asrService_
        ->recognize(
            pcm
        );
}


// ============================================================
// ASR文字标准化
// ============================================================

QString VoiceController::normalizeCommand(
    const QString &text
) const
{
    return VoiceCommandNormalizer::normalize(
        text
    );
}


// ============================================================
// 退出语音助手命令
// ============================================================

bool VoiceController::isExitCommand(
    const QString &text
) const
{
    const QString cmd =
        normalizeCommand(
            text
        );


    /*
     * 必须使用整句匹配。
     *
     * 不能：
     *
     * cmd.contains("退出")
     *
     * 否则：
     *
     * “退出相机”
     *
     * 也会被误认为退出整个语音助手。
     */
    return
        cmd
        ==
        QStringLiteral(
            "退出语音助手"
        )
        ||
        cmd
        ==
        QStringLiteral(
            "结束语音助手"
        )
        ||
        cmd
        ==
        QStringLiteral(
            "退出语音"
        )
        ||
        cmd
        ==
        QStringLiteral(
            "结束语音"
        )
        ||
        cmd
        ==
        QStringLiteral(
            "退出对话"
        )
        ||
        cmd
        ==
        QStringLiteral(
            "结束对话"
        );
}


// ============================================================
// ASR识别完成
// ============================================================

void VoiceController::onRecognitionFinished(
    const QString &text
)
{
    /*
     * ========================================================
     * AI回答期间的打断语音识别结果
     * ========================================================
     */
    if (
        state_
        ==
        State::ANSWER_INTERRUPT_RECOGNIZING
    )
    {
        const QString interruptCommand =
            normalizeCommand(
                text
            );


        qDebug()
            << "[VoiceController]"
            << "interrupt ASR result:"
            << interruptCommand;


        /*
         * 用户说了：
         *
         * “停止回答”
         */
        if (
            isStopAnswerCommand(
                interruptCommand
            )
        )
        {
            qDebug()
                << "[VoiceController]"
                << "STOP ANSWER command detected";


            /*
             * 如果AI此时仍然在生成，
             * 立即调用现有的 stopGeneration()。
             */
            if (
                assistant_
                &&
                assistant_->isGenerating()
            )
            {
                qDebug()
                    << "[VoiceController]"
                    << "request assistant stop generation";


                /*
                * 回到 ANSWERING，
                * 等 stopGeneration() 最终触发 answerFinished。
                */
                setState(
                    State::ANSWERING
                );


                /*
                * 真正停止当前 RKLLM 回答。
                */
                assistant_
                    ->stopGeneration();


                return;
            }


            /*
             * ASR识别期间，
             * AI可能刚好自己回答完了。
             */
            assistantFinishedDuringInterrupt_ =
                false;


            finishAssistantTurn();

            return;
        }


        /*
         * ====================================================
         * 用户说的不是“停止回答”
         * ====================================================
         *
         * 第一版先不把它当新的问题。
         *
         * 如果AI仍然生成：
         * 继续监听“停止回答”。
         */
        if (
            assistant_
            &&
            assistant_->isGenerating()
        )
        {
            qDebug()
                << "[VoiceController]"
                << "stopping assistant generation";


            /*
            * 回到 ANSWERING。
            *
            * 此时不要重新启动 VAD，
            * 等 stopGeneration() 最终触发 answerFinished。
            */
            setState(
                State::ANSWERING
            );


            /*
            * 直接停止当前 RKLLM 回答。
            *
            * 注意：
            * “停止回答”不是新的聊天问题，
            * 所以绝对不能再调用 assistant_->chat()。
            */
            assistant_
                ->stopGeneration();


            return;
        }


        /*
         * AI已经在识别期间正常结束。
         */
        assistantFinishedDuringInterrupt_ =
            false;


        finishAssistantTurn();

        return;
    }


    /*
     * ========================================================
     * 正常用户命令
     * ========================================================
     */
    if (
        state_
        !=
        State::RECOGNIZING
    )
    {
        return;
    }

    const QString question =
        normalizeCommand(
            text
        );


    qDebug()
        << "====================================";

    qDebug()
        << "[VoiceController]"
        << "ASR RESULT:"
        << question;

    qDebug()
        << "====================================";


    /*
     * ========================================================
     * ASR没有识别到有效内容
     * ========================================================
     */
    if (question.isEmpty())
    {
        qWarning()
            << "[VoiceController]"
            << "empty ASR result";


        resumeCurrentWait();

        return;
    }


    /*
     * ========================================================
     * 用户明确要求退出语音助手
     * ========================================================
     *
     * 这个命令属于 VoiceController，
     * 不送 VehicleAssistant。
     *
     * 因此：
     *
     * 不进入RKLLM
     * 不进入ConversationManager
     */
    if (
        isExitCommand(
            question
        )
    )
    {
        exitVoiceSession(
            QStringLiteral(
                "user exit command"
            )
        );

        return;
    }


    /*
     * 理论上 ANSWERING 阶段根本不会开 VAD。
     *
     * 这里再做防御检查。
     */
    if (
        assistant_
        &&
        assistant_->isGenerating()
    )
    {
        qWarning()
            << "[VoiceController]"
            << "assistant is still generating";


        resumeCurrentWait();

        return;
    }


    /*
     * ========================================================
     * 第一个真正有效的语音问题
     * ========================================================
     *
     * 注意：
     *
     * 不是检测到“你好小车”以后立即创建新对话。
     *
     * 而是到这里：
     *
     * ASR真正识别出了有效用户问题
     *
     * 才创建。
     *
     * 这样误唤醒不会制造空历史对话。
     */
    if (!sessionConversationStarted_)
    {
        const QString oldConversationId =
            assistant_
                ->currentConversationId();


        assistant_
            ->newConversation();


        /*
         * 如果 newConversation() 失败，
         * 当前 conversation ID 不会改变。
         */
        if (
            assistant_
                ->currentConversationId()
            ==
            oldConversationId
        )
        {
            qWarning()
                << "[VoiceController]"
                << "failed to create"
                << "voice conversation";


            exitVoiceSession(
                QStringLiteral(
                    "new conversation failed"
                )
            );

            return;
        }


        sessionConversationStarted_ =
            true;
    }


    /*
     * ========================================================
     * 进入 Assistant 处理阶段
     * ========================================================
     *
     * 从现在开始：
     *
     * 不启动任何Timer。
     *
     * 因此：
     *
     * 本地指令耗时
     * WeatherService网络耗时
     * RAG耗时
     * RKLLM推理耗时
     * RKLLM回答耗时
     *
     * 都不会算进30秒。
     */
    setState(
        State::ANSWERING
    );


    /*
    * 把正常问题交给 VehicleAssistant。
    */
    assistant_
        ->chat(
            question
        );


    /*
    * ========================================================
    * AI回答期间继续开启VAD
    *
    * 这里只用于监听：
    *
    * “停止回答”
    *
    * 不开启5秒/30秒计时。
    * ========================================================
    */
    if (
        state_
        ==
        State::ANSWERING
    )
    {
        startAnswerInterruptListening();
    }
}


// ============================================================
// Assistant回答结束
// ============================================================

void VoiceController::onAssistantAnswerFinished()
{
    /*
     * ========================================================
     * 正常AI回答结束
     * ========================================================
     */
    if (
        state_
        ==
        State::ANSWERING
    )
    {
        finishAssistantTurn();

        return;
    }


    /*
     * ========================================================
     * 用户正在说打断命令时，
     * AI刚好自己回答完了
     * ========================================================
     *
     * 不能马上Reset VAD，
     * 否则用户说到一半会被截断。
     *
     * 等当前这句话ASR结束后再处理。
     */
    if (
        state_
        ==
        State::ANSWER_INTERRUPT_LISTENING
        ||
        state_
        ==
        State::ANSWER_INTERRUPT_RECOGNIZING
    )
    {
        assistantFinishedDuringInterrupt_ =
            true;


        qDebug()
            << "[VoiceController]"
            << "assistant finished during"
            << "interrupt recognition";


        return;
    }
}

void VoiceController::finishAssistantTurn()
{
    /*
     * 回答阶段用于监听“停止回答”的VAD
     * 必须先关闭。
     */
    if (vadService_)
    {
        vadService_
            ->stopListening();
    }


    waitTimer_.stop();


    assistantFinishedDuringInterrupt_ =
        false;


    /*
     * 至少完成过一次正式交流。
     */
    conversationEstablished_ =
        true;


    qDebug()
        << "[VoiceController]"
        << "assistant turn finished";


    /*
     * ========================================================
     * 回答结束以后才进入30秒
     * ========================================================
     */
    startFollowupWait();
}
// ============================================================
// VehicleAssistant拒绝请求
// ============================================================

void VoiceController::onChatRejected(
    const QString &reason
)
{
    /*
     * 键盘请求被拒绝也可能产生这个信号。
     *
     * 只处理当前语音请求。
     */
    if (
        state_
        !=
        State::ANSWERING
    )
    {
        return;
    }


    qWarning()
        << "[VoiceController]"
        << "chat rejected:"
        << reason;


    /*
     * 没有完成本轮回答，
     * 回到当前Session应该使用的等待阶段。
     */
    resumeCurrentWait();
}


// ============================================================
// ASR错误
// ============================================================

void VoiceController::onAsrError(
    const QString &error
)
{

     /*
     * AI回答期间，
     * 打断命令ASR失败。
     */
    if (
        state_
        ==
        State::ANSWER_INTERRUPT_RECOGNIZING
    )
    {
        qWarning()
            << "[VoiceController]"
            << "interrupt ASR error:"
            << error;


        if (
            assistant_
            &&
            assistant_->isGenerating()
        )
        {
            setState(
                State::ANSWERING
            );

            startAnswerInterruptListening();
        }
        else
        {
            assistantFinishedDuringInterrupt_ =
                false;

            finishAssistantTurn();
        }


        return;
    }
    /*
     * ASRService 初始化时也可能发 errorOccurred。
     *
     * 只有当前真的正在识别时才处理。
     */
    if (
        state_
        !=
        State::RECOGNIZING
    )
    {
        return;
    }


    qWarning()
        << "[VoiceController]"
        << "ASR error:"
        << error;


    /*
     * 一次ASR错误没必要结束整个语音Session。
     *
     * 重新等用户说一次。
     */
    resumeCurrentWait();
}


// ============================================================
// 等待超时
// ============================================================

void VoiceController::onWaitTimeout()
{
    /*
     * ========================================================
     * 5秒：
     *
     * 刚刚唤醒，
     * 用户一句话没说。
     *
     * 判定误唤醒。
     * ========================================================
     */
    if (
        state_
        ==
        State::WAIT_FIRST_COMMAND
    )
    {
        qDebug()
            << "[VoiceController]"
            << "first command timeout:"
            << "false wake";


        exitVoiceSession(
            QStringLiteral(
                "false wake timeout"
            )
        );

        return;
    }


    /*
     * ========================================================
     * 30秒：
     *
     * 已经交流过，
     * 用户没有继续讲话。
     *
     * 正常结束此次语音Session。
     * ========================================================
     */
    if (
        state_
        ==
        State::WAIT_FOLLOWUP
    )
    {
        qDebug()
            << "[VoiceController]"
            << "follow-up timeout:"
            << "session finished";


        exitVoiceSession(
            QStringLiteral(
                "follow-up timeout"
            )
        );

        return;
    }
}


// ============================================================
// 退出整个语音Session
// ============================================================

void VoiceController::exitVoiceSession(
    const QString &reason
)
{
    /*
     * 停止任何等待Timer。
     */
    waitTimer_.stop();


    /*
     * 停止VAD。
     *
     * 如果VAD已经自动停止，
     * stopListening()自己会直接返回。
     */
    if (vadService_)
    {
        vadService_
            ->stopListening();
    }


    /*
     * 清理Session状态。
     */
    conversationEstablished_ =
        false;

    sessionConversationStarted_ =
        false;


    setState(
        State::WAIT_WAKE
    );


    /*
     * ========================================================
     * 重新等待唤醒词：
     *
     * “你好小车”
     * ========================================================
     */
    if (wakeWordService_)
    {
        wakeWordService_
            ->setEnabled(
                true
            );
    }


    qDebug()
        << "[VoiceController]"
        << "voice session exited:"
        << reason;
}


// ============================================================
// 状态修改
// ============================================================

void VoiceController::setState(
    State state
)
{
    if (state_ == state)
    {
        return;
    }


    qDebug()
        << "[VoiceController]"
        << stateName(
            state_
        )
        << "->"
        << stateName(
            state
        );


    state_ =
        state;
}


// ============================================================
// 状态名称
// ============================================================

const char *VoiceController::stateName(
    State state
) const
{
    switch (state)
    {
        case State::WAIT_WAKE:
            return "WAIT_WAKE";


        case State::WAIT_FIRST_COMMAND:
            return "WAIT_FIRST_COMMAND";


        case State::LISTENING:
            return "LISTENING";


        case State::RECOGNIZING:
            return "RECOGNIZING";


        case State::ANSWERING:
            return "ANSWERING";


        case State::WAIT_FOLLOWUP:
            return "WAIT_FOLLOWUP";
        
        case State::ANSWER_INTERRUPT_LISTENING:
            return "ANSWER_INTERRUPT_LISTENING";


        case State::ANSWER_INTERRUPT_RECOGNIZING:
            return "ANSWER_INTERRUPT_RECOGNIZING";
    }


    return "UNKNOWN";
}

bool VoiceController::isStopAnswerCommand(
    const QString &text
) const
{
    const QString cmd =
        normalizeCommand(
            text
        );


    /*
     * 使用整句匹配。
     *
     * 避免普通问题里出现“停止”
     * 就误杀AI回答。
     */
    return
        cmd == QStringLiteral(
            "停止回答"
        )
        ||
        cmd == QStringLiteral(
            "停止回答问题"
        )
        ||
        cmd == QStringLiteral(
            "别回答了"
        )
        ||
        cmd == QStringLiteral(
            "不要回答了"
        )
        ||
        cmd == QStringLiteral(
            "停止"
        );
}

void VoiceController::startAnswerInterruptListening()
{
    /*
     * ========================================================
     * AI回答期间监听“停止回答”
     * ========================================================
     *
     * 这里没有：
     *
     * 5秒Timer
     * 30秒Timer
     *
     * AI回答多久都不影响。
     */

    waitTimer_.stop();


    if (vadService_)
    {
        vadService_
            ->startListening();
    }


    qDebug()
        << "[VoiceController]"
        << "listening for stop-answer command";
}