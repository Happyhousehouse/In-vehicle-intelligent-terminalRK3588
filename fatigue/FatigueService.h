#pragma once

#include <QObject>
#include <QString>

#include <atomic>
#include <thread>

#include "../camera/CameraService.h"

#include "FatigueVision.h"
#include "FatigueBaselineManager.h"
#include "FatigueEventTracker.h"
#include "FatigueDecision.h"


// ============================================================
// FatigueService
//
// 独立疲劳检测服务。
//
// CameraService:
//     只作为共享图像源。
//
// FatigueService:
//     自己负责疲劳检测线程、Baseline、EventTracker、
//     WindowAnalyzer、FatigueDecision。
//
// UI 可以独立开启 / 关闭疲劳检测。
// ============================================================

class FatigueService : public QObject
{
    Q_OBJECT


    // --------------------------------------------------------
    // 是否正在进行疲劳检测
    // --------------------------------------------------------

    Q_PROPERTY(
        bool monitoringEnabled
        READ isRunning
        NOTIFY monitoringEnabledChanged
    )


    // --------------------------------------------------------
    // 当前最终稳定疲劳等级
    //
    // UNKNOWN
    // NORMAL
    // FATIGUE
    // DEEP_FATIGUE
    // --------------------------------------------------------

    Q_PROPERTY(
        QString fatigueLevel
        READ fatigueLevel
        NOTIFY fatigueLevelChanged
    )
    // --------------------------------------------------------
    // 当前是否存在即时闭眼危险警告
    // --------------------------------------------------------

    Q_PROPERTY(
        bool eyeWarningActive
        READ eyeWarningActive
        NOTIFY eyeWarningActiveChanged
    )

public:

    explicit FatigueService(
        CameraService* camera,
        QObject* parent = nullptr
    );


    ~FatigueService() override;


    // 初始化 RKNN
    bool initialize();


    // 启动疲劳检测线程
    bool start();


    // 停止疲劳检测
    void stop();


    bool isRunning() const;


    // QML读取当前疲劳等级
    QString fatigueLevel() const;

    bool eyeWarningActive() const;
    // QML独立控制疲劳检测
    Q_INVOKABLE bool setMonitoringEnabled(
        bool enabled
    );


signals:

    // 疲劳检测开启/关闭发生变化
    void monitoringEnabledChanged();

    void eyeWarningActiveChanged();
    // 最终疲劳等级发生变化
    void fatigueLevelChanged();


private:

    void processLoop();


    // 更新供UI读取的最终疲劳等级
    void setFatigueLevel(
        FatigueLevel level
    );

    void setEyeWarningActive(
        bool active
    );
private:

    // --------------------------------------------------------
    // 不拥有 CameraService。
    //
    // CameraService 由 main.cpp 创建，
    // 是整个程序共享的图像源。
    // --------------------------------------------------------

    CameraService* camera_ = nullptr;


    FatigueVision vision_;

    FatigueBaselineManager baseline_;

    FatigueEventTracker eventTracker_;


    std::thread workerThread_;


    std::atomic<bool> running_{false};


    // FatigueLevel 底层是 enum，
    // 使用 atomic<int> 避免 worker 与 UI 线程的数据竞争。
    std::atomic<int> fatigueLevelCode_{
        static_cast<int>(
            FatigueLevel::UNKNOWN
        )
    };
    std::atomic<bool> eyeWarningActive_{
        false
    };

    bool initialized_ = false;
};