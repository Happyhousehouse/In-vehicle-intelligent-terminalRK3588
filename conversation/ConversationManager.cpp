#include "ConversationManager.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QFileInfo>
#include <QVariantMap>

#include <algorithm>

ConversationManager::ConversationManager()
{
    storageDir_ =
        QStringLiteral(
            "/home/firefly/VehicleTerminal/data/conversations"
        );


    QDir dir;

    if (!dir.mkpath(storageDir_))
    {
        qWarning()
            << "[ConversationManager]"
            << "Failed to create directory:"
            << storageDir_;
    }
}


QString ConversationManager::currentTimeString() const
{
    return QDateTime::currentDateTime()
        .toString(
            Qt::ISODate
        );
}


QString ConversationManager::currentConversationId() const
{
    return currentConversationId_;
}


bool ConversationManager::hasMessages() const
{
    return !messages_.isEmpty();
}


bool ConversationManager::startNewConversation(
    const QString &id
)
{
    if (id.trimmed().isEmpty())
    {
        qWarning()
            << "[ConversationManager]"
            << "startNewConversation:"
            << "empty id";

        return false;
    }


    currentConversationId_ =
        id;


    title_ =
        QStringLiteral("新对话");


    createdAt_ =
        currentTimeString();


    updatedAt_ =
        createdAt_;


    messages_ =
        QJsonArray();


    streamingAssistantIndex_ =
        -1;


    qDebug()
        << "[ConversationManager]"
        << "new session:"
        << currentConversationId_;


    return true;
}


void ConversationManager::updateTitleFromFirstUserMessage(
    const QString &content
)
{
    /*
     * 只有第一次用户消息才生成标题。
     */
    if (title_ != QStringLiteral("新对话"))
        return;


    QString text =
        content.trimmed();


    /*
     * 去掉换行，
     * 避免历史列表标题出现多行。
     */
    text.replace(
        '\n',
        ' '
    );


    text.replace(
        '\r',
        ' '
    );


    /*
     * 标题不要太长。
     */
    constexpr int maxTitleLength = 24;


    if (text.length() > maxTitleLength)
    {
        text =
            text.left(maxTitleLength)
            +
            QStringLiteral("...");
    }


    if (!text.isEmpty())
    {
        title_ = text;
    }
}


void ConversationManager::appendUserMessage(
    const QString &content
)
{
    if (currentConversationId_.isEmpty())
        return;


    updateTitleFromFirstUserMessage(
        content
    );


    QJsonObject message;


    message.insert(
        QStringLiteral("role"),
        QStringLiteral("user")
    );


    message.insert(
        QStringLiteral("content"),
        content
    );


    message.insert(
        QStringLiteral("time"),
        currentTimeString()
    );


    message.insert(
        QStringLiteral("status"),
        QStringLiteral("complete")
    );


    messages_.append(
        message
    );


    updatedAt_ =
        currentTimeString();


    qDebug()
        << "[ConversationManager]"
        << "user message added";
}


void ConversationManager::appendAssistantMessage(
    const QString &content,
    const QString &status
)
{
    if (currentConversationId_.isEmpty())
        return;


    QJsonObject message;


    message.insert(
        QStringLiteral("role"),
        QStringLiteral("assistant")
    );


    message.insert(
        QStringLiteral("content"),
        content
    );


    message.insert(
        QStringLiteral("time"),
        currentTimeString()
    );


    message.insert(
        QStringLiteral("status"),
        status
    );


    messages_.append(
        message
    );


    updatedAt_ =
        currentTimeString();


    qDebug()
        << "[ConversationManager]"
        << "assistant message added";
}


void ConversationManager::beginAssistantMessage()
{
    if (currentConversationId_.isEmpty())
        return;


    /*
     * 防止重复创建。
     */
    if (streamingAssistantIndex_ >= 0)
    {
        qWarning()
            << "[ConversationManager]"
            << "assistant message already streaming";

        return;
    }


    QJsonObject message;


    message.insert(
        QStringLiteral("role"),
        QStringLiteral("assistant")
    );


    message.insert(
        QStringLiteral("content"),
        QString()
    );


    message.insert(
        QStringLiteral("time"),
        currentTimeString()
    );


    message.insert(
        QStringLiteral("status"),
        QStringLiteral("streaming")
    );


    messages_.append(
        message
    );


    streamingAssistantIndex_ =
        messages_.size() - 1;


    updatedAt_ =
        currentTimeString();


    qDebug()
        << "[ConversationManager]"
        << "assistant streaming started";
}


