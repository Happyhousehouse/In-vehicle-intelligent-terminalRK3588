#include "VisionServiceManager.h"

#include "../camera/CameraService.h"
#include "../controller/CameraController.h"
#include "../fatigue/FatigueService.h"

#include <QDebug>


VisionServiceManager::VisionServiceManager(
    CameraService *cameraService,
    CameraController *cameraController,
    FatigueService *fatigueService,
    QObject *parent
)
    :
    QObject(parent),
    cameraService_(cameraService),
    cameraController_(cameraController),
    fatigueService_(fatigueService)
{
}


void VisionServiceManager::configureCamera(
    const std::string &device,
    int width,
    int height
)
{
    cameraDevice_ =
        device;

    cameraWidth_ =
        width;

    cameraHeight_ =
        height;
}


// ============================================================
// CameraService
// ============================================================

bool VisionServiceManager::isCameraRunning() const
{
    return
        cameraService_
        &&
        cameraService_->isRunning();
}


bool VisionServiceManager::ensureCameraRunning()
{
    if (!cameraService_)
    {
        qWarning()
            << "[VisionServiceManager]"
            << "CameraService is null";

        return false;
    }


    if (cameraService_->isRunning())
    {
        return true;
    }


    qDebug()
        << "[VisionServiceManager]"
        << "CameraService is OFF,"
        << "starting dependency";


    const bool ok =
        cameraService_->start(
            cameraDevice_,
            cameraWidth_,
            cameraHeight_
        );


    if (!ok)
    {
        qWarning()
            << "[VisionServiceManager]"
            << "failed to start CameraService";

        return false;
    }


    qDebug()
        << "[VisionServiceManager]"
        << "CameraService dependency ready";


    return true;
}


bool VisionServiceManager::openCameraDevice()
{
    /*
     * “打开摄像头”
     *
     * 只保证底层 CameraService ON。
     *
     * 不打开相机页面。
     */
    return ensureCameraRunning();
}

VisionServiceManager::CloseCameraResult
VisionServiceManager::closeCameraDevice()
{
    /*
     * ========================================================
     * 1. 先处理相机业务依赖
     * ========================================================
     *
     * 无论 CameraService 当前是不是 ON，
     * 只要相机页面/预览还开着，都先关闭。
     */
    if (
        cameraController_
        &&
        cameraController_->isPreviewActive()
    )
    {
        qDebug()
            << "[VisionServiceManager]"
            << "closing camera business first";


        cameraController_
            ->stopCamera();


        emit closeCameraPageRequested();
    }


    /*
     * ========================================================
     * 2. 底层本来就没运行
     * ========================================================
     */
    if (
        !cameraService_
        ||
        !cameraService_->isRunning()
    )
    {
        qDebug()
            << "[VisionServiceManager]"
            << "CameraService already stopped";


        return
            CloseCameraResult::AlreadyStopped;
    }


    /*
     * ========================================================
     * 3. 检查疲劳检测依赖
     * ========================================================
     *
     * 疲劳检测属于安全业务，
     * “关闭摄像头”不能偷偷把它一起关掉。
     */
    if (
        fatigueService_
        &&
        fatigueService_->isRunning()
    )
    {
        qWarning()
            << "[VisionServiceManager]"
            << "cannot stop CameraService:"
            << "FatigueService is using camera";


        return
            CloseCameraResult::BlockedByFatigue;
    }


    /*
     * ========================================================
     * 4. 已经不存在上层依赖，可以真正关闭底层
     * ========================================================
     */
    cameraService_->stop();


    qDebug()
        << "[VisionServiceManager]"
        << "CameraService stopped";


    return
        CloseCameraResult::Closed;
}
// ============================================================
// Camera business
// ============================================================

bool VisionServiceManager::openCameraBusiness()
{
    /*
     * 打开相机之前，
     * 自动补齐 CameraService 依赖。
     */
    if (!ensureCameraRunning())
    {
        return false;
    }


    /*
     * 页面加载以后 Camera.qml 本身会调用：
     *
     * cameraController.startCamera()
     *
     * 所以这里不用重复启动 Timer。
     */
    emit openCameraPageRequested();


    qDebug()
        << "[VisionServiceManager]"
        << "camera business open requested";


    return true;
}


void VisionServiceManager::closeCameraBusiness()
{
    /*
     * 只关闭相机业务。
     *
     * CameraService 不动。
     */

    if (cameraController_)
    {
        cameraController_
            ->stopCamera();
    }


    emit closeCameraPageRequested();


    qDebug()
        << "[VisionServiceManager]"
        << "camera business closed";
}


bool VisionServiceManager::isCameraBusinessActive() const
{
    return
        cameraController_
        &&
        cameraController_->isPreviewActive();
}

bool VisionServiceManager::capturePhoto()
{
    if (!cameraController_)
    {
        qWarning()
            << "[VisionServiceManager]"
            << "CameraController is null";

        return false;
    }


    /*
     * 相机页面可以不开，
     * 但底层 CameraService 必须运行。
     */
    if (!ensureCameraRunning())
    {
        return false;
    }


    cameraController_
        ->capturePhoto();


    return true;
}

// ============================================================
// Fatigue business
// ============================================================

bool VisionServiceManager::enableFatigue()
{
    if (!fatigueService_)
    {
        qWarning()
            << "[VisionServiceManager]"
            << "FatigueService is null";

        return false;
    }


    /*
     * 疲劳检测依赖 CameraService。
     */
    if (!ensureCameraRunning())
    {
        return false;
    }


    const bool ok =
        fatigueService_
            ->setMonitoringEnabled(
                true
            );


    if (!ok)
    {
        qWarning()
            << "[VisionServiceManager]"
            << "failed to enable FatigueService";

        return false;
    }


    qDebug()
        << "[VisionServiceManager]"
        << "FatigueService enabled";


    return true;
}


void VisionServiceManager::disableFatigue()
{
    if (!fatigueService_)
    {
        return;
    }


    fatigueService_
        ->setMonitoringEnabled(
            false
        );


    /*
     * 注意：
     *
     * 关闭疲劳检测以后，
     * 不自动 CameraService.stop()。
     */
    qDebug()
        << "[VisionServiceManager]"
        << "FatigueService disabled";
}


bool VisionServiceManager::isFatigueEnabled() const
{
    return
        fatigueService_
        &&
        fatigueService_->isRunning();
}