#include "VehicleAssistant.h"

#include <QDebug>
#include <QMetaObject>
#include <QUuid>


VehicleAssistant::VehicleAssistant(
    QObject *parent
)
    :
    QObject(parent)
{
    /*
     * APP 启动时自动创建第一条会话。
     */
    currentConversationId_ =
        QUuid::createUuid()
            .toString(
                QUuid::WithoutBraces
            );
    
    /*
     * 同时通知 ConversationManager
     * 创建这个会话。
     */
    conversationManager_
        .startNewConversation(
            currentConversationId_
        );


    qDebug()
        << "[VehicleAssistant]"
        << "initial conversation:"
        << currentConversationId_;
}

QString VehicleAssistant::currentConversationId() const
{
    return currentConversationId_;
}

QVariantList VehicleAssistant::conversationHistory() const
{
    return conversationManager_
        .conversationList();
}


QVariantList VehicleAssistant::historyMessages(
    const QString &conversationId
) const
{
    return conversationManager_
        .conversationMessages(
            conversationId
        );
}

bool VehicleAssistant::init(
    const std::string &model,
    const std::string &knowledge
)
{
    /*
     * 初始化 RKLLM。
     */
    if (!llm_.init(model))
    {
        qWarning()
            << "[VehicleAssistant]"
            << "LLM init failed";

        return false;
    }


    /*
     * 初始化 RAG。
     */
    if (!rag_.init(knowledge))
    {
        qWarning()
            << "[VehicleAssistant]"
            << "RAG init failed";

        return false;
    }


    return true;
}


/*
 * ============================================================
 * 状态查询
 * ============================================================
 */

bool VehicleAssistant::isGenerating() const
{
    return llmRunning_.load();
}


bool VehicleAssistant::isStopping() const
{
    return stopRequested_.load();
}


/*
 * ============================================================
 * 状态修改
 * ============================================================
 */

void VehicleAssistant::setGenerating(
    bool generating
)
{
    const bool old =
        llmRunning_.exchange(
            generating
        );


    if (old == generating)
        return;


    qDebug()
        << "[VehicleAssistant]"
        << "generating ="
        << generating;


    emit generatingChanged();
}


void VehicleAssistant::setStopping(
    bool stopping
)
{
    const bool old =
        stopRequested_.exchange(
            stopping
        );


    if (old == stopping)
        return;


    qDebug()
        << "[VehicleAssistant]"
        << "stopping ="
        << stopping;


    emit stoppingChanged();
}


/*
 * ============================================================
 * 回收上一次线程
 * ============================================================
 */

void VehicleAssistant::joinFinishedThread()
{
    if (llmThread_.joinable())
    {
        qDebug()
            << "[VehicleAssistant]"
            << "joining previous LLM thread";


        llmThread_.join();


        qDebug()
            << "[VehicleAssistant]"
            << "previous LLM thread joined";
    }
}


/*
 * ============================================================
 * 发送问题
 * ============================================================
 */

