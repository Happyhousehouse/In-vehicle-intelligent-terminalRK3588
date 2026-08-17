#pragma once

#include <QObject>

#include <string>


class CameraService;
class CameraController;
class FatigueService;


class VisionServiceManager : public QObject
{
    Q_OBJECT

public:

    /*
     * 关闭底层摄像头时的结果。
     */
    enum class CloseCameraResult
    {
        Closed = 0,

        /*
         * 原本就没有运行。
         */
        AlreadyStopped,

        /*
         * 疲劳检测仍然依赖摄像头，
         * 因此拒绝真正关闭 CameraService。
         */
        BlockedByFatigue
    };


public:

    explicit VisionServiceManager(
        CameraService *cameraService,
        CameraController *cameraController,
        FatigueService *fatigueService,
        QObject *parent = nullptr
    );


    /*
     * 设置底层摄像头启动参数。
     */
    void configureCamera(
        const std::string &device,
        int width,
        int height
    );


    // ============================================================
    // 底层 CameraService
    // ============================================================

    /*
     * 单独开启底层摄像头。
     *
     * 不打开 Camera.qml。
     */
    bool openCameraDevice();


    /*
     * 关闭底层摄像头。
     *
     * 会先处理相机业务依赖，
     * 然后检查疲劳检测依赖。
     */
    CloseCameraResult closeCameraDevice();


    /*
     * 保证底层摄像头正在运行。
     *
     * 上层业务启动前统一调用。
     */
    bool ensureCameraRunning();


    bool isCameraRunning() const;


    // ============================================================
    // 相机业务
    // ============================================================

    /*
     * 打开相机业务。
     *
     * 如果 CameraService 已关闭，
     * 会自动先启动底层摄像头。
     */
    bool openCameraBusiness();


    /*
     * 关闭相机业务。
     *
     * 不关闭 CameraService。
     */
    void closeCameraBusiness();


    bool isCameraBusinessActive() const;


    /*
     * 拍照。
     *
     * 即使当前没有打开 Camera.qml，
     * 也会自动保证 CameraService 可用。
     */
    bool capturePhoto();


    // ============================================================
    // 疲劳检测业务
    // ============================================================

    /*
     * 开启疲劳检测。
     *
     * 自动保证 CameraService 可用。
     */
    bool enableFatigue();


    /*
     * 只关闭疲劳业务。
     *
     * 不关闭 CameraService。
     */
    void disableFatigue();


    bool isFatigueEnabled() const;


signals:

    /*
     * 这里不直接操作 QML。
     *
     * 管理器只发出页面请求，
     * 后面由 VehicleAssistant / Main.qml 接。
     */
    void openCameraPageRequested();

    void closeCameraPageRequested();


private:

    CameraService *cameraService_ =
        nullptr;

    CameraController *cameraController_ =
        nullptr;

    FatigueService *fatigueService_ =
        nullptr;


    std::string cameraDevice_ =
        "/dev/video11";

    int cameraWidth_ =
        1920;

    int cameraHeight_ =
        1080;
};