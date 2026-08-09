#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QVariantList>

class QNetworkReply;

class WeatherService : public QObject
{
    Q_OBJECT

    /*
     * 当前城市名称。
     */
    Q_PROPERTY(
        QString city
        READ city
        NOTIFY weatherChanged
    )

    Q_PROPERTY(
        bool citySearching
        READ citySearching
        NOTIFY citySearchingChanged
    )

    /*
     * 当前温度，单位 ℃。
     */
    Q_PROPERTY(
        double temperature
        READ temperature
        NOTIFY weatherChanged
    )

    /*
     * 体感温度。
     */
    Q_PROPERTY(
        double apparentTemperature
        READ apparentTemperature
        NOTIFY weatherChanged
    )

    /*
     * 相对湿度 %。
     */
    Q_PROPERTY(
        int humidity
        READ humidity
        NOTIFY weatherChanged
    )

    /*
     * 风速 km/h。
     */
    Q_PROPERTY(
        double windSpeed
        READ windSpeed
        NOTIFY weatherChanged
    )

    /*
     * 当前降水量 mm。
     */
    Q_PROPERTY(
        double precipitation
        READ precipitation
        NOTIFY weatherChanged
    )

    /*
     * Open-Meteo WMO weather code。
     */
    Q_PROPERTY(
        int weatherCode
        READ weatherCode
        NOTIFY weatherChanged
    )

    /*
     * 中文天气描述。
     */
    Q_PROPERTY(
        QString weatherText
        READ weatherText
        NOTIFY weatherChanged
    )

    /*
     * 是否正在获取天气。
     */
    Q_PROPERTY(
        bool loading
        READ loading
        NOTIFY loadingChanged
    )

    /*
     * 最后一次错误。
     */
    Q_PROPERTY(
        QString errorString
        READ errorString
        NOTIFY errorChanged
    )

    /*
    * 未来7天天气。
    */
    Q_PROPERTY(
        QVariantList dailyForecast
        READ dailyForecast
        NOTIFY weatherChanged
    )

public:

    explicit WeatherService(
        QObject *parent = nullptr
    );

    /*
     * 设置天气位置。
     *
     * 第一版我们直接使用固定经纬度。
     */
    void setLocation(
        double latitude,
        double longitude,
        const QString &city
    );

    /*
     * 手动刷新天气。
     *
     * 后面可以直接从 QML：
     *
     * weatherService.refresh()
     */
    Q_INVOKABLE
    void refresh();


    QString city() const;

    double temperature() const;

    double apparentTemperature() const;

    int humidity() const;

    double windSpeed() const;

    double precipitation() const;

    int weatherCode() const;

    QString weatherText() const;

    bool loading() const;

    QString errorString() const;

    /*
    * 根据城市名称搜索位置。
    *
    * QML:
    *
    * weatherService.searchCity("广州")
    */
    Q_INVOKABLE
    void searchCity(
        const QString &cityName
    );


    bool citySearching() const;

    QVariantList dailyForecast() const;

signals:

    /*
     * 天气数据更新成功。
     */
    void weatherChanged();

    /*
     * 网络请求状态变化。
     */
    void loadingChanged();

    /*
     * 请求失败。
     */
    void errorChanged();

    /*
     * 单独提供成功信号，
     * 后面调试以及 QML 都可以使用。
     */
    void updateFinished();

    void updateFailed(
        QString error
    );

    void citySearchingChanged();

    void citySearchFailed(
        QString error
    );

    /*
    * 城市搜索成功。
    */
    void citySearchSucceeded(
        QString city
    );
private slots:

    void onWeatherReplyFinished(
        QNetworkReply *reply
    );
    void onGeocodingReplyFinished(
        QNetworkReply *reply
    );

private:

    /*
     * WMO weather code
     * 转换为中文。
     */
    QString weatherCodeToText(
        int code
    ) const;

    /*
    * 城市搜索单独使用一个 NetworkManager。
    *
    * 这样不会影响已经跑通的天气请求。
    */
    QNetworkAccessManager geocodingManager_;

    bool citySearching_ = false;
private:

    QNetworkAccessManager networkManager_;

    double latitude_ = 0.0;

    double longitude_ = 0.0;

    QString city_;


    double temperature_ = 0.0;

    double apparentTemperature_ = 0.0;

    int humidity_ = 0;

    double windSpeed_ = 0.0;

    double precipitation_ = 0.0;

    int weatherCode_ = -1;

    QString weatherText_ =
        QStringLiteral("暂无天气");


    bool loading_ = false;

    QString errorString_;

    /*
    * 未来7天天气。
    *
    * 每一个 QVariantMap：
    *
    * date
    * weatherCode
    * weatherText
    * maxTemperature
    * minTemperature
    * rainProbability
    */
    QVariantList dailyForecast_;
};