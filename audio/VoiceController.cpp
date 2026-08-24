#include "VoiceController.h"

#include "WakeWordService.h"
#include "VADService.h"
#include "ASRService.h"
#include "VoiceCommandNormalizer.h"

#include "../assistant/VehicleAssistant.h"
#include "TTSService.h"
#include <QDebug>
#include <QRegularExpression>
#include <QVector>
#include <functional>

namespace
{
struct TextReplacement
{
    int start = 0;
    int length = 0;
    QString text;
};

void replaceRegexMatches(
    QString &text,
    const QRegularExpression &expression,
    const std::function<QString(const QRegularExpressionMatch &)> &makeText)
{
    QVector<TextReplacement> replacements;
    QRegularExpressionMatchIterator iterator =
        expression.globalMatch(text);

    while (iterator.hasNext())
    {
        const QRegularExpressionMatch match =
            iterator.next();
        replacements.push_back(
            {
                match.capturedStart(0),
                match.capturedLength(0),
                makeText(match)
            });
    }

    for (int i = replacements.size() - 1;
         i >= 0;
         --i)
    {
        const TextReplacement &replacement =
            replacements.at(i);
        text.replace(
            replacement.start,
            replacement.length,
            replacement.text);
    }
}

QString speakDigitsIndividually(const QString &digits)
{
    static const QString names[] =
    {
        QStringLiteral("零"), QStringLiteral("一"),
        QStringLiteral("二"), QStringLiteral("三"),
        QStringLiteral("四"), QStringLiteral("五"),
        QStringLiteral("六"), QStringLiteral("七"),
        QStringLiteral("八"), QStringLiteral("九")
    };

    QString result;
    result.reserve(digits.size());

    for (const QChar ch : digits)
    {
        if (ch.isDigit())
            result += names[ch.digitValue()];
    }

    return result;
}

QString speakSmallInteger(const QString &digits)
{
    if (digits.isEmpty())
        return QString();

    // Identifiers, telephone-like values and large numbers are safer when
    // read digit by digit. Values up to four digits use normal cardinal form.
    if (digits.size() > 4 ||
        (digits.size() > 1 && digits.startsWith(QChar('0'))))
    {
        return speakDigitsIndividually(digits);
    }

    static const QString names[] =
    {
        QStringLiteral("零"), QStringLiteral("一"),
        QStringLiteral("二"), QStringLiteral("三"),
        QStringLiteral("四"), QStringLiteral("五"),
        QStringLiteral("六"), QStringLiteral("七"),
        QStringLiteral("八"), QStringLiteral("九")
    };
    static const QString units[] =
    {
        QString(), QStringLiteral("十"),
        QStringLiteral("百"), QStringLiteral("千")
    };

    QString result;
    bool pendingZero = false;

    for (int i = 0; i < digits.size(); ++i)
    {
        const int digit = digits.at(i).digitValue();
        const int position = digits.size() - i - 1;

        if (digit == 0)
        {
            if (!result.isEmpty())
                pendingZero = true;
            continue;
        }

        if (pendingZero)
        {
            result += QStringLiteral("零");
            pendingZero = false;
        }

        // 10..19 are normally spoken as 十、十一... rather than 一十...
        if (!(digit == 1 && position == 1 && result.isEmpty()))
            result += names[digit];

        result += units[position];
    }

    return result.isEmpty()
        ? QStringLiteral("零")
        : result;
}

QString speakNumber(const QString &number)
{
    const int point = number.indexOf(QChar('.'));

    if (point < 0)
        return speakSmallInteger(number);

    return speakSmallInteger(number.left(point)) +
        QStringLiteral("点") +
        speakDigitsIndividually(number.mid(point + 1));
}

QString spellLatinLetters(const QString &letters)
{
    static const QString names[] =
    {
        QStringLiteral("诶"),       // A
        QStringLiteral("比"),       // B
        QStringLiteral("西"),       // C
        QStringLiteral("迪"),       // D
        QStringLiteral("伊"),       // E
        QStringLiteral("艾弗"),     // F
        QStringLiteral("吉"),       // G
        QStringLiteral("艾尺"),     // H
        QStringLiteral("艾"),       // I
        QStringLiteral("杰"),       // J
        QStringLiteral("开"),       // K
        QStringLiteral("艾勒"),     // L
        QStringLiteral("艾姆"),     // M
        QStringLiteral("恩"),       // N
        QStringLiteral("欧"),       // O
        QStringLiteral("披"),       // P
        QStringLiteral("丘"),       // Q
        QStringLiteral("阿尔"),     // R
        QStringLiteral("艾丝"),     // S
        QStringLiteral("提"),       // T
        QStringLiteral("优"),       // U
        QStringLiteral("维"),       // V
        QStringLiteral("达不溜"),   // W
        QStringLiteral("艾克斯"),   // X
        QStringLiteral("歪"),       // Y
        QStringLiteral("贼德")      // Z
    };

    QString result;

    for (const QChar ch : letters.toUpper())
    {
        const ushort code = ch.unicode();
        if (code >= 'A' && code <= 'Z')
            result += names[code - 'A'];
    }

    return result;
}

void replaceAsciiTerm(
    QString &text,
    const QString &term,
    const QString &spoken)
{
    text.replace(
        QRegularExpression(
            QStringLiteral("(?<![A-Za-z])") +
                QRegularExpression::escape(term) +
                QStringLiteral("(?![A-Za-z])"),
            QRegularExpression::CaseInsensitiveOption),
        spoken);
}

QString normalizeUnsupportedTtsText(QString text)
{
    // Do not spell long URLs and e-mail addresses character by character.
    text.replace(
        QRegularExpression(
            QStringLiteral("(?:https?://|www\\.)\\S+"),
            QRegularExpression::CaseInsensitiveOption),
        QStringLiteral("链接"));
    text.replace(
        QRegularExpression(
            QStringLiteral("[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}")),
        QStringLiteral("电子邮箱"));

    // Frequently occurring vehicle/computing abbreviations are clearer when
    // expanded semantically. Unknown Latin words are still spelled below.
    replaceAsciiTerm(text, QStringLiteral("SUV"), QStringLiteral("运动型多用途汽车"));
    replaceAsciiTerm(text, QStringLiteral("MPV"), QStringLiteral("多用途汽车"));
    replaceAsciiTerm(text, QStringLiteral("PHEV"), QStringLiteral("插电式混合动力汽车"));
    replaceAsciiTerm(text, QStringLiteral("HEV"), QStringLiteral("混合动力汽车"));
    replaceAsciiTerm(text, QStringLiteral("EV"), QStringLiteral("电动汽车"));
    replaceAsciiTerm(text, QStringLiteral("ADAS"), QStringLiteral("高级驾驶辅助系统"));
    replaceAsciiTerm(text, QStringLiteral("ABS"), QStringLiteral("防抱死制动系统"));
    replaceAsciiTerm(text, QStringLiteral("ESP"), QStringLiteral("车身电子稳定系统"));
    replaceAsciiTerm(text, QStringLiteral("NPU"), QStringLiteral("神经网络处理器"));
    replaceAsciiTerm(text, QStringLiteral("CPU"), QStringLiteral("中央处理器"));
    replaceAsciiTerm(text, QStringLiteral("GPU"), QStringLiteral("图形处理器"));
    replaceAsciiTerm(text, QStringLiteral("LLM"), QStringLiteral("大语言模型"));
    replaceAsciiTerm(text, QStringLiteral("AI"), QStringLiteral("人工智能"));

    // Units must be replaced before generic letters and numbers.
    text.replace(
        QRegularExpression(
            QStringLiteral("[lL]\\s*/\\s*100\\s*[kK][mM](?![A-Za-z])")),
        QStringLiteral("升每百公里"));
    text.replace(
        QRegularExpression(
            QStringLiteral("[kK][mM]\\s*/\\s*[hH](?![A-Za-z])")),
        QStringLiteral("公里每小时"));
    text.replace(
        QRegularExpression(
            QStringLiteral("[kK][wW][hH](?![A-Za-z])")),
        QStringLiteral("千瓦时"));
    text.replace(
        QRegularExpression(
            QStringLiteral("[kK][wW](?![A-Za-z])")),
        QStringLiteral("千瓦"));
    text.replace(
        QRegularExpression(
            QStringLiteral("[rR][pP][mM](?![A-Za-z])")),
        QStringLiteral("转每分钟"));
    text.replace(
        QRegularExpression(
            QStringLiteral("(?:°\\s*[cC]|℃)")),
        QStringLiteral("摄氏度"));
    text.replace(
        QRegularExpression(
            QStringLiteral("(?<=\\d)\\s*[lL](?![A-Za-z])")),
        QStringLiteral("升"));
    text.replace(
        QRegularExpression(
            QStringLiteral("[kK][mM](?![A-Za-z])")),
        QStringLiteral("公里"));

    // 8.5% -> 百分之八点五. Apply before the generic number pass.
    replaceRegexMatches(
        text,
        QRegularExpression(QStringLiteral("(\\d+(?:\\.\\d+)?)\\s*%")),
        [](const QRegularExpressionMatch &match)
        {
            return QStringLiteral("百分之") +
                speakNumber(match.captured(1));
        });

    // A four-digit year/model year is naturally read digit by digit.
    replaceRegexMatches(
        text,
        QRegularExpression(QStringLiteral("(\\d{4})(?=[年款])")),
        [](const QRegularExpressionMatch &match)
        {
            return speakDigitsIndividually(match.captured(1));
        });

    // Numeric ranges should retain their relationship after digit conversion.
    text.replace(
        QRegularExpression(QStringLiteral("(?<=\\d)\\s*[-~～—]\\s*(?=\\d)")),
        QStringLiteral("到"));

    replaceRegexMatches(
        text,
        QRegularExpression(QStringLiteral("\\d+\\.\\d+|\\d+")),
        [](const QRegularExpressionMatch &match)
        {
            return speakNumber(match.captured(0));
        });

    replaceRegexMatches(
        text,
        QRegularExpression(QStringLiteral("[A-Za-z]+")),
        [](const QRegularExpressionMatch &match)
        {
            return spellLatinLetters(match.captured(0));
        });

    // Remaining slashes usually mean alternatives, e.g. 国产/进口.
    text.replace(QChar('/'), QStringLiteral("或"));
    text.replace(QStringLiteral("￥"), QStringLiteral("人民币"));
    text.replace(QStringLiteral("¥"), QStringLiteral("人民币"));

    return text;
}
} // namespace