void VehicleAssistant::chat(
    const QString &question
)
{
    QString cmd =
        question.trimmed();


    if (cmd.isEmpty())
        return;


    /*
     * --------------------------------------------------------
     * AI 正在生成：
     * 不允许第二个问题进入 RKLLM。
     * --------------------------------------------------------
     */
    if (llmRunning_.load())
    {
        qWarning()
            << "[VehicleAssistant]"
            << "chat rejected:"
            << "LLM is still generating";


        emit chatRejected(
            QStringLiteral(
                "AI正在回答，请等待回答完成或停止当前回答。"
            )
        );


        return;
    }


    /*
     * --------------------------------------------------------
     * 回收上一条已经结束的线程。
     * --------------------------------------------------------
     */
    {
        std::lock_guard<std::mutex> lock(
            llmMutex_
        );


        joinFinishedThread();
    }

    /*
    * ========================================================
    * 保存用户消息。
    * ========================================================
    */

    conversationManager_
        .appendUserMessage(
            cmd
        );


    /*
    * 用户消息先写一次磁盘。
    *
    * 即使模型运行过程中 APP 意外关闭，
    * 至少用户问题已经保留下来。
    */
    conversationManager_
        .saveCurrent();


    /*
     * ========================================================
     * 本地命令
     * ========================================================
     */


    /*
     * 相册。
     */
    if (
        cmd.contains("打开相册")
        ||
        cmd.contains("查看相册")
        ||
        cmd.contains("打开照片")
        ||
        cmd.contains("查看照片")
    )
    {
        QMetaObject::invokeMethod(
            this,

            [this]()
            {
                emit openGalleryPage();
            },

            Qt::QueuedConnection
        );


        const QString answer =
            QStringLiteral(
                "正在打开相册"
            );


        conversationManager_
            .appendAssistantMessage(
                answer
            );


        conversationManager_
            .saveCurrent();


        emit answerToken(
            answer
        );


        emit answerFinished();


        return;
    }


    /*
     * 摄像头。
     */
    if (
        cmd.contains("打开摄像头")
        ||
        cmd.contains("开启摄像头")
        ||
        cmd.contains("打开相机")
    )
    {
        QMetaObject::invokeMethod(
            this,

            [this]()
            {
                emit openCameraPage();
            },

            Qt::QueuedConnection
        );


        const QString answer =
            QStringLiteral(
                "正在打开摄像头"
            );


        conversationManager_
            .appendAssistantMessage(
                answer
            );


        conversationManager_
            .saveCurrent();


        emit answerToken(
            answer
        );


        emit answerFinished();


        return;
    }


    /*
     * 拍照。
     */
    if (
        cmd.contains("拍照")
        ||
        cmd.contains("拍一张")
    )
    {
        const QString answer =
            QStringLiteral(
                "拍照功能暂未绑定"
            );


        conversationManager_
            .appendAssistantMessage(
                answer
            );


        conversationManager_
            .saveCurrent();


        emit answerToken(
            answer
        );


        emit answerFinished();


        return;
    }


    /*
     * ========================================================
     * RKLLM 推理
     * ========================================================
     */


    /*
    * ========================================================
    * 真正发给 RAG / RKLLM 的问题。
    *
    * 正常会话：
    *     就是当前 question
    *
    * 刚从历史恢复：
    *     历史上下文 + 当前 question
    * ========================================================
    */

    QString questionForModel =
        question;


    /*
    * 如果刚刚打开了一个历史会话，
    * 当前这一问负责重新建立模型上下文。
    */
    if (
        restoreHistoryOnNextPrompt_
        &&
        !restoredHistoryContext_.isEmpty()
    )
    {
        questionForModel =
            QStringLiteral(
                "以下内容是当前会话此前已经发生的历史对话。"
                "请把它作为上下文继续交流。"
                "不要重新总结历史，不要声称这些内容是用户刚刚说的。"
                "请直接回答最后的新问题。\n\n"
                "===== 历史对话 =====\n"
                "%1\n"
                "===== 历史对话结束 =====\n\n"
                "用户现在的新问题：\n"
                "%2"
            )
            .arg(
                restoredHistoryContext_,
                question
            );


        qDebug()
            << "[VehicleAssistant]"
            << "restoring historical context for first prompt";


        /*
        * 只恢复一次。
        *
        * 本次 prompt 会因为 RKLLM：
        *
        * keep_history = 1
        *
        * 进入新的 KV Cache。
        *
        * 后面第二问、第三问就继续使用正常历史。
        */
        restoreHistoryOnNextPrompt_ =
            false;


        restoredHistoryContext_
            .clear();
    }


    const std::string q =
        questionForModel
            .toStdString();


    /*
     * 创建新的 generation ID。
     */
    const std::uint64_t currentGeneration =
        generationId_.fetch_add(1) + 1;


    /*
     * 新任务开始时：
     *
     * generating = true
     * stopping   = false
     */
    setStopping(false);

    setGenerating(true);

    /*
    * 创建一条空 assistant 消息。
    *
    * 后面的 token 会不断追加进去。
    */
    conversationManager_
        .beginAssistantMessage();

    /*
     * 创建唯一 LLM 线程。
     */
    {
        std::lock_guard<std::mutex> lock(
            llmMutex_
        );


        llmThread_ =
            std::thread(
                [
                    this,
                    q,
                    currentGeneration
                ]()
                {
                    qDebug()
                        << "[VehicleAssistant]"
                        << "LLM worker start"
                        << "generation ="
                        << currentGeneration;


                    /*
                     * RAG 增强。
                     */
                    std::string prompt =
                        rag_.buildPrompt(q);


                    /*
                     * RKLLM 同步运行。
                     *
                     * stopGeneration()
                     * 调用 rkllm_abort() 后，
                     * 最终会让这里返回。
                     */
                    llm_.chat(
                        prompt,

                        [
                            this,
                            currentGeneration
                        ](
                            const std::string &token
                        )
                        {
                            /*
                             * 已请求停止：
                             * 不再接受任何新 token。
                             */
                            if (
                                stopRequested_.load()
                            )
                            {
                                return;
                            }


                            /*
                             * 不是当前 generation。
                             */
                            if (
                                generationId_.load()
                                != currentGeneration
                            )
                            {
                                return;
                            }


                            if (
                                !llmRunning_.load()
                            )
                            {
                                return;
                            }


                            const QString qtoken =
                                QString::fromStdString(
                                    token
                                );


                            /*
                             * 回到 Qt 主线程。
                             */
                            QMetaObject::invokeMethod(
                                this,

                                [
                                    this,
                                    qtoken,
                                    currentGeneration
                                ]()
                                {
                                    /*
                                     * queued token 到达主线程时
                                     * 再检查一次。
                                     */
                                    if (
                                        stopRequested_.load()
                                    )
                                    {
                                        return;
                                    }


                                    if (
                                        generationId_.load()
                                        != currentGeneration
                                    )
                                    {
                                        return;
                                    }


                                    if (
                                        !llmRunning_.load()
                                    )
                                    {
                                        return;
                                    }
                                    
                                    /*
                                    * 先更新 C++ 会话数据。
                                    */
                                    conversationManager_
                                        .appendAssistantToken(
                                            qtoken
                                        );

                                    emit answerToken(
                                        qtoken
                                    );
                                },

                                Qt::QueuedConnection
                            );
                        }
                    );


                    /*
                     * =================================================
                     * rkllm_run() 已经真正返回。
                     * =================================================
                     */


                    const bool wasStopped =
                        stopRequested_.load();


                    qDebug()
                        << "[VehicleAssistant]"
                        << "LLM worker finished"
                        << "generation ="
                        << currentGeneration
                        << "stopped ="
                        << wasStopped;


                    /*
                     * 最终状态切换必须回到 Qt 主线程。
                     */
                    QMetaObject::invokeMethod(
                        this,

                        [
                            this,
                            currentGeneration,
                            wasStopped
                        ]()
                        {
                            /*
                             * 旧 generation 不处理。
                             */
                            if (
                                generationId_.load()
                                != currentGeneration
                            )
                            {
                                return;
                            }


                            /*
                             * 任务现在才真正结束。
                             */
                            setGenerating(false);


                            /*
                            * 保存 AI 消息状态。
                            */
                            if (wasStopped)
                            {
                                conversationManager_
                                    .finishAssistantMessage(
                                        QStringLiteral(
                                            "interrupted"
                                        )
                                    );
                            }
                            else
                            {
                                conversationManager_
                                    .finishAssistantMessage(
                                        QStringLiteral(
                                            "complete"
                                        )
                                    );
                            }


                            /*
                            * 当前一轮完整保存到 JSON。
                            */
                            conversationManager_
                                .saveCurrent();


                            if (wasStopped)
                            {
                                emit answerStopped();
                            }


                            setStopping(false);


                            emit answerFinished();


                            qDebug()
                                << "[VehicleAssistant]"
                                << "answerFinished"
                                << "generation ="
                                << currentGeneration;
                        },

                        Qt::QueuedConnection
                    );
                }
            );
    }
}


