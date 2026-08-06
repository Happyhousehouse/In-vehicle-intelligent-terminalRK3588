#include "OnboardController.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusVariant>

#include <QDebug>
#include <QProcess>
#include <QStringList>

namespace
{
const QString kOnboardService =
    QStringLiteral("org.onboard.Onboard");

const QString kOnboardPath =
    QStringLiteral("/org/onboard/Onboard/Keyboard");

const QString kOnboardInterface =
    QStringLiteral("org.onboard.Onboard.Keyboard");

const QString kPropertiesInterface =
    QStringLiteral("org.freedesktop.DBus.Properties");
}

OnboardController::OnboardController(QObject *parent)
    : QObject(parent)
{
    /*
     * 监听 Onboard 的真实显示/隐藏状态。
     *
     * service 参数使用空字符串，
     * 这样即使 Onboard 是在 VehicleTerminal 启动之后
     * 才注册 D-Bus 服务，也能接收属性变化信号。
     */
    const bool connected =
        QDBusConnection::sessionBus().connect(
            QString(),
            kOnboardPath,
            kPropertiesInterface,
            QStringLiteral("PropertiesChanged"),
            this,
            SLOT(onPropertiesChanged(
                QString,
                QVariantMap,
                QStringList
            ))
        );

    qDebug()
        << "[OnboardController]"
        << "PropertiesChanged connected:"
        << connected;

    /*
     * Onboard 启动之后不一定马上注册 D-Bus 服务，
     * 因此每隔 250ms 检查一次。
     */
    m_startTimer.setSingleShot(true);
    m_startTimer.setInterval(250);

    connect(
        &m_startTimer,
        &QTimer::timeout,
        this,
        [this]()
        {
            qDebug()
                << "[OnboardController]"
                << "等待 Onboard D-Bus，次数:"
                << m_retryCount;

            /*
             * D-Bus 服务已经可用。
             */
            if (isOnboardServiceAvailable())
            {
                const QString method =
                    m_wantVisible
                    ? QStringLiteral("Show")
                    : QStringLiteral("Hide");

                if (callOnboardMethod(method))
                {
                    setKeyboardVisible(
                        m_wantVisible
                    );

                    m_starting = false;
                    m_retryCount = 0;

                    return;
                }
            }

            /*
             * 最多等待约 3 秒。
             */
            ++m_retryCount;

            if (m_retryCount <= 12)
            {
                m_startTimer.start();
                return;
            }

            qWarning()
                << "[OnboardController]"
                << "Onboard D-Bus 启动超时";

            m_starting = false;
            m_retryCount = 0;

            setKeyboardVisible(false);
        }
    );
}

bool OnboardController::keyboardVisible() const
{
    return m_keyboardVisible;
}

void OnboardController::setKeyboardVisible(
    bool visible
)
{
    if (m_keyboardVisible == visible)
        return;

    m_keyboardVisible = visible;

    qDebug()
        << "[OnboardController]"
        << "keyboardVisible:"
        << m_keyboardVisible;

    emit keyboardVisibleChanged();
}

bool OnboardController::isOnboardRunning() const
{
    QProcess process;

    process.start(
        QStringLiteral("/usr/bin/pgrep"),
        QStringList()
            << QStringLiteral("-x")
            << QStringLiteral("onboard")
    );

    if (!process.waitForStarted(500))
        return false;

    if (!process.waitForFinished(1000))
    {
        process.kill();
        process.waitForFinished();

        return false;
    }

    return
        process.exitStatus() ==
            QProcess::NormalExit
        &&
        process.exitCode() == 0;
}

bool OnboardController::isOnboardServiceAvailable() const
{
    QDBusConnectionInterface *interface =
        QDBusConnection::sessionBus().interface();

    if (!interface)
        return false;

    QDBusReply<bool> reply =
        interface->isServiceRegistered(
            kOnboardService
        );

    return reply.isValid() &&
           reply.value();
}

bool OnboardController::callOnboardMethod(
    const QString &method
)
{
    if (!isOnboardServiceAvailable())
    {
        qWarning()
            << "[OnboardController]"
            << "Onboard D-Bus 服务不可用";

        return false;
    }

    QDBusMessage message =
        QDBusMessage::createMethodCall(
            kOnboardService,
            kOnboardPath,
            kOnboardInterface,
            method
        );

    QDBusMessage reply =
        QDBusConnection::sessionBus().call(
            message,
            QDBus::Block,
            1000
        );

    if (reply.type() ==
        QDBusMessage::ErrorMessage)
    {
        qWarning()
            << "[OnboardController]"
            << "调用失败:"
            << method
            << reply.errorName()
            << reply.errorMessage();

        return false;
    }

    qDebug()
        << "[OnboardController]"
        << "调用成功:"
        << method;

    return true;
}

