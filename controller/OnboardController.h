#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantMap>

class OnboardController : public QObject
{
    Q_OBJECT

    /*
     * 提供给 QML 的真实键盘显示状态。
     *
     * Main.qml 中可以直接使用：
     *
     * onboardController.keyboardVisible
     */
    Q_PROPERTY(
        bool keyboardVisible
        READ keyboardVisible
        NOTIFY keyboardVisibleChanged
    )

public:
    explicit OnboardController(QObject *parent = nullptr);

    bool keyboardVisible() const;

    /*
     * 显示 Onboard。
     */
    Q_INVOKABLE void showKeyboard();

    /*
     * 隐藏 Onboard，但不结束进程。
     */
    Q_INVOKABLE void hideKeyboard();

    /*
     * 完全结束 Onboard。
     */
    Q_INVOKABLE void quitKeyboard();

    /*
     * 切换 Fcitx5 中英文状态。
     */
    Q_INVOKABLE void toggleInputMethod();

signals:
    /*
     * 键盘真实可见状态改变。
     */
    void keyboardVisibleChanged();

private slots:
    /*
     * 接收 Onboard 的 D-Bus 属性变化信号。
     *
     * 用户点击 Onboard 自己的隐藏按钮时，
     * 也会进入这里。
     */
    void onPropertiesChanged(
        const QString &interfaceName,
        const QVariantMap &changedProperties,
        const QStringList &invalidatedProperties
    );

private:
    /*
     * 检查 Onboard 进程。
     */
    bool isOnboardRunning() const;

    /*
     * 检查 Onboard D-Bus 服务是否已经注册。
     */
    bool isOnboardServiceAvailable() const;

    /*
     * 调用 Onboard 的 Show、Hide 方法。
     */
    bool callOnboardMethod(const QString &method);

    /*
     * 修改真实可见状态，并向 QML 发送通知。
     */
    void setKeyboardVisible(bool visible);

private:
    /*
     * Onboard 启动后等待 D-Bus 服务注册。
     */
    QTimer m_startTimer;

    bool m_starting = false;
    bool m_wantVisible = false;
    bool m_keyboardVisible = false;

    int m_retryCount = 0;
};