/*
 * ============================================================
 * 停止当前回答
 * ============================================================
 */

void VehicleAssistant::stopGeneration()
{
    /*
     * 没有任务。
     */
    if (!llmRunning_.load())
    {
        qDebug()
            << "[VehicleAssistant]"
            << "stop ignored:"
            << "not generating";

        return;
    }


    /*
     * 已经请求停止了。
     *
     * 防止用户连续点击十几次。
     */
    if (stopRequested_.load())
    {
        qDebug()
            << "[VehicleAssistant]"
            << "stop ignored:"
            << "already stopping";

        return;
    }


    qDebug()
        << "[VehicleAssistant]"
        << "stopGeneration requested";


    /*
     * --------------------------------------------------------
     * 第一步：
     * 屏蔽 RKLLM 后续 token。
     * --------------------------------------------------------
     */
    setStopping(true);


    /*
     * --------------------------------------------------------
     * 第二步：
     * 请求 RKLLM abort。
     *
     * 这里绝对不能：
     *
     * setGenerating(false)
     *
     * 因为 rkllm_run() 此时可能还没有真正 return。
     * --------------------------------------------------------
     */
    llm_.stop();


    /*
     * 后续流程：
     *
     * rkllm_run()
     *     ↓
     * return
     *     ↓
     * llmThread_ 尾部
     *     ↓
     * setGenerating(false)
     *     ↓
     * answerStopped()
     *     ↓
     * setStopping(false)
     *     ↓
     * answerFinished()
     */
}