// ============================================================
// Constructor
// ============================================================

VoiceController::VoiceController(
    WakeWordService *wakeWordService,
    VADService *vadService,
    ASRService *asrService,
    VehicleAssistant *assistant,
    TTSService *ttsService,
    QObject *parent
)
    :
    QObject(parent),
    wakeWordService_(wakeWordService),
    vadService_(vadService),
    asrService_(asrService),
    assistant_(assistant),
    ttsService_(ttsService)
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
    * VehicleAssistant流式分句
    *
    * RKLLM边生成，
    * 这里边把句子交给TTS。
    * ========================================================
    */
    connect(
        assistant_,
        &VehicleAssistant::answerSpeechChunkReady,

        this,
        &VoiceController::onAssistantSpeechChunkReady,

        Qt::QueuedConnection
    );
    if (ttsService_)
    {
        /*
        * TTS真正开始出声。
        */
        connect(
            ttsService_,
            &TTSService::speechStarted,

            this,
            &VoiceController::onTtsSpeechStarted,

            Qt::QueuedConnection
        );


        /*
        * 注意：
        *
        * speechFinished不是一句结束，
        * 是整轮TTS全部播放结束。
        */
        connect(
            ttsService_,
            &TTSService::speechFinished,

            this,
            &VoiceController::onTtsSpeechFinished,

            Qt::QueuedConnection
        );


        /*
        * 用户通过“小车停止回答”主动打断播放。
        */
        connect(
            ttsService_,
            &TTSService::speechStopped,

            this,
            &VoiceController::onTtsSpeechStopped,

            Qt::QueuedConnection
        );
    }
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
     * ========================================================
     * AI回答期间的专用KWS打断。
     *
     * 不在TTS播放期间开放普通VAD/ASR，避免把扬声器
     * 回灌误当成用户语音。KWS只有完整命中“小车停止回答”
     * 才会进入这个分支。
     * ========================================================
     */
    if (
        state_ == State::ANSWERING
        &&
        isStopAnswerCommand(keyword)
    )
    {
        qDebug()
            << "[VoiceController]"
            << "STOP ANSWER KWS detected:"
            << keyword;


        waitTimer_.stop();


        /*
         * 先关闭KWS，防止同一句语音重复触发。
         */
        if (wakeWordService_)
        {
            wakeWordService_->setEnabled(false);
        }


        /*
         * 立即清空尚未播放的PCM和尚未合成的文本。
         */
        if (ttsService_)
        {
            ttsService_->stop();
        }


        /*
         * RKLLM真正退出后会发出answerFinished()。
         */
        if (
            assistant_
            &&
            assistant_->isGenerating()
        )
        {
            assistant_->stopGeneration();

            return;
        }


        /*
         * 文本已经生成完，但TTS可能还在清理队列。
         * 等speechStopped()后再恢复后续对话。
         */
        if (
            ttsService_
            &&
            ttsService_->isBusy()
        )
        {
            return;
        }


        ttsSpeaking_ = false;
        finishAssistantTurn();

        return;
    }


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
    /*
     * 已经进入语音Session，此时只使用VAD等待命令。
     */
    if (wakeWordService_)
    {
        wakeWordService_->setEnabled(false);
    }


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
    /*
     * 离开回答阶段后关闭专用停止KWS，恢复普通VAD。
     */
    if (wakeWordService_)
    {
        wakeWordService_->setEnabled(false);
    }


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
    * TTS正在播放时，
    * 所有麦克风VAD事件都忽略。
    */
    if (ttsSpeaking_)
    {
        qDebug()
            << "[VoiceController]"
            << "speechStarted ignored:"
            << "TTS is speaking";

        return;
    }
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
    * TTS正在播放时，
    * 不处理扬声器回灌形成的语音段。
    */
    if (ttsSpeaking_)
    {
        qDebug()
            << "[VoiceController]"
            << "speech segment ignored:"
            << "TTS is speaking";

        return;
    }
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

            // Drop queued PCM immediately. An in-flight rknn_run() cannot be
            // interrupted safely, but its result is discarded when it returns.
            if (ttsService_)
                ttsService_->stop();


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
                << "non-stop interrupt ignored," 
                << "dedicated KWS remains active";


            /*
             * 非停止指令不应该终止回答。回到ANSWERING，
             * 由专用KWS继续监听“小车停止回答”。
             */
            setState(
                State::ANSWERING
            );


            if (wakeWordService_)
            {
                wakeWordService_->setEnabled(true);
            }


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
     * 回答阶段只开放专用KWS。“你好小车”如果被检出，
     * onWakeWordDetected()会因为不是停止口令而忽略。
     */
    if (wakeWordService_)
    {
        wakeWordService_->setEnabled(true);
    }


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
    // if (
    //     state_
    //     ==
    //     State::ANSWERING
    //     &&
    //     assistant_
    //     &&
    //     assistant_->isGenerating()
    // )
    // {
    //     startAnswerInterruptListening();
    // }
}
void VoiceController::onTtsSpeechStarted()
{
    /*
     * 不在语音 Session 中时，
     * 不需要处理。
     */
    if (
        state_
        ==
        State::WAIT_WAKE
    )
    {
        return;
    }


    /*
     * TTS 已经开始真正播放声音。
     *
     * 不调用 vadService_->stopListening()，
     * 避免和音频采集线程里的 VAD 处理产生并发冲突。
     *
     * 后续通过 ttsSpeaking_，
     * 在 onSpeechStarted() 和
     * onSpeechSegmentReady() 中直接忽略
     * 扬声器回灌产生的 VAD 事件。
     */
    ttsSpeaking_ =
        true;


    waitTimer_.stop();


    qDebug()
        << "[VoiceController]"
        << "TTS speaking,"
        << "VAD events will be ignored";
}

