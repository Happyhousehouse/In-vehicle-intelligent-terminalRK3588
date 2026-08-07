#pragma once

#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariantList>

class ConversationManager
{
public:

    ConversationManager();


    /*
     * 创建新的当前会话。
     *
     * id 由 VehicleAssistant 生成。
     */
    bool startNewConversation(
        const QString &id
    );


    /*
     * 当前会话 ID。
     */
    QString currentConversationId() const;


    /*
     * 当前会话是否有消息。
     */
    bool hasMessages() const;


    /*
     * 添加用户消息。
     */
    void appendUserMessage(
        const QString &content
    );


    /*
     * 添加一个完整的 AI 消息。
     *
     * 用于：
     * 打开相册
     * 打开摄像头
     * 等本地命令。
     */
    void appendAssistantMessage(
        const QString &content,
        const QString &status = QStringLiteral("complete")
    );


    /*
     * LLM 开始流式回答。
     *
     * 先创建一个空的 assistant 消息。
     */
    void beginAssistantMessage();


    /*
     * 追加流式 token。
     */
    void appendAssistantToken(
        const QString &token
    );


    /*
     * 完成当前 assistant 消息。
     *
     * status:
     *
     * complete
     * interrupted
     * error
     */
    void finishAssistantMessage(
        const QString &status
    );


    /*
     * 保存当前会话到 JSON。
     */
    bool saveCurrent();
    QVariantList conversationList() const;
    QVariantList conversationMessages(
        const QString &conversationId
    ) const;

    /*
    * 从 JSON 加载一个历史会话，
    * 并把它变成当前 ConversationManager 会话。
    */
    bool loadConversation(
        const QString &conversationId
    );


    /*
    * 给模型构造恢复上下文。
    *
    * 只取最近一部分历史，
    * 防止超过 RKLLM 4096 context。
    */
    QString buildModelContext(
        int maxCharacters = 2400
    ) const;

    bool deleteConversation(
        const QString &conversationId
    );

private:

    /*
     * 自动生成会话标题。
     *
     * 当前直接使用第一条用户问题。
     */
    void updateTitleFromFirstUserMessage(
        const QString &content
    );


    /*
     * 当前时间。
     */
    QString currentTimeString() const;


    /*
     * 当前 JSON 文件路径。
     */
    QString currentFilePath() const;


private:

    QString storageDir_;


    QString currentConversationId_;


    QString title_;


    QString createdAt_;


    QString updatedAt_;


    QJsonArray messages_;


    /*
     * 当前正在流式写入的 assistant
     * 在 messages_ 中的位置。
     *
     * -1 表示没有。
     */
    int streamingAssistantIndex_ = -1;
    
    bool isValidConversationId(
        const QString &conversationId
    ) const;
};