/*
 * ============================================================
 * 析构
 * ============================================================
 */

VehicleAssistant::~VehicleAssistant()
{
    qDebug()
        << "[VehicleAssistant]"
        << "destructor";


    /*
     * 防止继续输出 token。
     */
    stopRequested_.store(true);


    /*
     * 请求停止 RKLLM。
     */
    llm_.stop();


    /*
     * 必须等线程真正结束。
     */
    {
        std::lock_guard<std::mutex> lock(
            llmMutex_
        );


        joinFinishedThread();

    }


    llmRunning_.store(false);
    conversationManager_
    .saveCurrent();
}

void VehicleAssistant::newConversation()
{
    qDebug()
        << "[VehicleAssistant]"
        << "★ newConversation() entered";


    /*
     * AI 正在生成时不能切换会话。
     */
    if (llmRunning_.load())
    {
        qWarning()
            << "[VehicleAssistant]"
            << "★ newConversation rejected:"
            << "LLM still running";


        emit conversationOperationRejected(
            QStringLiteral(
                "AI正在回答，请先停止当前回答。"
            )
        );

        return;
    }


    /*
     * 回收上一轮已经完成的线程。
     */
    {
        std::lock_guard<std::mutex> lock(
            llmMutex_
        );


        qDebug()
            << "[VehicleAssistant]"
            << "★ joining old thread";


        joinFinishedThread();


        qDebug()
            << "[VehicleAssistant]"
            << "★ old thread joined";
    }


    const QString oldId =
        currentConversationId_;


    /*
     * 清理上一会话的 RKLLM KV Cache。
     */
    qDebug()
        << "[VehicleAssistant]"
        << "★ clearing RKLLM history";


    const bool resetOk =
        llm_.resetContext(true);


    qDebug()
        << "[VehicleAssistant]"
        << "★ resetContext result ="
        << resetOk;


    if (!resetOk)
    {
        qWarning()
            << "[VehicleAssistant]"
            << "★ new conversation failed:"
            << "resetContext failed";


        emit conversationOperationRejected(
            QStringLiteral(
                "创建新对话失败：无法清除模型上下文。"
            )
        );

        return;
    }


    /*
    * 新对话绝对不能继续使用
    * 上一个历史会话的恢复上下文。
    */
    restoreHistoryOnNextPrompt_ =
        false;


    restoredHistoryContext_
        .clear();
    /*
     * 使旧 generation 的 queued token 失效。
     */
    generationId_.fetch_add(1);


    stopRequested_.store(false);


    /*
    * 保存旧会话。
    */
    conversationManager_
        .saveCurrent();


    /*
    * 废弃旧 generation。
    */
    generationId_.fetch_add(1);

    stopRequested_.store(false);


    /*
    * 创建新的 conversation ID。
    */
    currentConversationId_ =
        QUuid::createUuid()
            .toString(
                QUuid::WithoutBraces
            );


    /*
    * 创建新的持久化会话。
    */
    conversationManager_
        .startNewConversation(
            currentConversationId_
        );

    /*
     * 创建新的会话 ID。
     */
    currentConversationId_ =
        QUuid::createUuid()
            .toString(
                QUuid::WithoutBraces
            );


    qDebug()
        << "[VehicleAssistant]"
        << "★ old conversation ="
        << oldId;


    qDebug()
        << "[VehicleAssistant]"
        << "★ new conversation ="
        << currentConversationId_;


    emit currentConversationIdChanged();


    /*
     * QML 收到这个信号以后
     * chatModel.clear()。
     */
    emit newConversationStarted(
        currentConversationId_
    );


    qDebug()
        << "[VehicleAssistant]"
        << "★ newConversation finished";
}

