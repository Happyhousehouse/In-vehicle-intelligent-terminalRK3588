#include "VehicleAssistant.h"

#include <QDebug>
#include <QMetaObject>
#include <QUuid>
#include <QRegularExpression>
#include <QVariantMap>

#include "../weather/WeatherService.h"

#include "../vision/VisionServiceManager.h"

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

void VehicleAssistant::setWeatherService(
    WeatherService *service
)
{
    /*
     * 同一个对象不重复绑定。
     */
    if (
        weatherService_
        ==
        service
    )
    {
        return;
    }


    /*
     * 如果以前绑定过天气服务，
     * 清掉旧信号。
     */
    if (weatherService_)
    {
        QObject::disconnect(
            weatherService_,
            nullptr,
            this,
            nullptr
        );
    }


    weatherService_ =
        service;


    if (!weatherService_)
    {
        return;
    }


    /*
     * ========================================================
     * 城市定位成功
     *
     * 注意：
     *
     * 这里还不能马上回答。
     *
     * 因为 citySearchSucceeded
     * 只是意味着经纬度找到了，
     * 新城市天气还没有回来。
     * ========================================================
     */
    connect(
        weatherService_,
        &WeatherService::citySearchSucceeded,

        this,

        [this](const QString &city)
        {
            if (
                weatherPendingMode_
                !=
                WeatherPendingMode::CityLocating
            )
            {
                return;
            }


            qDebug()
                << "[VehicleAssistant]"
                << "weather city located:"
                << city;


            /*
             * WeatherService 接下来会自动 refresh()。
             *
             * 等 weatherChanged() 再真正回答。
             */
            weatherPendingMode_ =
                WeatherPendingMode::CityWeather;
        }
    );


    /*
     * ========================================================
     * 城市搜索失败
     * ========================================================
     */
    connect(
        weatherService_,
        &WeatherService::citySearchFailed,

        this,

        [this](const QString &error)
        {
            if (
                weatherPendingMode_
                !=
                WeatherPendingMode::CityLocating
            )
            {
                return;
            }


            const QString requestedCity =
                pendingWeatherCity_;


            weatherPendingMode_ =
                WeatherPendingMode::None;


            pendingWeatherCity_
                .clear();


            qWarning()
                << "[VehicleAssistant]"
                << "weather city search failed:"
                << requestedCity
                << error;


            finishLocalAnswer(
                QStringLiteral(
                    "没有找到“%1”的天气信息，请检查地区名称是否正确。"
                )
                .arg(
                    requestedCity
                )
            );
        }
    );


    /*
     * ========================================================
     * 天气真正更新成功
     * ========================================================
     */
    connect(
        weatherService_,
        &WeatherService::weatherChanged,

        this,

        [this]()
        {
            /*
             * ------------------------------------------------
             * 情况1：
             * 用户问当前城市天气。
             * ------------------------------------------------
             */
            if (
                weatherPendingMode_
                ==
                WeatherPendingMode::CurrentCity
            )
            {
                weatherPendingMode_ =
                    WeatherPendingMode::None;


                pendingWeatherCity_
                    .clear();


                finishLocalAnswer(
                    buildWeatherAnswer()
                );


                return;
            }


            /*
             * ------------------------------------------------
             * 情况2：
             *
             * 原来的天气请求结束了，
             * 现在开始用户排队等待的新城市。
             * ------------------------------------------------
             */
            if (
                weatherPendingMode_
                ==
                WeatherPendingMode::CityQueued
            )
            {
                const QString city =
                    pendingWeatherCity_;


                weatherPendingMode_ =
                    WeatherPendingMode::CityLocating;


                weatherService_
                    ->searchCity(
                        city
                    );


                return;
            }


            /*
             * ------------------------------------------------
             * 情况3：
             *
             * 新城市天气真正返回。
             * ------------------------------------------------
             */
            if (
                weatherPendingMode_
                ==
                WeatherPendingMode::CityWeather
            )
            {
                weatherPendingMode_ =
                    WeatherPendingMode::None;


                pendingWeatherCity_
                    .clear();


                /*
                 * 此时 WeatherService 内部已经完全是
                 * 新城市的数据。
                 */
                finishLocalAnswer(
                    buildWeatherAnswer()
                );


                return;
            }
        }
    );


    /*
     * ========================================================
     * 天气 HTTPS 请求失败
     * ========================================================
     */
    connect(
        weatherService_,
        &WeatherService::updateFailed,

        this,

        [this](const QString &error)
        {
            /*
             * 如果只是 Weather.qml 自己刷新失败，
             * 不要往 AI 聊天框输出。
             */
            if (
                weatherPendingMode_
                ==
                WeatherPendingMode::None
            )
            {
                return;
            }


            /*
             * 特殊情况：
             *
             * 用户想查另外一个城市，
             * 但是此前的旧天气请求失败。
             *
             * 旧请求结束以后仍然继续查新城市。
             */
            if (
                weatherPendingMode_
                ==
                WeatherPendingMode::CityQueued
            )
            {
                const QString city =
                    pendingWeatherCity_;


                weatherPendingMode_ =
                    WeatherPendingMode::CityLocating;


                weatherService_
                    ->searchCity(
                        city
                    );


                return;
            }


            weatherPendingMode_ =
                WeatherPendingMode::None;


            pendingWeatherCity_
                .clear();


            qWarning()
                << "[VehicleAssistant]"
                << "weather request failed:"
                << error;


            finishLocalAnswer(
                QStringLiteral(
                    "天气数据获取失败，请检查网络连接后重新尝试。"
                )
            );
        }
    );


    qDebug()
        << "[VehicleAssistant]"
        << "WeatherService connected";
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
 * Weather 页面状态
 * ============================================================
 */

void VehicleAssistant::setWeatherPageActive(
    bool active
)
{
    if (
        weatherPageActive_
        ==
        active
    )
    {
        return;
    }

    weatherPageActive_ =
        active;

    qDebug()
        << "[VehicleAssistant]"
        << "weather page active ="
        << weatherPageActive_;
}


bool VehicleAssistant::isWeatherPageActive() const
{
    return weatherPageActive_;
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
    * ============================================================
    * Weather.qml 页面内语音天气查询
    * ============================================================
    *
    * Weather.qml 当前正在显示时：
    *
    * “广州天气怎么样”
    * “查询深圳天气”
    * “告诉我北京现在什么天气”
    *
    * 不进入 AI 聊天历史，
    * 而是直接把城市交给 Weather.qml。
    * ============================================================
    */

    /*
    * ============================================================
    * 退出 Weather.qml
    * ============================================================
    *
    * 必须放在：
    *
    * weatherPageActive_ && isWeatherQuestion(cmd)
    *
    * 前面。
    *
    * 因为“退出天气模块”本身也包含“天气”，
    * 如果放在后面，就会先被当成天气查询。
    * ============================================================
    */
    if (
        weatherPageActive_
        &&
        (
            cmd == QStringLiteral("退出天气模块")
            ||
            cmd == QStringLiteral("推出天气模块")
            ||
            cmd == QStringLiteral("关闭天气模块")
            ||
            cmd == QStringLiteral("退出天气页面")
            ||
            cmd == QStringLiteral("推出天气页面")
            ||
            cmd == QStringLiteral("关闭天气页面")
            ||
            cmd == QStringLiteral("退出天气界面")
            ||
            cmd == QStringLiteral("推出天气界面")
            ||
            cmd == QStringLiteral("关闭天气界面")
        )
    )
    {
        qDebug()
            << "[VehicleAssistant]"
            << "exit Weather.qml command:"
            << cmd;

        QMetaObject::invokeMethod(
            this,

            [this]()
            {
                emit returnToMainPage();
            },

            Qt::QueuedConnection
        );

        /*
        * 这里属于页面控制命令。
        *
        * 因为此位置还没有进入 AI 聊天历史，
        * 不调用 finishLocalAnswer()。
        *
        * 只告诉 VoiceController：
        * 本轮命令已经处理完成。
        */
        emit answerFinished();

        return;
    }
    if (
        weatherPageActive_
        &&
        isWeatherQuestion(
            cmd
        )
    )
    {
        /*
        * “打开天气页面”属于页面控制，
        * 不能误当成页面内部的天气查询。
        */
        const bool openWeatherCommand =
            cmd.contains(
                QStringLiteral(
                    "打开天气页面"
                )
            )
            ||
            cmd.contains(
                QStringLiteral(
                    "打开天气界面"
                )
            )
            ||
            cmd.contains(
                QStringLiteral(
                    "进入天气页面"
                )
            )
            ||
            cmd.contains(
                QStringLiteral(
                    "进入天气界面"
                )
            )
            ||
            cmd.contains(
                QStringLiteral(
                    "打开天气模块"
                )
            )
            ||
            cmd
            ==
            QStringLiteral(
                "打开天气"
            );


        if (!openWeatherCommand)
        {
            const QString city =
                extractWeatherCity(
                    cmd
                );


            qDebug()
                << "[VehicleAssistant]"
                << "route weather query to Weather.qml"
                << "city ="
                << city;


            /*
            * 不调用：
            *
            * userMessageAccepted()
            * appendUserMessage()
            * startWeatherCitySearch()
            *
            * 所以不会进入 AI 聊天框。
            */
            emit weatherPageQueryRequested(
                city
            );


            /*
            * 非常重要：
            *
            * VoiceController 已经把这一轮状态切到了 ANSWERING。
            * 虽然这次不需要 AI 回答，
            * 仍然必须告诉 VoiceController：
            * 本轮命令处理完成。
            *
            * 否则语音状态会一直卡在 ANSWERING。
            */
            emit answerFinished();


            return;
        }
    }
    /*
    * ========================================================
    * 保存用户消息。
    * ========================================================
    */

    /*
    * ========================================================
    * 用户消息已经被正式接受。
    *
    * 通知QML：
    * 现在可以创建
    *
    * User消息
    * AI回复占位
    *
    * 键盘输入和语音输入都统一走这里。
    * ========================================================
    */
    emit userMessageAccepted(
        cmd
    );


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
    * Fatigue Detection
    *
    * 疲劳检测属于本地车辆功能，
    * 不进入 RKLLM。
    *
    * 支持：
    *
    * 打开疲劳检测
    * 开启疲劳检测
    * 启动疲劳监测
    *
    * 关闭疲劳检测
    * 停止疲劳检测
    * 停用疲劳监测
    *
    * 疲劳检测状态
    * 疲劳检测开着吗
    * 疲劳检测是否开启
    * ========================================================
    */

    const bool isFatigueTarget =
        cmd.contains(
            QStringLiteral("疲劳检测")
        )
        ||
        cmd.contains(
            QStringLiteral("疲劳监测")
        );


    if (isFatigueTarget)
    {
        // ====================================================
        // 服务没有绑定
        // ====================================================

        if (!visionManager_)
        {
            finishLocalAnswer(
                QStringLiteral(
                    "视觉服务当前不可用。"
                )
            );

            return;
        }


        // ====================================================
        // 1. 查询疲劳检测状态
        //
        // 这一项必须放在“开启”判断前面。
        //
        // 因为：
        //
        // “疲劳检测是否开启”
        //
        // 里面也包含“开启”两个字，
        // 不能误认为是开启命令。
        // ====================================================

        const bool statusQuery =
            cmd.contains(
                QStringLiteral("状态")
            )
            ||
            cmd.contains(
                QStringLiteral("开着吗")
            )
            ||
            cmd.contains(
                QStringLiteral("开启了吗")
            )
            ||
            cmd.contains(
                QStringLiteral("打开了吗")
            )
            ||
            cmd.contains(
                QStringLiteral("是否开启")
            )
            ||
            cmd.contains(
                QStringLiteral("有没有开启")
            )
            ||
            cmd.contains(
                QStringLiteral("运行吗")
            );


        if (statusQuery)
        {
            if (
                visionManager_
                    ->isFatigueEnabled()
            )
            {
                finishLocalAnswer(
                    QStringLiteral(
                        "疲劳检测当前已开启。"
                    )
                );
            }
            else
            {
                finishLocalAnswer(
                    QStringLiteral(
                        "疲劳检测当前已关闭。"
                    )
                );
            }


            return;
        }


        // ====================================================
        // 2. 关闭疲劳检测
        // ====================================================

        const bool disableCommand =
            cmd.contains(
                QStringLiteral("关闭")
            )
            ||
            cmd.contains(
                QStringLiteral("停止")
            )
            ||
            cmd.contains(
                QStringLiteral("停用")
            )
            ||
            cmd.contains(
                QStringLiteral("禁用")
            );


        if (disableCommand)
        {
            // 已经关闭
            if (
                !visionManager_
                    ->isFatigueEnabled()
            )
            {
                finishLocalAnswer(
                    QStringLiteral(
                        "疲劳检测已经处于关闭状态。"
                    )
                );

                return;
            }


            visionManager_
                ->disableFatigue();


            finishLocalAnswer(
                QStringLiteral(
                    "疲劳检测已关闭。"
                )
            );


            return;
        }


        // ====================================================
        // 3. 开启疲劳检测
        // ====================================================

        const bool enableCommand =
            cmd.contains(
                QStringLiteral("打开")
            )
            ||
            cmd.contains(
                QStringLiteral("开启")
            )
            ||
            cmd.contains(
                QStringLiteral("启动")
            )
            ||
            cmd.contains(
                QStringLiteral("恢复")
            );


        if (enableCommand)
        {
            // 已经打开
            if (
                visionManager_
                    ->isFatigueEnabled()
            )
            {
                finishLocalAnswer(
                    QStringLiteral(
                        "疲劳检测已经处于开启状态。"
                    )
                );

                return;
            }


            const bool started =
                visionManager_
                    ->enableFatigue();


            if (started)
            {
                finishLocalAnswer(
                    QStringLiteral(
                        "疲劳检测已开启，正在重新建立驾驶员检测基准。"
                    )
                );
            }
            else
            {
                finishLocalAnswer(
                    QStringLiteral(
                        "疲劳检测启动失败，请检查摄像头和疲劳检测模型状态。"
                    )
                );
            }


            return;
        }
    }

    /*
    * ========================================================
    * Weather Page
    * ========================================================
    *
    * 控制天气页面本身。
    *
    * 必须放在 isWeatherQuestion(cmd) 前面，
    * 否则“打开天气”会被普通天气查询提前匹配。
    */
    if (
        cmd.contains(
            QStringLiteral("打开天气页面")
        )
        ||
        cmd.contains(
            QStringLiteral("打开天气界面")
        )
        ||
        cmd.contains(
            QStringLiteral("进入天气页面")
        )
        ||
        cmd.contains(
            QStringLiteral("进入天气界面")
        )
        ||
        cmd.contains(
            QStringLiteral("打开天气模块")
        )
        ||
        cmd == QStringLiteral("打开天气")
    )
    {
        QMetaObject::invokeMethod(
            this,

            [this]()
            {
                emit openWeatherPage();
            },

            Qt::QueuedConnection
        );


        finishLocalAnswer(
            QStringLiteral(
                "正在打开天气页面。"
            )
        );


        return;
    }

    /*
    * ========================================================
    * Weather
    * ========================================================
    */
    if (
        isWeatherQuestion(
            cmd
        )
    )
    {
        /*
        * 天气服务没绑定。
        */
        if (!weatherService_)
        {
            finishLocalAnswer(
                QStringLiteral(
                    "天气服务当前不可用。"
                )
            );

            return;
        }


        /*
        * 防止同时发起两个天气查询。
        */
        if (
            weatherPendingMode_
            !=
            WeatherPendingMode::None
        )
        {
            finishLocalAnswer(
                QStringLiteral(
                    "天气正在查询中，请稍候再试。"
                )
            );

            return;
        }


        /*
        * 看用户有没有指定城市。
        */
        const QString city =
            extractWeatherCity(
                cmd
            );


        /*
        * ====================================================
        * 指定城市
        *
        * 我要查询广州地区的天气
        * 广州天气怎么样
        * ====================================================
        */
        if (!city.isEmpty())
        {
            qDebug()
                << "[VehicleAssistant]"
                << "specific weather city:"
                << city;


            startWeatherCitySearch(
                city
            );


            return;
        }


        /*
        * ====================================================
        * 没有指定城市
        *
        * 今天天气怎么样
        * 当前天气如何
        * 今天会下雨吗
        *
        * 查询 WeatherService 当前城市。
        * ====================================================
        */

        qDebug()
            << "[VehicleAssistant]"
            << "current city weather:"
            << weatherService_->city();


        weatherPendingMode_ =
            WeatherPendingMode::CurrentCity;


        /*
        * 主动联网刷新，
        * 确保不是旧缓存。
        *
        * 如果 WeatherService 本身正在刷新，
        * refresh() 会忽略重复请求，
        * 我们继续等现有 weatherChanged 即可。
        */
        weatherService_
            ->refresh();


        return;
    }

   /*
    * ============================================================
    * Page Navigation
    * ============================================================
    *
    * 通用页面退出：
    *
    * 返回主页
    * 返回主界面
    * 回到主页
    * 关闭当前页面
    * ============================================================
    */

    if (
        cmd.contains(
            QStringLiteral("返回主页")
        )
        ||
        cmd.contains(
            QStringLiteral("回到主页")
        )
        ||
        cmd.contains(
            QStringLiteral("返回主界面")
        )
        ||
        cmd.contains(
            QStringLiteral("回到主界面")
        )
        ||
        cmd.contains(
            QStringLiteral("返回首页")
        )
        ||
        cmd.contains(
            QStringLiteral("关闭当前页面")
        )
        ||
        cmd.contains(
            QStringLiteral("退出当前页面")
        )
    )
    {
        emit returnToMainPage();


        finishLocalAnswer(
            QStringLiteral(
                "已返回主界面。"
            )
        );


        return;
    }

    /*
    * ============================================================
    * Camera Business
    * ============================================================
    *
    * 注意：
    *
    * 这里控制的是“相机业务”，
    * 不是 CameraService。
    *
    * CameraService 仍然作为底层全局视觉输入持续运行。
    * ============================================================
    */
    /*
    * ============================================================
    * Camera Device
    *
    * 底层摄像头服务
    * ============================================================
    */


    /*
    * ------------------------------------------------------------
    * 打开 / 开启摄像头
    *
    * 只启动 CameraService。
    * 不打开 Camera.qml。
    * ------------------------------------------------------------
    */

    if (
        cmd.contains(
            QStringLiteral("打开摄像头")
        )
        ||
        cmd.contains(
            QStringLiteral("开启摄像头")
        )
        ||
        cmd.contains(
            QStringLiteral("启动摄像头")
        )
    )
    {
        if (!visionManager_)
        {
            finishLocalAnswer(
                QStringLiteral(
                    "视觉服务当前不可用。"
                )
            );

            return;
        }


        const bool alreadyRunning =
            visionManager_
                ->isCameraRunning();


        if (
            !visionManager_
                ->openCameraDevice()
        )
        {
            finishLocalAnswer(
                QStringLiteral(
                    "摄像头启动失败。"
                )
            );

            return;
        }


        if (alreadyRunning)
        {
            finishLocalAnswer(
                QStringLiteral(
                    "摄像头已经处于开启状态。"
                )
            );
        }
        else
        {
            finishLocalAnswer(
                QStringLiteral(
                    "摄像头已开启。"
                )
            );
        }


        return;
    }

    /*
    * ------------------------------------------------------------
    * 关闭底层摄像头
    *
    * 这里必须处理上层依赖。
    * ------------------------------------------------------------
    */

    if (
        cmd.contains(
            QStringLiteral("关闭摄像头")
        )
        ||
        cmd.contains(
            QStringLiteral("停止摄像头")
        )
        ||
        cmd.contains(
            QStringLiteral("退出摄像头")
        )
    )
    {
        if (!visionManager_)
        {
            finishLocalAnswer(
                QStringLiteral(
                    "视觉服务当前不可用。"
                )
            );

            return;
        }


        const bool cameraBusinessWasActive =
            visionManager_
                ->isCameraBusinessActive();


        const auto result =
            visionManager_
                ->closeCameraDevice();


        switch (result)
        {
            case
            VisionServiceManager::CloseCameraResult::Closed:
            {
                if (cameraBusinessWasActive)
                {
                    finishLocalAnswer(
                        QStringLiteral(
                            "相机界面已关闭，摄像头已关闭。"
                        )
                    );
                }
                else
                {
                    finishLocalAnswer(
                        QStringLiteral(
                            "摄像头已关闭。"
                        )
                    );
                }

                break;
            }


            case
            VisionServiceManager::CloseCameraResult::AlreadyStopped:
            {
                if (cameraBusinessWasActive)
                {
                    finishLocalAnswer(
                        QStringLiteral(
                            "相机界面已关闭，摄像头已经处于关闭状态。"
                        )
                    );
                }
                else
                {
                    finishLocalAnswer(
                        QStringLiteral(
                            "摄像头已经处于关闭状态。"
                        )
                    );
                }

                break;
            }


            case
            VisionServiceManager::CloseCameraResult::BlockedByFatigue:
            {
                if (cameraBusinessWasActive)
                {
                    finishLocalAnswer(
                        QStringLiteral(
                            "相机界面已关闭，但疲劳检测正在使用摄像头，摄像头保持开启。"
                        )
                    );
                }
                else
                {
                    finishLocalAnswer(
                        QStringLiteral(
                            "疲劳检测正在使用摄像头，当前不能关闭摄像头。"
                        )
                    );
                }

                break;
            }
        }


        return;
    }
    /*
    * ============================================================
    * Camera Business
    * ============================================================
    */


    /*
    * ------------------------------------------------------------
    * 打开相机
    *
    * 相机业务依赖 CameraService。
    *
    * 如果底层摄像头已经关闭，
    * VisionServiceManager 自动重新开启。
    * ------------------------------------------------------------
    */

    if (
        cmd.contains(
            QStringLiteral("打开相机")
        )
        ||
        cmd.contains(
            QStringLiteral("开启相机")
        )
        ||
        cmd.contains(
            QStringLiteral("进入相机")
        )
    )
    {
        if (!visionManager_)
        {
            finishLocalAnswer(
                QStringLiteral(
                    "相机功能当前不可用。"
                )
            );

            return;
        }


        if (
            visionManager_
                ->openCameraBusiness()
        )
        {
            finishLocalAnswer(
                QStringLiteral(
                    "正在打开相机。"
                )
            );
        }
        else
        {
            finishLocalAnswer(
                QStringLiteral(
                    "相机启动失败，无法获取摄像头画面。"
                )
            );
        }


        return;
    }
    
    /*
    * ------------------------------------------------------------
    * 关闭相机业务
    *
    * 只关闭 Camera.qml / 预览。
    * 不关闭底层 CameraService。
    * ------------------------------------------------------------
    */

    if (
        cmd.contains(
            QStringLiteral("关闭相机")
        )
        ||
        cmd.contains(
            QStringLiteral("退出相机")
        )
    )
    {
        if (!visionManager_)
        {
            finishLocalAnswer(
                QStringLiteral(
                    "相机功能当前不可用。"
                )
            );

            return;
        }


        visionManager_
            ->closeCameraBusiness();


        finishLocalAnswer(
            QStringLiteral(
                "相机已关闭。"
            )
        );


        return;
    }
    /*
    * ------------------------------------------------------------
    * 拍照
    * ------------------------------------------------------------
    */

    if (
        cmd.contains(
            QStringLiteral("拍照")
        )
        ||
        cmd.contains(
            QStringLiteral("拍一张")
        )
        ||
        cmd.contains(
            QStringLiteral("拍张照片")
        )
        ||
        cmd.contains(
            QStringLiteral("拍一张照片")
        )
    )
    {
        if (!visionManager_)
        {
            finishLocalAnswer(
                QStringLiteral(
                    "相机功能当前不可用。"
                )
            );

            return;
        }


        if (
            visionManager_
                ->capturePhoto()
        )
        {
            finishLocalAnswer(
                QStringLiteral(
                    "已执行拍照。"
                )
            );
        }
        else
        {
            finishLocalAnswer(
                QStringLiteral(
                    "拍照失败，摄像头当前不可用。"
                )
            );
        }


        return;
    }

    /*
    * ============================================================
    * Gallery Business
    * ============================================================
    */


    /*
    * ------------------------------------------------------------
    * 关闭相册
    * ------------------------------------------------------------
    */

    if (
        cmd.contains(
            QStringLiteral("关闭相册")
        )
        ||
        cmd.contains(
            QStringLiteral("退出相册")
        )
        ||
        cmd.contains(
            QStringLiteral("关闭照片")
        )
        ||
        cmd.contains(
            QStringLiteral("退出照片")
        )
    )
    {
        emit closeGalleryPage();


        finishLocalAnswer(
            QStringLiteral(
                "已关闭相册。"
            )
        );


        return;
    }

    /*
    * ------------------------------------------------------------
    * 打开相册
    * ------------------------------------------------------------
    */

    if (
        cmd.contains(
            QStringLiteral("打开相册")
        )
        ||
        cmd.contains(
            QStringLiteral("查看相册")
        )
        ||
        cmd.contains(
            QStringLiteral("打开照片")
        )
        ||
        cmd.contains(
            QStringLiteral("查看照片")
        )
    )
    {
        emit openGalleryPage();


        finishLocalAnswer(
            QStringLiteral(
                "正在打开相册。"
            )
        );


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
                    * ========================================================
                    * TTS 分句缓存
                    * ========================================================
                    *
                    * RKLLM token 不一定刚好是一句话。
                    * 所以先不断累积。
                    *
                    * 遇到：
                    * 。！？；
                    *
                    * 就切出一句交给 TTS。
                    */
                    QString speechBuffer;


                    /*
                    * 把一句完整语音文本安全地发送回 Qt 主线程。
                    */
                    auto postSpeechChunk =
                        [this, currentGeneration](
                            const QString &text,
                            int pauseAfterMs
                        )
                        {
                            const QString chunk =
                                text.trimmed();

                            if (chunk.isEmpty())
                            {
                                return;
                            }


                            QMetaObject::invokeMethod(
                                this,

                                [
                                    this,
                                    currentGeneration,
                                    chunk,
                                    pauseAfterMs
                                ]()
                                {
                                    /*
                                    * 已经切换到其他 generation，
                                    * 旧语音不能再播。
                                    */
                                    if (
                                        generationId_.load()
                                        !=
                                        currentGeneration
                                    )
                                    {
                                        return;
                                    }


                                    /*
                                    * 用户已经说了“停止回答”。
                                    *
                                    * 未进入 TTS 的句子直接丢弃。
                                    */
                                    if (stopRequested_.load())
                                    {
                                        return;
                                    }


                                    qDebug()
                                        << "[VehicleAssistant]"
                                        << "speech chunk:"
                                        << chunk;


                                    emit answerSpeechChunkReady(
                                        chunk,
                                        pauseAfterMs
                                    );
                                },

                                Qt::QueuedConnection
                            );
                        };
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
                            currentGeneration,
                            &speechBuffer,
                            &postSpeechChunk
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


                            /*
                            * ========================================================
                            * 当前 token
                            * ========================================================
                            */
                            const QString qtoken =
                                QString::fromUtf8(
                                    token.data(),
                                    static_cast<int>(
                                        token.size()
                                    )
                                );


                            /*
                            * ========================================================
                            * TTS 分句缓存
                            * ========================================================
                            */
                            QString speechToken = qtoken;

                            // Preserve newlines until their context is known.
                            // A trailing single newline is held in speechBuffer
                            // because the next RKLLM token may turn it into a
                            // paragraph break ("\n\n").
                            speechToken.replace(
                                QStringLiteral("\r\n"),
                                QStringLiteral("\n"));
                            speechToken.replace('\r', '\n');

                            speechBuffer += speechToken;


                            /*
                            * 强分句符号。
                            *
                            * 模型只要输出这些符号，
                            * 就可以立即开始朗读这一句。
                            */
                            const QString hardDelimiters =
                                QStringLiteral("。！？；");


                            /*
                            * 弱分句符号。
                            *
                            * 当一句话太长时，
                            * 尽量在逗号等位置切开。
                            */
                            const QString softStops =
                                QStringLiteral(
                                    "，、,:："
                                );


                            /*
                            * 弱标点前至少保留 10 个字符。
                            *
                            * 这样逗号子句能够在上一段播放期间提前生成，
                            * 又不会把回答切成过短的两三个字。
                            */
                            constexpr int kEarlySoftStopMinChars =
                                10;


                            /*
                            * 最长大约 48 个字符。
                            *
                            * 防止模型几十个字都不输出任何标点，
                            * 导致 TTS 一直不开始讲话。
                            */
                            constexpr int kMaxSpeechChunkChars =
                                48;


                            const auto isProtectedSoftStop =
                                [&speechBuffer](int position)
                                {
                                    if (
                                        position < 0
                                        ||
                                        position >= speechBuffer.size()
                                    )
                                    {
                                        return false;
                                    }

                                    const QChar symbol =
                                        speechBuffer.at(position);

                                    const bool numericSeparator =
                                        position > 0
                                        &&
                                        position + 1 < speechBuffer.size()
                                        &&
                                        speechBuffer.at(position - 1).isDigit()
                                        &&
                                        speechBuffer.at(position + 1).isDigit()
                                        &&
                                        (symbol == ',' || symbol == ':');

                                    if (numericSeparator)
                                        return true;

                                    if (symbol == ':')
                                    {
                                        const QString prefix =
                                            speechBuffer
                                                .left(position)
                                                .toLower();

                                        if (
                                            prefix.endsWith(
                                                QStringLiteral("http"))
                                            ||
                                            prefix.endsWith(
                                                QStringLiteral("https"))
                                        )
                                        {
                                            return true;
                                        }
                                    }

                                    return false;
                                };


                            /*
                            * ========================================================
                            * 可能一个 token 里面包含两句话，
                            * 所以这里必须 while。
                            * ========================================================
                            */
                            while (!speechBuffer.isEmpty())
                            {
                                int cutPos = -1;
                                int pauseAfterMs = 0;
                                bool normalizedNewline = false;
                                bool pendingBoundaryLookahead = false;


                                /*
                                * ----------------------------------------------------
                                * 第一优先级：
                                * 找完整句子的结束符。
                                * ----------------------------------------------------
                                */
                                for (
                                    int i = 0;
                                    i < speechBuffer.size();
                                    ++i
                                )
                                {
                                    if (
                                        hardDelimiters.contains(
                                            speechBuffer.at(i)
                                        )
                                    )
                                    {
                                        // Hold a trailing terminator for one
                                        // token of lookahead. A following
                                        // blank line upgrades this boundary
                                        // from sentence (60 ms) to paragraph
                                        // (120 ms).
                                        if (i + 1 == speechBuffer.size())
                                        {
                                            pendingBoundaryLookahead = true;
                                            break;
                                        }

                                        if (speechBuffer.at(i + 1) == '\n')
                                            continue;

                                        cutPos =
                                            i + 1;

                                        pauseAfterMs = 60;

                                        break;
                                    }

                                    if (speechBuffer.at(i) == '\n')
                                    {
                                        int newlineEnd = i;
                                        while (
                                            newlineEnd < speechBuffer.size()
                                            &&
                                            speechBuffer.at(newlineEnd) == '\n'
                                        )
                                        {
                                            ++newlineEnd;
                                        }

                                        const int newlineCount =
                                            newlineEnd - i;

                                        // Wait for one character of lookahead
                                        // before classifying a single newline.
                                        if (
                                            newlineCount == 1
                                            &&
                                            newlineEnd == speechBuffer.size()
                                        )
                                        {
                                            pendingBoundaryLookahead = true;
                                            break;
                                        }

                                        int previous = i - 1;
                                        while (
                                            previous >= 0
                                            &&
                                            speechBuffer.at(previous).isSpace()
                                        )
                                        {
                                            --previous;
                                        }

                                        int next = newlineEnd;
                                        while (
                                            next < speechBuffer.size()
                                            &&
                                            speechBuffer.at(next).isSpace()
                                        )
                                        {
                                            ++next;
                                        }

                                        const QChar previousChar =
                                            previous >= 0
                                                ? speechBuffer.at(previous)
                                                : QChar();
                                        const QChar nextChar =
                                            next < speechBuffer.size()
                                                ? speechBuffer.at(next)
                                                : QChar();

                                        const bool paragraphBreak =
                                            newlineCount >= 2;
                                        const bool listBreak =
                                            next < speechBuffer.size()
                                            &&
                                            (nextChar == '-'
                                             || nextChar == '*'
                                             || nextChar == '#'
                                             || nextChar.isDigit());
                                        const bool afterStrongPunctuation =
                                            hardDelimiters.contains(previousChar);
                                        const bool afterWeakPunctuation =
                                            softStops.contains(previousChar);

                                        if (
                                            paragraphBreak
                                            ||
                                            listBreak
                                            ||
                                            afterStrongPunctuation
                                            ||
                                            afterWeakPunctuation
                                        )
                                        {
                                            cutPos = newlineEnd;
                                            pauseAfterMs =
                                                paragraphBreak ? 120
                                                : listBreak ? 70
                                                : afterStrongPunctuation ? 60
                                                : 25;
                                            break;
                                        }

                                        // Ordinary line wrapping is not a
                                        // spoken boundary. Chinese is joined
                                        // directly; ASCII words retain a space.
                                        const bool asciiWordWrap =
                                            previousChar.unicode() < 128
                                            &&
                                            nextChar.unicode() < 128
                                            &&
                                            previousChar.isLetterOrNumber()
                                            &&
                                            nextChar.isLetterOrNumber();

                                        speechBuffer.replace(
                                            i,
                                            newlineCount,
                                            asciiWordWrap
                                                ? QStringLiteral(" ")
                                                : QString());
                                        normalizedNewline = true;
                                        break;
                                    }
                                }

                                if (normalizedNewline)
                                    continue;


                                /*
                                * ----------------------------------------------------
                                * 第二优先级：
                                *
                                * 已经有足够长的逗号子句时立即提交。
                                *
                                * Matcha 内部可能再次按逗号拆成多次推理，
                                * 但旧实现要等这些推理全部结束才交给播放线程，
                                * 容易让上一句播放完以后队列断流。
                                * ----------------------------------------------------
                                */
                                if (
                                    cutPos < 0
                                    &&
                                    !pendingBoundaryLookahead
                                    &&
                                    speechBuffer.size()
                                        >=
                                        kEarlySoftStopMinChars
                                )
                                {
                                    for (
                                        int i =
                                            kEarlySoftStopMinChars - 1;
                                        i < speechBuffer.size();
                                        ++i
                                    )
                                    {
                                        if (
                                            !softStops.contains(
                                                speechBuffer.at(i)
                                            )
                                        )
                                        {
                                            continue;
                                        }

                                        // 1,000, 12:30 and https: are not
                                        // spoken phrase boundaries.
                                        if (isProtectedSoftStop(i))
                                            continue;

                                        // Hold a trailing weak stop for one
                                        // token. The following token may be a
                                        // newline/list marker whose pause rule
                                        // takes priority.
                                        if (i + 1 == speechBuffer.size())
                                        {
                                            pendingBoundaryLookahead = true;
                                            break;
                                        }

                                        cutPos = i + 1;
                                        pauseAfterMs = 25;
                                        break;
                                    }
                                }


                                /*
                                * ----------------------------------------------------
                                * 第三优先级：
                                *
                                * 如果超过 48 个字仍然没有句号，
                                * 尝试在最近的逗号位置切开。
                                * ----------------------------------------------------
                                */
                                if (
                                    cutPos < 0
                                    &&
                                    !pendingBoundaryLookahead
                                    &&
                                    speechBuffer.size()
                                        >=
                                        kMaxSpeechChunkChars
                                )
                                {
                                    const int limit =
                                        qMin(
                                            kMaxSpeechChunkChars,
                                            speechBuffer.size()
                                        );


                                    /*
                                    * 从后向前寻找逗号。
                                    *
                                    * 不要在太靠前的位置切，
                                    * 否则句子会过短。
                                    */
                                    for (
                                        int i = limit - 1;
                                        i >= 18;
                                        --i
                                    )
                                    {
                                        if (
                                            softStops.contains(
                                                speechBuffer.at(i)
                                            )
                                        )
                                        {
                                            if (isProtectedSoftStop(i))
                                                continue;

                                            cutPos =
                                                i + 1;

                                            pauseAfterMs = 25;

                                            break;
                                        }
                                    }


                                    /*
                                    * 连逗号都没有，
                                    * 那只能按长度强制切。
                                    */
                                    if (cutPos < 0)
                                    {
                                        cutPos =
                                            limit;

                                        pauseAfterMs = 0;
                                    }
                                }


                                /*
                                * 当前还不足一句。
                                *
                                * 继续等待 RKLLM 后面的 token。
                                */
                                if (cutPos < 0)
                                {
                                    break;
                                }


                                /*
                                * 取出一句。
                                */
                                const QString chunk =
                                    speechBuffer
                                        .left(
                                            cutPos
                                        )
                                        .trimmed();


                                /*
                                * 从缓存删除已经取出的部分。
                                */
                                speechBuffer.remove(
                                    0,
                                    cutPos
                                );


                                if (!chunk.isEmpty())
                                {
                                    postSpeechChunk(
                                        chunk,
                                        pauseAfterMs
                                    );
                                }
                            }


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
                    if (!wasStopped)
                    {
                        const QString tail =
                            speechBuffer.trimmed();


                        if (!tail.isEmpty())
                        {
                            int tailPauseMs = 0;
                            const QChar last = tail.back();
                            if (QStringLiteral("。！？；.!?;").contains(last))
                                tailPauseMs = 60;
                            else if (QStringLiteral("，、,:：").contains(last))
                                tailPauseMs = 25;

                            postSpeechChunk(
                                tail,
                                tailPauseMs
                            );
                        }
                    }
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
     * AI正在回答时不能切换会话。
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
     * 回收已经结束的旧LLM线程。
     */
    {
        std::lock_guard<std::mutex> lock(
            llmMutex_
        );

        joinFinishedThread();
    }


    const QString oldId =
        currentConversationId_;


    /*
     * 清除RKLLM历史上下文。
     */
    const bool resetOk =
        llm_.resetContext(true);


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
     * 新会话不能继续恢复旧历史。
     */
    restoreHistoryOnNextPrompt_ =
        false;

    restoredHistoryContext_.clear();


    /*
     * 只废弃一次旧generation。
     */
    generationId_.fetch_add(1);

    stopRequested_.store(false);


    /*
     * 保存旧会话。
     */
    conversationManager_.saveCurrent();


    /*
     * 只创建一次新ID。
     */
    currentConversationId_ =
        QUuid::createUuid()
            .toString(
                QUuid::WithoutBraces
            );


    /*
     * ConversationManager和VehicleAssistant
     * 使用同一个ID。
     */
    conversationManager_
        .startNewConversation(
            currentConversationId_
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

bool VehicleAssistant::isWeatherQuestion(
    const QString &text
) const
{
    const QString cmd =
        text.trimmed();


    if (
        cmd.contains(
            QStringLiteral("天气")
        )
    )
    {
        return true;
    }


    /*
     * 顺便支持一些正常表达。
     */
    if (
        cmd.contains(
            QStringLiteral("会下雨")
        )
        ||
        cmd.contains(
            QStringLiteral("有雨吗")
        )
        ||
        cmd.contains(
            QStringLiteral("降雨情况")
        )
    )
    {
        return true;
    }


    return false;
}

QString VehicleAssistant::extractWeatherCity(
    const QString &text
) const
{
    const QString cmd =
        text.trimmed();

        /*
     * ========================================================
     * 城市名称清洗
     * ========================================================
     *
     * 防止语音表达中的时间词进入城市名：
     *
     * 今天广州  -> 广州
     * 广州今天  -> 广州
     * 现在深圳  -> 深圳
     * 北京现在  -> 北京
     */
    const auto cleanCity =
        [](QString city)
        {
            city =
                city.trimmed();

            const QStringList timeWords = {
                QStringLiteral("今天"),
                QStringLiteral("今日"),
                QStringLiteral("现在"),
                QStringLiteral("当前")
            };


            bool changed = true;

            while (changed)
            {
                changed = false;

                for (const QString &word : timeWords)
                {
                    if (city.startsWith(word))
                    {
                        city.remove(
                            0,
                            word.length()
                        );

                        city =
                            city.trimmed();

                        changed = true;
                    }


                    if (city.endsWith(word))
                    {
                        city.chop(
                            word.length()
                        );

                        city =
                            city.trimmed();

                        changed = true;
                    }
                }
            }


            return city;
        };
    /*
     * ========================================================
     * 模式1
     *
     * 我要查询广州地区的天气
     * 查询广州天气
     * 查一下深圳的天气
     * 帮我看看上海天气
     * ========================================================
     */
    {
        const QRegularExpression regex(
            QStringLiteral(
                "(?:我要|我想|请|请帮我|帮我)?"
                "\\s*"
                "(?:查询|查一下|查查|查|看看|看一下)"
                "\\s*"
                "([\\p{Han}A-Za-z·\\-\\s]{2,30}?)"
                "(?:地区)?"
                "(?:的)?"
                "(?:今天天气|天气)"
            )
        );


        const QRegularExpressionMatch match =
            regex.match(
                cmd
            );


        if (match.hasMatch())
        {
            QString city =
                cleanCity(
                    match.captured(1)
                );


            if (!city.isEmpty())
            {
                return city;
            }
        }
    }


    /*
     * ========================================================
     * 模式2
     *
     * 广州天气怎么样
     * 深圳今天天气怎么样
     * 北京地区的天气
     * ========================================================
     */
    {
        const QRegularExpression regex(
            QStringLiteral(
                "^\\s*"
                "([\\p{Han}A-Za-z·\\-\\s]{2,30}?)"
                "(?:地区)?"
                "(?:的)?"
                "(?:今天天气|天气)"
                "(?:怎么样|如何|情况)?"
                "[？?。！!]*"
                "\\s*$"
            )
        );


        const QRegularExpressionMatch match =
            regex.match(
                cmd
            );


        if (match.hasMatch())
        {
            QString city =
                cleanCity(
                    match.captured(1)
                );


            /*
             * 防止：
             *
             * 今天天气怎么样
             *
             * 被错误解析成：
             *
             * city = 今天
             */
            const QStringList currentWords = {
                QStringLiteral("今天"),
                QStringLiteral("今日"),
                QStringLiteral("现在"),
                QStringLiteral("当前"),
                QStringLiteral("本地"),
                QStringLiteral("这里"),
                QStringLiteral("这边")
            };


            if (
                !city.isEmpty()
                &&
                !currentWords.contains(
                    city
                )
            )
            {
                return city;
            }
        }
    }


    /*
     * ========================================================
     * 模式3
     *
     * 我想知道广州的天气
     * 请问上海天气怎么样
     * ========================================================
     */
    {
        const QRegularExpression regex(
            QStringLiteral(
                "(?:我想知道|请问)"
                "\\s*"
                "([\\p{Han}A-Za-z·\\-\\s]{2,30}?)"
                "(?:地区)?"
                "(?:的)?"
                "(?:今天天气|天气)"
            )
        );


        const QRegularExpressionMatch match =
            regex.match(
                cmd
            );


        if (match.hasMatch())
        {
            QString city =
                cleanCity(
                    match.captured(1)
                );

            if (!city.isEmpty())
            {
                return city;
            }
        }
    }

    /*
    * ========================================================
    * 模式4
    *
    * 更自然的语音表达：
    *
    * 告诉我广州的天气
    * 广州今天什么天气
    * 广州现在天气怎么样
    * ========================================================
    */
    {
        const QRegularExpression regex(
            QStringLiteral(
                "^(?:告诉我|跟我说一下|说一下)?"
                "\\s*"
                "([\\p{Han}A-Za-z·\\-\\s]{2,30}?)"
                "(?:地区)?"
                "(?:的)?"
                "(?:今天|今日|现在|当前)?"
                "\\s*"
                "(?:是什么天气|什么天气|天气)"
                "(?:怎么样|如何|情况)?"
                "[？?。！!]*"
                "\\s*$"
            )
        );


        const QRegularExpressionMatch match =
            regex.match(
                cmd
            );


        if (match.hasMatch())
        {
            QString city =
                cleanCity(
                    match.captured(1)
                );


            /*
            * 防止：
            *
            * 今天天气怎么样
            * 当前天气如何
            *
            * 被误认为城市。
            */
            const QStringList invalidCities = {
                QStringLiteral("今天"),
                QStringLiteral("今日"),
                QStringLiteral("现在"),
                QStringLiteral("当前"),
                QStringLiteral("本地"),
                QStringLiteral("这里"),
                QStringLiteral("这边")
            };


            if (
                !city.isEmpty()
                &&
                !invalidCities.contains(
                    city
                )
            )
            {
                return city;
            }
        }
    }
    /*
     * 没有指定城市。
     *
     * 那么就是 WeatherService 当前城市。
     */
    return QString();
}

QString VehicleAssistant::buildWeatherAnswer() const
{
    if (!weatherService_)
    {
        return QStringLiteral(
            "天气服务当前不可用。"
        );
    }


    const QString city =
        weatherService_->city();


    const double temperature =
        weatherService_->temperature();


    const double apparent =
        weatherService_->apparentTemperature();


    const int humidity =
        weatherService_->humidity();


    const double windSpeed =
        weatherService_->windSpeed();


    const QString weather =
        weatherService_->weatherText();


    /*
     * 今日最高 / 最低 / 降雨概率。
     */
    QString maxTemperature =
        QStringLiteral("--");


    QString minTemperature =
        QStringLiteral("--");


    QString rainProbability =
        QStringLiteral("--");


    const QVariantList forecast =
        weatherService_->dailyForecast();


    if (!forecast.isEmpty())
    {
        const QVariantMap today =
            forecast
                .first()
                .toMap();


        maxTemperature =
            QString::number(
                today.value(
                    QStringLiteral(
                        "maxTemperature"
                    )
                ).toDouble(),
                'f',
                1
            );


        minTemperature =
            QString::number(
                today.value(
                    QStringLiteral(
                        "minTemperature"
                    )
                ).toDouble(),
                'f',
                1
            );


        rainProbability =
            QString::number(
                today.value(
                    QStringLiteral(
                        "rainProbability"
                    )
                ).toInt()
            );
    }


    return QStringLiteral(
        "%1今天天气为%2，"
        "当前温度%3℃，"
        "体感温度%4℃；"
        "今天最高%5℃，最低%6℃；"
        "湿度%7％，"
        "风速%8 km/h，"
        "降雨概率%9％。"
    )
    .arg(
        city,
        weather,
        QString::number(
            temperature,
            'f',
            1
        ),
        QString::number(
            apparent,
            'f',
            1
        ),
        maxTemperature,
        minTemperature,
        QString::number(
            humidity
        ),
        QString::number(
            windSpeed,
            'f',
            1
        ),
        rainProbability
    );
}

void VehicleAssistant::finishLocalAnswer(
    const QString &answer
)
{
    /*
     * 保存 AI 回答。
     */
    conversationManager_
        .appendAssistantMessage(
            answer
        );


    /*
     * 写入会话 JSON。
     */
    conversationManager_
        .saveCurrent();


    /*
     * QML已经创建了一个 AI 空消息，
     * answerToken 会把天气答案填进去。
     */
    emit answerToken(
        answer
    );
    /*
    * 本地回答同样可以交给 TTS。
    */
    if (!answer.trimmed().isEmpty())
    {
        emit answerSpeechChunkReady(
            answer.trimmed(),
            60
        );
    }


    emit answerFinished();
}

void VehicleAssistant::startWeatherCitySearch(
    const QString &city
)
{
    if (!weatherService_)
    {
        finishLocalAnswer(
            QStringLiteral(
                "天气服务当前不可用。"
            )
        );

        return;
    }


    pendingWeatherCity_ =
        city;


    /*
     * WeatherService 如果此时还在执行
     * 上一次天气请求，
     * 不要马上切城市。
     *
     * 否则可能发生：
     *
     * 深圳请求还没回来
     * ↓
     * city_ 被改成广州
     * ↓
     * 深圳数据回来却被标成广州
     */
    if (
        weatherService_->loading()
    )
    {
        qDebug()
            << "[VehicleAssistant]"
            << "weather request busy,"
            << "queue city:"
            << city;


        weatherPendingMode_ =
            WeatherPendingMode::CityQueued;


        return;
    }


    weatherPendingMode_ =
        WeatherPendingMode::CityLocating;


    qDebug()
        << "[VehicleAssistant]"
        << "search weather city:"
        << city;


    weatherService_
        ->searchCity(
            city
        );
}

void VehicleAssistant::setVisionServiceManager(
    VisionServiceManager *manager
)
{
    if (visionManager_ == manager)
    {
        return;
    }


    /*
     * 清掉旧 Manager 与 VehicleAssistant 的连接。
     */
    if (visionManager_)
    {
        QObject::disconnect(
            visionManager_,
            nullptr,
            this,
            nullptr
        );
    }


    visionManager_ =
        manager;


    if (!visionManager_)
    {
        return;
    }


    /*
     * ========================================================
     * Vision Manager 请求打开相机页面
     * ========================================================
     */
    QObject::connect(
        visionManager_,
        &VisionServiceManager::openCameraPageRequested,

        this,

        [this]()
        {
            emit openCameraPage();
        }
    );


    /*
     * ========================================================
     * Vision Manager 请求关闭相机页面
     * ========================================================
     */
    QObject::connect(
        visionManager_,
        &VisionServiceManager::closeCameraPageRequested,

        this,

        [this]()
        {
            emit closeCameraPage();
        }
    );


    qDebug()
        << "[VehicleAssistant]"
        << "VisionServiceManager connected";
}