void OnboardController::showKeyboard()
{
    qDebug()
        << "[OnboardController]"
        << "showKeyboard()";

    m_wantVisible = true;

    /*
     * Onboard 服务已经可用，直接显示。
     */
    if (isOnboardServiceAvailable())
    {
        if (callOnboardMethod(
                QStringLiteral("Show")))
        {
            /*
             * 先立即通知 QML 上移。
             * 后续 D-Bus 信号会继续校正真实状态。
             */
            setKeyboardVisible(true);
        }

        return;
    }

    /*
     * 已在等待启动，不重复启动。
     */
    if (m_starting)
        return;

    /*
     * 进程不存在时才启动。
     */
    if (!isOnboardRunning())
    {
        const bool started =
            QProcess::startDetached(
                QStringLiteral("/usr/bin/onboard"),
                QStringList()
            );

        qDebug()
            << "[OnboardController]"
            << "启动 Onboard:"
            << started;

        if (!started)
        {
            setKeyboardVisible(false);
            return;
        }
    }

    /*
     * Onboard 进程可能已存在，
     * 但 D-Bus 服务还没准备好。
     */
    m_starting = true;
    m_retryCount = 0;

    m_startTimer.start();
}

void OnboardController::hideKeyboard()
{
    qDebug()
        << "[OnboardController]"
        << "hideKeyboard()";

    m_wantVisible = false;

    /*
     * 先立即通知 QML 下移。
     */
    setKeyboardVisible(false);

    if (isOnboardServiceAvailable())
    {
        callOnboardMethod(
            QStringLiteral("Hide")
        );

        return;
    }

    /*
     * 如果 Onboard 正在启动，
     * 不停止定时器。
     *
     * 服务注册完成后，定时器会根据
     * m_wantVisible == false 调用 Hide。
     */
}

void OnboardController::quitKeyboard()
{
    qDebug()
        << "[OnboardController]"
        << "quitKeyboard()";

    m_wantVisible = false;
    m_starting = false;
    m_retryCount = 0;

    m_startTimer.stop();

    setKeyboardVisible(false);

    if (isOnboardServiceAvailable())
    {
        callOnboardMethod(
            QStringLiteral("Hide")
        );
    }

    QProcess process;

    process.start(
        QStringLiteral("/usr/bin/pkill"),
        QStringList()
            << QStringLiteral("-x")
            << QStringLiteral("onboard")
    );

    process.waitForFinished(1000);
}

void OnboardController::toggleInputMethod()
{
    qDebug()
        << "[OnboardController]"
        << "切换 Fcitx5 中英文";

    const bool started =
        QProcess::startDetached(
            QStringLiteral(
                "/usr/bin/fcitx5-remote"
            ),
            QStringList()
                << QStringLiteral("-t")
        );

    if (!started)
    {
        qWarning()
            << "[OnboardController]"
            << "fcitx5-remote 启动失败";
    }
}

void OnboardController::onPropertiesChanged(
    const QString &interfaceName,
    const QVariantMap &changedProperties,
    const QStringList &invalidatedProperties
)
{
    Q_UNUSED(invalidatedProperties)

    /*
     * 只处理 Onboard 键盘接口。
     */
    if (interfaceName != kOnboardInterface)
        return;

    if (!changedProperties.contains(
            QStringLiteral("Visible")))
    {
        return;
    }

    QVariant value =
        changedProperties.value(
            QStringLiteral("Visible")
        );

    /*
     * D-Bus 属性有时由 QDBusVariant 包装。
     */
    if (value.userType() ==
        qMetaTypeId<QDBusVariant>())
    {
        value =
            value.value<QDBusVariant>()
                 .variant();
    }

    const bool visible =
        value.toBool();

    qDebug()
        << "[OnboardController]"
        << "收到 Onboard Visible 变化:"
        << visible;

    /*
     * 这里是关键：
     *
     * 用户点击 Onboard 自己的隐藏按钮时，
     * visible 会变成 false，
     * Main.qml 中的 AI 面板会自动下移。
     */
    setKeyboardVisible(visible);

    m_wantVisible = visible;
}
