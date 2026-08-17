#pragma once

#include <QString>


/*
 * ============================================================
 * VoiceCommandNormalizer
 *
 * 只负责：
 *
 * 对 ASR 结果做少量、确定性的命令词修正。
 *
 * 例如：
 *
 * “皮劳检测”
 *      ↓
 * “疲劳检测”
 *
 *
 * 注意：
 *
 * 这里不是通用纠错器。
 *
 * 不应该随意修改普通聊天内容，
 * 只修我们已经明确知道的固定车载命令词。
 * ============================================================
 */
class VoiceCommandNormalizer
{
public:

    /*
     * 对 ASR 文本进行标准化。
     */
    static QString normalize(
        const QString &text
    );


private:

    /*
     * 处理基础格式：
     *
     * 去首尾空格
     * 去句尾标点
     */
    static QString normalizeBasic(
        const QString &text
    );


    /*
     * 修正已知固定命令词。
     */
    static QString normalizeKnownCommands(
        const QString &text
    );
};