// ============================================================
// Assistant回答结束
// ============================================================
void VoiceController::onAssistantSpeechChunkReady(
    const QString &text,
    int pauseAfterMs
)
{
    /*
     * 只朗读语音Session中的回答。
     *
     * 如果用户用键盘问AI，
     * 此时state通常是WAIT_WAKE，
     * 不自动朗读。
     */
    if (
        state_
        !=
        State::ANSWERING
    )
    {
        return;
    }


    if (
        !ttsService_
        ||
        !ttsService_->isInitialized()
    )
    {
        return;
    }


    QString speech =
        text.trimmed();


    /*
    * TTS专用文字规范化。
    *
    * 只改变朗读内容，
    * 不影响聊天窗口原始回答。
    */
    speech.replace(
        QStringLiteral("Qwen3.5"),
        QStringLiteral("千问三点五"),
        Qt::CaseInsensitive
    );

    speech.replace(
        QStringLiteral("Qwen"),
        QStringLiteral("千问"),
        Qt::CaseInsensitive
    );

    speech.replace(
        QStringLiteral("RKLLM"),
        QStringLiteral("瑞芯微大语言模型"),
        Qt::CaseInsensitive
    );

    speech.replace(
        QStringLiteral("RKNN"),
        QStringLiteral("瑞芯微神经网络模型"),
        Qt::CaseInsensitive
    );

    // LLM answers often contain Markdown. Keep the visible link label, remove
    // formatting marks, and drop emoji that are not present in the Matcha
    // pronunciation lexicon. This normalization affects speech only; the QML
    // conversation still displays the original answer.
    speech.replace(
        QRegularExpression(
            QStringLiteral("\\[([^\\]]+)\\]\\([^\\)]+\\)")),
        QStringLiteral("\\1"));
    speech.remove(QStringLiteral("**"));
    speech.remove(QStringLiteral("__"));
    speech.remove(QChar('`'));
    speech.remove(QChar('#'));
    speech.replace(
        QRegularExpression(
            QStringLiteral("^\\s*[-+*]\\s+")),
        QString());
    speech.remove(
        QRegularExpression(
            QStringLiteral("[\\x{1F000}-\\x{1FAFF}\\x{2600}-\\x{27BF}]")));

    // Turn Markdown numbered-list prefixes into text the Chinese Matcha
    // frontend can pronounce.  Without this conversion, "1." becomes an
    // isolated punctuation chunk; its token conversion fails before the
    // actual list-item text is reached.
    const QRegularExpression numberedListPrefix(
        QStringLiteral("^\\s*(\\d{1,2})(?:[、．]|\\.(?!\\d))\\s*"));
    const QRegularExpressionMatch numberedMatch =
        numberedListPrefix.match(speech);

    if (numberedMatch.hasMatch())
    {
        const int number =
            numberedMatch.captured(1).toInt();

        if (number >= 1 && number <= 99)
        {
            static const QString chineseDigits[] =
            {
                QStringLiteral("零"),
                QStringLiteral("一"),
                QStringLiteral("二"),
                QStringLiteral("三"),
                QStringLiteral("四"),
                QStringLiteral("五"),
                QStringLiteral("六"),
                QStringLiteral("七"),
                QStringLiteral("八"),
                QStringLiteral("九")
            };

            QString spokenNumber;

            if (number < 10)
            {
                spokenNumber = chineseDigits[number];
            }
            else
            {
                const int tens = number / 10;
                const int ones = number % 10;

                if (tens > 1)
                    spokenNumber += chineseDigits[tens];

                spokenNumber += QStringLiteral("十");

                if (ones > 0)
                    spokenNumber += chineseDigits[ones];
            }

            speech.replace(
                0,
                numberedMatch.capturedLength(0),
                QStringLiteral("第") +
                    spokenNumber +
                    QStringLiteral("，"));
        }
    }

    // Common vehicle abbreviations are not present in the Chinese-only
    // pronunciation lexicon.  Expand them before sending text to Matcha.
    speech.replace(
        QRegularExpression(
            QStringLiteral("\\bSUV\\b"),
            QRegularExpression::CaseInsensitiveOption),
        QStringLiteral("运动型多用途汽车"));
    speech.replace(
        QRegularExpression(
            QStringLiteral("\\bMPV\\b"),
            QRegularExpression::CaseInsensitiveOption),
        QStringLiteral("多用途汽车"));
    speech.replace(
        QRegularExpression(
            QStringLiteral("\\bEV\\b"),
            QRegularExpression::CaseInsensitiveOption),
        QStringLiteral("电动汽车"));

    speech = normalizeUnsupportedTtsText(speech);

    speech = speech.simplified();

    if (speech.isEmpty())
    {
        return;
    }


    qDebug()
        << "[VoiceController]"
        << "send speech chunk to TTS:"
        << speech;


    /*
     * TTSService内部现在已经有队列，
     * 所以这里直接不断speak()即可。
     */
    ttsService_
        ->speak(
            speech,
            pauseAfterMs
        );
}
void VoiceController::onTtsSpeechFinished()
{
    qDebug()
        << "[VoiceController]"
        << "TTS turn finished,"
        << "waiting audio tail";


    /*
     * 扬声器刚刚停止时，
     * 麦克风/VAD里可能还有最后一点残留声音。
     *
     * 暂时继续保持：
     *
     * ttsSpeaking_ = true
     *
     * 让这些残留VAD事件继续被丢弃。
     */
    QTimer::singleShot(
        200,
        this,

        [this]()
        {
            ttsSpeaking_ =
                false;


            if (
                state_
                ==
                State::ANSWERING
            )
            {
                /*
                 * stop()路径会同时发出speechStopped和
                 * speechFinished。RKLLM尚未真正退出时，不能提前
                 * 进入后续对话。
                 */
                if (
                    assistant_
                    &&
                    (
                        assistant_->isGenerating()
                        ||
                        assistant_->isStopping()
                    )
                )
                {
                    qDebug()
                        << "[VoiceController]"
                        << "TTS finished after stop,"
                        << "waiting for assistant";

                    return;
                }


                finishAssistantTurn();

                return;
            }


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

                return;
            }
        }
    );
}