void ConversationManager::appendAssistantToken(
    const QString &token
)
{
    if (streamingAssistantIndex_ < 0)
        return;


    if (
        streamingAssistantIndex_
        >= messages_.size()
    )
    {
        return;
    }


    QJsonObject message =
        messages_.at(
            streamingAssistantIndex_
        )
        .toObject();


    QString content =
        message.value(
            QStringLiteral("content")
        )
        .toString();


    content += token;


    message.insert(
        QStringLiteral("content"),
        content
    );


    messages_.replace(
        streamingAssistantIndex_,
        message
    );


    updatedAt_ =
        currentTimeString();
}


void ConversationManager::finishAssistantMessage(
    const QString &status
)
{
    if (streamingAssistantIndex_ < 0)
        return;


    if (
        streamingAssistantIndex_
        >= messages_.size()
    )
    {
        streamingAssistantIndex_ =
            -1;

        return;
    }


    QJsonObject message =
        messages_.at(
            streamingAssistantIndex_
        )
        .toObject();


    message.insert(
        QStringLiteral("status"),
        status
    );


    /*
     * 更新时间。
     */
    message.insert(
        QStringLiteral("time"),
        currentTimeString()
    );


    messages_.replace(
        streamingAssistantIndex_,
        message
    );


    qDebug()
        << "[ConversationManager]"
        << "assistant finished:"
        << status;


    streamingAssistantIndex_ =
        -1;


    updatedAt_ =
        currentTimeString();
}


QString ConversationManager::currentFilePath() const
{
    if (currentConversationId_.isEmpty())
        return QString();


    return
        storageDir_
        +
        QStringLiteral("/")
        +
        currentConversationId_
        +
        QStringLiteral(".json");
}


bool ConversationManager::saveCurrent()
{
    if (currentConversationId_.isEmpty())
    {
        qWarning()
            << "[ConversationManager]"
            << "save failed:"
            << "no current conversation";

        return false;
    }


    /*
     * 空会话暂时不保存。
     *
     * 防止用户连续点“新对话”
     * 产生大量空 JSON。
     */
    if (messages_.isEmpty())
    {
        qDebug()
            << "[ConversationManager]"
            << "skip empty conversation";

        return true;
    }


    QJsonObject root;


    root.insert(
        QStringLiteral("id"),
        currentConversationId_
    );


    root.insert(
        QStringLiteral("title"),
        title_
    );


    root.insert(
        QStringLiteral("createdAt"),
        createdAt_
    );


    root.insert(
        QStringLiteral("updatedAt"),
        updatedAt_
    );


    /*
     * 后面做跨会话记忆时使用。
     * 当前先保留字段。
     */
    root.insert(
        QStringLiteral("summary"),
        QString()
    );


    root.insert(
        QStringLiteral("messages"),
        messages_
    );


    const QString filePath =
        currentFilePath();


    QFile file(
        filePath
    );


    if (
        !file.open(
            QIODevice::WriteOnly
            |
            QIODevice::Truncate
        )
    )
    {
        qWarning()
            << "[ConversationManager]"
            << "cannot open:"
            << filePath;

        return false;
    }


    QJsonDocument document(
        root
    );


    const qint64 written =
        file.write(
            document.toJson(
                QJsonDocument::Indented
            )
        );


    file.close();


    if (written <= 0)
    {
        qWarning()
            << "[ConversationManager]"
            << "write failed:"
            << filePath;

        return false;
    }


    qDebug()
        << "[ConversationManager]"
        << "saved:"
        << filePath;


    return true;
}