void VehicleAssistant::openConversation(
    const QString &conversationId
)
{
    const QString id =
        conversationId.trimmed();


    qDebug()
        << "[VehicleAssistant]"
        << "openConversation:"
        << id;


    if (id.isEmpty())
    {
        emit conversationOperationRejected(
            QStringLiteral(
                "历史会话 ID 无效。"
            )
        );

        return;
    }


    /*
     * ========================================================
     * 正在推理时不能切换。
     *
     * 保持前几阶段建立的原则：
     *
     * generating
     * → stop
     * → rkllm_run return
     * → Idle
     * → 才可以切换 conversation
     * ========================================================
     */
    if (llmRunning_.load())
    {
        emit conversationOperationRejected(
            QStringLiteral(
                "AI正在回答，请先停止当前回答。"
            )
        );

        return;
    }


    /*
     * 如果用户选中的就是当前会话，
     * 没必要清 RKLLM KV。
     *
     * 只重新把消息返回给 QML 即可。
     */
    if (
        id
        ==
        currentConversationId_
    )
    {
        const QVariantList messages =
            conversationManager_
                .conversationMessages(
                    id
                );


        emit conversationLoaded(
            id,
            messages
        );


        return;
    }


    /*
     * 回收上一轮结束的 std::thread。
     */
    {
        std::lock_guard<std::mutex> lock(
            llmMutex_
        );


        joinFinishedThread();
    }


    /*
     * ========================================================
     * 第一步：
     * 保存当前会话。
     * ========================================================
     */
    if (
        !conversationManager_
            .saveCurrent()
    )
    {
        qWarning()
            << "[VehicleAssistant]"
            << "save current conversation failed";

        emit conversationOperationRejected(
            QStringLiteral(
                "当前对话保存失败，无法切换历史对话。"
            )
        );

        return;
    }


    /*
     * ========================================================
     * 第二步：
     * 先确认目标会话确实可以读取。
     *
     * 暂时读取一次消息用于验证。
     * ========================================================
     */
    const QVariantList targetMessages =
        conversationManager_
            .conversationMessages(
                id
            );


    /*
     * 空消息不一定代表文件不存在，
     * 但目前我们的历史 JSON 不保存空会话，
     * 所以这里可以直接视为异常。
     */
    if (targetMessages.isEmpty())
    {
        qWarning()
            << "[VehicleAssistant]"
            << "target conversation empty/not found:"
            << id;

        emit conversationOperationRejected(
            QStringLiteral(
                "无法读取该历史对话。"
            )
        );

        return;
    }


    /*
     * ========================================================
     * 第三步：
     * 清掉当前 RKLLM 的 KV Cache。
     * ========================================================
     */
    if (
        !llm_.resetContext(true)
    )
    {
        qWarning()
            << "[VehicleAssistant]"
            << "resetContext failed while opening history";

        emit conversationOperationRejected(
            QStringLiteral(
                "无法切换历史对话：模型上下文清除失败。"
            )
        );

        return;
    }


    /*
     * ========================================================
     * 第四步：
     * ConversationManager 正式加载历史 JSON。
     * ========================================================
     */
    if (
        !conversationManager_
            .loadConversation(
                id
            )
    )
    {
        qWarning()
            << "[VehicleAssistant]"
            << "loadConversation failed:"
            << id;


        /*
         * 模型此时已经清 KV。
         *
         * 为了避免残留错误状态，
         * 关闭历史恢复标志。
         */
        restoreHistoryOnNextPrompt_ =
            false;


        restoredHistoryContext_
            .clear();


        emit conversationOperationRejected(
            QStringLiteral(
                "历史对话加载失败。"
            )
        );

        return;
    }


    /*
     * ========================================================
     * 第五步：
     * 更新 VehicleAssistant 当前 ID。
     * ========================================================
     */
    currentConversationId_ =
        id;


    emit currentConversationIdChanged();


    /*
     * ========================================================
     * 第六步：
     * 准备下一次提问时用于恢复 RKLLM 的历史文本。
     *
     * 注意：
     * 现在没有马上调用 rkllm_run。
     *
     * 用户下一次真正提问时才把历史 + 新问题
     * 一起送给模型。
     * ========================================================
     */
    restoredHistoryContext_ =
        conversationManager_
            .buildModelContext(
                2400
            );


    restoreHistoryOnNextPrompt_ =
        !restoredHistoryContext_
            .isEmpty();


    /*
     * 旧 queued token 全部失效。
     */
    generationId_
        .fetch_add(1);


    stopRequested_
        .store(false);


    /*
     * ========================================================
     * 第七步：
     * 通知 QML 把旧聊天恢复进主聊天区域。
     * ========================================================
     */
    emit conversationLoaded(
        currentConversationId_,
        targetMessages
    );


    qDebug()
        << "[VehicleAssistant]"
        << "history conversation loaded:"
        << currentConversationId_
        << "restore context ="
        << restoreHistoryOnNextPrompt_;
}