void VoiceController::onTtsSpeechStopped()
{
    qDebug()
        << "[VoiceController]"
        << "TTS stopped, waiting audio tail";


    /*
     * 停止播放后仍保留200ms回声尾音保护。
     */
    QTimer::singleShot(
        200,
        this,

        [this]()
        {
            ttsSpeaking_ = false;


            if (
                state_ != State::ANSWERING
            )
            {
                return;
            }


            /*
             * TTS可能比RKLLM先停下。如果RKLLM还在退出，
             * 等VehicleAssistant::answerFinished()完成收尾。
             */
            if (
                assistant_
                &&
                (
                    assistant_->isGenerating()
                    ||
                    assistant_->isStopping()
                )
            )
            {
                qDebug()
                    << "[VoiceController]"
                    << "TTS stopped, waiting for assistant stop";

                return;
            }


            finishAssistantTurn();
        }
    );
}
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
        /*
        * RKLLM文字生成结束了，
        * 但TTS可能还有两三句话没有播放。
        *
        * 所以现在不能：
        *
        * finishAssistantTurn()
        *
        * 而是先告诉TTS：
        *
        * “不会再有新的句子了。”
        */
        if (
            ttsService_
            &&
            ttsService_->isInitialized()
        )
        {
            /*
            * 回答文本已经全部送完。
            */
            ttsService_
                ->finishInput();


            /*
            * 如果TTS仍然忙，
            * 保持ANSWERING。
            *
            * 等speechFinished()。
            */
            if (
                ttsService_->isBusy()
            )
            {
                qDebug()
                    << "[VoiceController]"
                    << "assistant text finished,"
                    << "waiting for TTS";

                return;
            }
        }


        /*
        * 没有TTS内容。
        *
        * 例如某些只执行页面动作、
        * 不产生语音回答的命令。
        */
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

        if (
            ttsService_
            &&
            ttsService_->isInitialized()
        )
        {
            ttsService_
                ->finishInput();
        }
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
     * 回答阶段现在不再启动VAD，
     * 因此这里不要做 stopListening()。
     *
     * 避免：
     *
     * stopListening()
     * ↓
     * 立即startListening()
     *
     * 与音频采集线程产生竞争。
     */

    waitTimer_.stop();


    /*
     * 回答期的专用停止KWS到此结束。
     */
    if (wakeWordService_)
    {
        wakeWordService_->setEnabled(false);
    }


    assistantFinishedDuringInterrupt_ =
        false;


    conversationEstablished_ =
        true;


    qDebug()
        << "[VoiceController]"
        << "assistant turn finished";


    /*
     * 在这里统一重新启动VAD，
     * 开始30秒连续会话。
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
            "小车停止回答"
        )
        ||
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