bool ConversationManager::isValidConversationId(
    const QString &conversationId
) const
{
    const QString id =
        conversationId.trimmed();


    if (id.isEmpty())
        return false;


    /*
     * conversationId 正常是 UUID。
     *
     * 至少阻止通过：
     *
     * ../
     * /
     * \
     *
     * 去读取 conversations 目录外的文件。
     */
    if (
        id.contains(QStringLiteral(".."))
        ||
        id.contains('/')
        ||
        id.contains('\\')
    )
    {
        return false;
    }


    return true;
}

QVariantList ConversationManager::conversationList() const
{
    QVariantList result;


    QDir dir(
        storageDir_
    );


    if (!dir.exists())
    {
        qWarning()
            << "[ConversationManager]"
            << "history directory does not exist:"
            << storageDir_;

        return result;
    }


    /*
     * 找到所有会话 JSON。
     */
    const QFileInfoList fileList =
        dir.entryInfoList(
            QStringList()
                << QStringLiteral("*.json"),

            QDir::Files
            |
            QDir::Readable,

            QDir::Name
        );


    /*
     * 先使用 QVariantMap 保存，
     * 最后按 updatedAt 排序。
     */
    QList<QVariantMap> conversations;


    for (const QFileInfo &fileInfo : fileList)
    {
        QFile file(
            fileInfo.absoluteFilePath()
        );


        if (
            !file.open(
                QIODevice::ReadOnly
            )
        )
        {
            qWarning()
                << "[ConversationManager]"
                << "cannot open history file:"
                << fileInfo.absoluteFilePath();

            continue;
        }


        const QByteArray data =
            file.readAll();


        file.close();


        QJsonParseError error;


        const QJsonDocument document =
            QJsonDocument::fromJson(
                data,
                &error
            );


        if (
            error.error
            !=
            QJsonParseError::NoError
        )
        {
            qWarning()
                << "[ConversationManager]"
                << "JSON parse error:"
                << fileInfo.fileName()
                << error.errorString();

            continue;
        }


        if (!document.isObject())
            continue;


        const QJsonObject root =
            document.object();


        const QString id =
            root.value(
                QStringLiteral("id")
            )
            .toString();


        /*
         * 跳过异常文件。
         */
        if (!isValidConversationId(id))
            continue;


        QString title =
            root.value(
                QStringLiteral("title")
            )
            .toString();


        if (title.trimmed().isEmpty())
        {
            title =
                QStringLiteral("未命名对话");
        }


        const QString createdAt =
            root.value(
                QStringLiteral("createdAt")
            )
            .toString();


        const QString updatedAt =
            root.value(
                QStringLiteral("updatedAt")
            )
            .toString();


        const QJsonArray messages =
            root.value(
                QStringLiteral("messages")
            )
            .toArray();


        QVariantMap item;


        item.insert(
            QStringLiteral("conversationId"),
            id
        );


        item.insert(
            QStringLiteral("title"),
            title
        );


        item.insert(
            QStringLiteral("createdAt"),
            createdAt
        );


        item.insert(
            QStringLiteral("updatedAt"),
            updatedAt
        );


        item.insert(
            QStringLiteral("messageCount"),
            messages.size()
        );


        /*
         * 是否为当前会话。
         *
         * 以后 UI 可以显示“当前”。
         */
        item.insert(
            QStringLiteral("isCurrent"),
            id == currentConversationId_
        );


        conversations.append(
            item
        );
    }


    /*
     * 更新时间新的排前面。
     */
    std::sort(
        conversations.begin(),
        conversations.end(),

        [](
            const QVariantMap &a,
            const QVariantMap &b
        )
        {
            const QDateTime timeA =
                QDateTime::fromString(
                    a.value(
                        QStringLiteral("updatedAt")
                    )
                    .toString(),

                    Qt::ISODate
                );


            const QDateTime timeB =
                QDateTime::fromString(
                    b.value(
                        QStringLiteral("updatedAt")
                    )
                    .toString(),

                    Qt::ISODate
                );


            return timeA > timeB;
        }
    );


    for (const QVariantMap &item : conversations)
    {
        result.append(
            item
        );
    }


    qDebug()
        << "[ConversationManager]"
        << "history count ="
        << result.size();


    return result;
}

