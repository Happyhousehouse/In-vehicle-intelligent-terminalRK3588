#pragma once

#include <QObject>
#include <QString>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

#include "../llm/LLMService.h"
#include "../rag/RAGService.h"
#include "../conversation/ConversationManager.h"
#include <QVariantList>

class CameraController;


class VehicleAssistant : public QObject
{
    Q_OBJECT

    /*
     * AI 是否正在生成。
     */
    Q_PROPERTY(
        bool generating
        READ isGenerating
        NOTIFY generatingChanged
    )

    /*
     * 是否已经发出停止请求，
     * 正在等待 RKLLM 真正停止。
     */
    Q_PROPERTY(
        bool stopping
        READ isStopping
        NOTIFY stoppingChanged
    )

    Q_PROPERTY(
        QString currentConversationId
        READ currentConversationId
        NOTIFY currentConversationIdChanged
    )

public:

    explicit VehicleAssistant(
        QObject *parent = nullptr
    );

    ~VehicleAssistant();


    bool init(
        const std::string &model,
        const std::string &knowledge
    );


    /*
     * 发送问题。
     */
    Q_INVOKABLE
    void chat(
        const QString &question
    );


    /*
     * 停止当前回答。
     */
    Q_INVOKABLE
    void stopGeneration();

    Q_INVOKABLE
    void newConversation();

    QString currentConversationId() const;


    bool isGenerating() const;

    bool isStopping() const;

    /*
    * 获取历史会话列表。
    */
    Q_INVOKABLE
    QVariantList conversationHistory() const;


    /*
    * 查看指定历史会话中的消息。
    *
    * 注意：
    * 当前只是查看，
    * 不会切换 RKLLM 当前上下文。
    */
    Q_INVOKABLE
    QVariantList historyMessages(
        const QString &conversationId
    ) const;

    /*
    * 从历史列表切换到指定会话并继续聊天。
    */
    Q_INVOKABLE
    void openConversation(
        const QString &conversationId
    );

    /*
    * 删除历史会话。
    *
    * 如果删除的是当前会话，
    * 会自动创建一个新的空会话。
    */
    Q_INVOKABLE
    void deleteConversation(
        const QString &conversationId
    );

signals:

    /*
     * 流式输出。
     */
    void answerToken(
        QString token
    );


    /*
     * 无论正常结束还是停止，
     * 最后都会发出。
     */
    void answerFinished();


    /*
     * 用户主动停止了回答。
     *
     * 后续 ConversationManager
     * 可以利用这个信号把消息标记为 interrupted。
     */
    void answerStopped();


    void generatingChanged();

    void stoppingChanged();


    /*
     * 拒绝重复发送。
     */
    void chatRejected(
        QString reason
    );


    /*
     * 页面控制。
     */
    void openCameraPage();

    void openGalleryPage();

    void currentConversationIdChanged();

    void newConversationStarted(
        QString conversationId
    );

    void conversationOperationRejected(
        QString reason
    );

    /*
    * 历史会话已经成功切换为当前会话。
    *
    * QML 收到后把历史消息恢复到主聊天框。
    */
    void conversationLoaded(
        QString conversationId,
        QVariantList messages
    );

    /*
    * 删除成功。
    *
    * wasCurrent:
    * true  = 删除的是当前活动会话
    * false = 删除普通历史会话
    */
    void conversationDeleted(
        QString conversationId,
        bool wasCurrent
    );

private:

    void setGenerating(
        bool generating
    );


    void setStopping(
        bool stopping
    );


    /*
     * 回收已经结束的上一条线程。
     */
    void joinFinishedThread();


private:

    LLMService llm_;

    RAGService rag_;

    ConversationManager conversationManager_;
    
    std::thread llmThread_;


    /*
     * 当前是否仍有一轮生成任务。
     */
    std::atomic<bool> llmRunning_{false};


    /*
     * 是否已经请求停止。
     */
    std::atomic<bool> stopRequested_{false};


    /*
     * generation ID：
     * 防止旧 token 混入下一轮回答。
     */
    std::atomic<std::uint64_t> generationId_{0};


    std::mutex llmMutex_;

    QString currentConversationId_;

    /*
    * ============================================================
    * 历史会话恢复
    * ============================================================
    */


    /*
    * true：
    * 下一条用户问题需要带上旧会话历史，
    * 用来重新建立 RKLLM 上下文。
    */
    bool restoreHistoryOnNextPrompt_ = false;


    /*
    * 打开历史会话时预先构造好的历史上下文。
    *
    * 下一次发送成功后清空。
    */
    QString restoredHistoryContext_;
};