void VehicleAssistant::deleteConversation(
    const QString &conversationId
)
{
    const QString id =
        conversationId.trimmed();


    qDebug()
        << "[VehicleAssistant]"
        << "deleteConversation:"
        << id;


    if (id.isEmpty())
    {
        emit conversationOperationRejected(
            QStringLiteral(
                "要删除的历史对话无效。"
            )
        );

        return;
    }


    /*
     * ========================================================
     * 推理过程中暂时禁止删除。
     *
     * 即使删除的是其他历史文件，
     * 这里也统一禁止，避免会话管理状态变复杂。
     * ========================================================
     */
    if (llmRunning_.load())
    {
        emit conversationOperationRejected(
            QStringLiteral(
                "AI正在回答，请先停止当前回答。"
            )
        );

        return;
    }


    const bool deletingCurrent =
        (
            id
            ==
            currentConversationId_
        );


    /*
     * ========================================================
     * 情况 1：
     * 删除普通历史会话
     * ========================================================
     */
    if (!deletingCurrent)
    {
        if (
            !conversationManager_
                .deleteConversation(
                    id
                )
        )
        {
            emit conversationOperationRejected(
                QStringLiteral(
                    "删除历史对话失败。"
                )
            );

            return;
        }


        emit conversationDeleted(
            id,
            false
        );


        qDebug()
            << "[VehicleAssistant]"
            << "historical conversation deleted:"
            << id;


        return;
    }


    /*
     * ========================================================
     * 情况 2：
     * 删除当前正在使用的会话
     * ========================================================
     */


    /*
     * 确保上一次推理线程已经被回收。
     */
    {
        std::lock_guard<std::mutex> lock(
            llmMutex_
        );

        joinFinishedThread();
    }


    /*
     * --------------------------------------------------------
     * 先清除当前 RKLLM KV Cache。
     *
     * 删除当前会话后，
     * 模型绝对不能继续记住这个会话。
     * --------------------------------------------------------
     */
    if (!llm_.resetContext(true))
    {
        qWarning()
            << "[VehicleAssistant]"
            << "delete current conversation:"
            << "resetContext failed";


        emit conversationOperationRejected(
            QStringLiteral(
                "删除当前对话失败：无法清除模型上下文。"
            )
        );

        return;
    }


    /*
     * --------------------------------------------------------
     * 删除当前 JSON。
     * --------------------------------------------------------
     */
    if (
        !conversationManager_
            .deleteConversation(
                id
            )
    )
    {
        /*
         * KV 已经清掉，但文件删除失败。
         *
         * ConversationManager 内存里仍然保存着
         * 当前历史，因此下一问时重新恢复一次上下文，
         * 避免当前会话突然失忆。
         */
        restoredHistoryContext_ =
            conversationManager_
                .buildModelContext(
                    2400
                );


        restoreHistoryOnNextPrompt_ =
            !restoredHistoryContext_
                .isEmpty();


        emit conversationOperationRejected(
            QStringLiteral(
                "删除当前对话文件失败。"
            )
        );


        return;
    }


    /*
     * --------------------------------------------------------
     * 已经真正删除当前会话。
     *
     * 清掉所有旧会话恢复状态。
     * --------------------------------------------------------
     */
    restoreHistoryOnNextPrompt_ =
        false;


    restoredHistoryContext_
        .clear();


    stopRequested_
        .store(false);


    /*
     * 让所有旧 generation 的 queued token 失效。
     */
    generationId_
        .fetch_add(1);


    /*
     * --------------------------------------------------------
     * 自动创建一个新的空会话。
     * --------------------------------------------------------
     */
    currentConversationId_ =
        QUuid::createUuid()
            .toString(
                QUuid::WithoutBraces
            );


    if (
        !conversationManager_
            .startNewConversation(
                currentConversationId_
            )
    )
    {
        qWarning()
            << "[VehicleAssistant]"
            << "failed to create conversation after delete";


        emit conversationOperationRejected(
            QStringLiteral(
                "原对话已删除，但创建新对话失败。"
            )
        );


        return;
    }


    emit currentConversationIdChanged();


    /*
     * 告诉历史窗口：
     * 原会话已经删除。
     */
    emit conversationDeleted(
        id,
        true
    );


    /*
     * 继续利用之前的新对话信号。
     *
     * QML 收到以后：
     * chatModel.clear()
     * questionInput.text = ""
     * hideKeyboard()
     */
    emit newConversationStarted(
        currentConversationId_
    );


    qDebug()
        << "[VehicleAssistant]"
        << "current conversation deleted:"
        << id
        << "new conversation:"
        << currentConversationId_;
}