QVariantList ConversationManager::conversationMessages(
    const QString &conversationId
) const
{
    QVariantList result;


    if (
        !isValidConversationId(
            conversationId
        )
    )
    {
        qWarning()
            << "[ConversationManager]"
            << "invalid conversation id:"
            << conversationId;

        return result;
    }


    const QString filePath =
        storageDir_
        +
        QStringLiteral("/")
        +
        conversationId
        +
        QStringLiteral(".json");


    QFile file(
        filePath
    );


    if (
        !file.open(
            QIODevice::ReadOnly
        )
    )
    {
        qWarning()
            << "[ConversationManager]"
            << "cannot open conversation:"
            << filePath;

        return result;
    }


    const QByteArray data =
        file.readAll();


    file.close();


    QJsonParseError error;


    const QJsonDocument document =
        QJsonDocument::fromJson(
            data,
            &error
        );


    if (
        error.error
        !=
        QJsonParseError::NoError
    )
    {
        qWarning()
            << "[ConversationManager]"
            << "conversation JSON error:"
            << error.errorString();

        return result;
    }


    if (!document.isObject())
        return result;


    const QJsonObject root =
        document.object();


    const QJsonArray messages =
        root.value(
            QStringLiteral("messages")
        )
        .toArray();


    for (const QJsonValue &value : messages)
    {
        if (!value.isObject())
            continue;


        const QJsonObject object =
            value.toObject();


        QVariantMap message;


        message.insert(
            QStringLiteral("role"),
            object.value(
                QStringLiteral("role")
            )
            .toString()
        );


        message.insert(
            QStringLiteral("content"),
            object.value(
                QStringLiteral("content")
            )
            .toString()
        );


        message.insert(
            QStringLiteral("time"),
            object.value(
                QStringLiteral("time")
            )
            .toString()
        );


        message.insert(
            QStringLiteral("status"),
            object.value(
                QStringLiteral("status")
            )
            .toString()
        );


        result.append(
            message
        );
    }


    qDebug()
        << "[ConversationManager]"
        << "loaded conversation:"
        << conversationId
        << "messages ="
        << result.size();


    return result;
}

bool ConversationManager::loadConversation(
    const QString &conversationId
)
{
    if (
        !isValidConversationId(
            conversationId
        )
    )
    {
        qWarning()
            << "[ConversationManager]"
            << "loadConversation invalid id:"
            << conversationId;

        return false;
    }


    const QString filePath =
        storageDir_
        +
        QStringLiteral("/")
        +
        conversationId
        +
        QStringLiteral(".json");


    QFile file(
        filePath
    );


    if (
        !file.open(
            QIODevice::ReadOnly
        )
    )
    {
        qWarning()
            << "[ConversationManager]"
            << "loadConversation cannot open:"
            << filePath;

        return false;
    }


    const QByteArray data =
        file.readAll();


    file.close();


    QJsonParseError error;


    const QJsonDocument document =
        QJsonDocument::fromJson(
            data,
            &error
        );


    if (
        error.error
        !=
        QJsonParseError::NoError
    )
    {
        qWarning()
            << "[ConversationManager]"
            << "loadConversation JSON error:"
            << error.errorString();

        return false;
    }


    if (!document.isObject())
    {
        return false;
    }


    /*
     * 先解析到局部变量。
     *
     * 全部成功以后才修改当前会话，
     * 防止坏 JSON 把当前 ConversationManager 状态破坏。
     */
    const QJsonObject root =
        document.object();


    const QString loadedId =
        root.value(
            QStringLiteral("id")
        )
        .toString();


    if (
        loadedId
        !=
        conversationId
    )
    {
        qWarning()
            << "[ConversationManager]"
            << "conversation id mismatch:"
            << loadedId
            << conversationId;

        return false;
    }


    QString loadedTitle =
        root.value(
            QStringLiteral("title")
        )
        .toString();


    if (loadedTitle.isEmpty())
    {
        loadedTitle =
            QStringLiteral("未命名对话");
    }


    QString loadedCreatedAt =
        root.value(
            QStringLiteral("createdAt")
        )
        .toString();


    QString loadedUpdatedAt =
        root.value(
            QStringLiteral("updatedAt")
        )
        .toString();


    const QJsonValue messagesValue =
        root.value(
            QStringLiteral("messages")
        );


    if (!messagesValue.isArray())
    {
        qWarning()
            << "[ConversationManager]"
            << "messages is not array";

        return false;
    }


    const QJsonArray loadedMessages =
        messagesValue.toArray();


    /*
     * ========================================================
     * 到这里文件已经验证成功。
     * 现在正式切换 ConversationManager 当前会话。
     * ========================================================
     */

    currentConversationId_ =
        loadedId;


    title_ =
        loadedTitle;


    createdAt_ =
        loadedCreatedAt;


    updatedAt_ =
        loadedUpdatedAt;


    messages_ =
        loadedMessages;


    /*
     * 历史文件中不能继续保持 streaming 状态。
     */
    streamingAssistantIndex_ =
        -1;


    qDebug()
        << "[ConversationManager]"
        << "conversation loaded:"
        << currentConversationId_
        << "messages ="
        << messages_.size();


    return true;
}

