#include "VoiceCommandNormalizer.h"

#include <QRegularExpression>


QString VoiceCommandNormalizer::normalize(
    const QString &text
)
{
    QString result =
        normalizeBasic(
            text
        );


    result =
        normalizeKnownCommands(
            result
        );


    return result;
}


QString VoiceCommandNormalizer::normalizeBasic(
    const QString &text
)
{
    QString result =
        text.trimmed();


    /*
     * 去掉 SenseVoice 经常附加的句尾标点：
     *
     * 。！？，,.!?
     */
    static const QRegularExpression trailing(
        QStringLiteral(
            R"([\s，。！？,.!?]+$)"
        )
    );


    result.remove(
        trailing
    );


    return result.trimmed();
}


QString VoiceCommandNormalizer::normalizeKnownCommands(
    const QString &text
)
{
    QString result =
        text;


    /*
     * ========================================================
     * 疲劳检测
     * ========================================================
     *
     * 当前实际测试中出现过：
     *
     * 疲劳检测
     *      ↓
     * 皮劳检测
     *
     * 这里只修正明确知道的固定业务词。
     */

    result.replace(
        QStringLiteral(
            "皮劳检测"
        ),
        QStringLiteral(
            "疲劳检测"
        )
    );


    result.replace(
        QStringLiteral(
            "疲老检测"
        ),
        QStringLiteral(
            "疲劳检测"
        )
    );


    result.replace(
        QStringLiteral(
            "皮劳监测"
        ),
        QStringLiteral(
            "疲劳监测"
        )
    );


    result.replace(
        QStringLiteral(
            "疲老监测"
        ),
        QStringLiteral(
            "疲劳监测"
        )
    );


    return result;
}