QString ConversationManager::buildModelContext(
    int maxCharacters
) const
{
    if (
        messages_.isEmpty()
        ||
        maxCharacters <= 0
    )
    {
        return QString();
    }


    /*
     * 我们从最近的聊天向前取。
     *
     * 原因：
     * RKLLM max_context_len = 4096，
     * 不应该把一个非常长的历史会话全部塞回模型。
     */
    QStringList selected;


    int currentLength = 0;


    for (
        int i = messages_.size() - 1;
        i >= 0;
        --i
    )
    {
        const QJsonObject message =
            messages_.at(i)
                .toObject();


        const QString role =
            message.value(
                QStringLiteral("role")
            )
            .toString();


        const QString content =
            message.value(
                QStringLiteral("content")
            )
            .toString()
            .trimmed();


        const QString status =
            message.value(
                QStringLiteral("status")
            )
            .toString();


        if (content.isEmpty())
        {
            continue;
        }


        QString line;


        if (role == QStringLiteral("user"))
        {
            line =
                QStringLiteral("用户：")
                +
                content;
        }
        else if (
            role
            ==
            QStringLiteral("assistant")
        )
        {
            /*
             * 被用户打断的回答不是完整结论。
             *
             * 当前恢复模型上下文时不使用。
             */
            if (
                status
                !=
                QStringLiteral("complete")
            )
            {
                continue;
            }


            line =
                QStringLiteral("助手：")
                +
                content;
        }
        else
        {
            continue;
        }


        /*
         * +2 给换行留一点空间。
         */
        const int required =
            line.length() + 2;


        if (
            currentLength + required
            >
            maxCharacters
        )
        {
            /*
             * 已经至少有一些近期上下文，
             * 就不再继续向前加。
             */
            if (!selected.isEmpty())
            {
                break;
            }


            /*
             * 如果单条消息自己就特别长，
             * 只保留尾部。
             */
            line =
                line.right(
                    maxCharacters
                );
        }


        selected.prepend(
            line
        );


        currentLength +=
            line.length() + 2;
    }


    if (selected.isEmpty())
    {
        return QString();
    }


    return selected.join(
        QStringLiteral("\n\n")
    );
}

bool ConversationManager::deleteConversation(
    const QString &conversationId
)
{
    const QString id =
        conversationId.trimmed();


    if (!isValidConversationId(id))
    {
        qWarning()
            << "[ConversationManager]"
            << "deleteConversation invalid id:"
            << id;

        return false;
    }


    const QString filePath =
        storageDir_
        +
        QStringLiteral("/")
        +
        id
        +
        QStringLiteral(".json");


    QFile file(
        filePath
    );


    /*
     * 文件不存在。
     */
    if (!file.exists())
    {
        qWarning()
            << "[ConversationManager]"
            << "conversation file does not exist:"
            << filePath;

        return false;
    }


    /*
     * 删除 JSON。
     */
    if (!file.remove())
    {
        qWarning()
            << "[ConversationManager]"
            << "delete failed:"
            << filePath
            << file.errorString();

        return false;
    }


    qDebug()
        << "[ConversationManager]"
        << "conversation deleted:"
        << id;


    return true;
}