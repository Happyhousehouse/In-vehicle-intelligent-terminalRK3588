#include "WeatherService.h"

#include <QDebug>

#include <QNetworkReply>
#include <QNetworkRequest>

#include <QJsonDocument>
#include <QJsonObject>

#include <QUrl>
#include <QUrlQuery>

#include <QJsonArray>
#include <QVariantMap>
WeatherService::WeatherService(
    QObject *parent
)
    :
    QObject(parent)
{
    /*
     * 所有 HTTP 请求完成以后，
     * 统一进入这里。
     */
    connect(
        &networkManager_,
        &QNetworkAccessManager::finished,
        this,
        &WeatherService::onWeatherReplyFinished
    );

    connect(
        &geocodingManager_,
        &QNetworkAccessManager::finished,
        this,
        &WeatherService::onGeocodingReplyFinished
    );
}


void WeatherService::setLocation(
    double latitude,
    double longitude,
    const QString &city
)
{
    latitude_ =
        latitude;

    longitude_ =
        longitude;

    city_ =
        city;

    qDebug()
        << "[WeatherService]"
        << "location:"
        << city_
        << latitude_
        << longitude_;
}


void WeatherService::refresh()
{
    /*
     * 防止用户连续刷新。
     */
    if (loading_)
    {
        qDebug()
            << "[WeatherService]"
            << "request already running";

        return;
    }


    /*
     * 简单检查经纬度。
     */
    if (
        latitude_ < -90.0
        ||
        latitude_ > 90.0
        ||
        longitude_ < -180.0
        ||
        longitude_ > 180.0
    )
    {
        errorString_ =
            QStringLiteral(
                "天气位置经纬度无效"
            );

        emit errorChanged();

        emit updateFailed(
            errorString_
        );

        return;
    }


    /*
     * ========================================================
     * Open-Meteo API
     * ========================================================
     */

    QUrl url(
        QStringLiteral(
            "https://api.open-meteo.com/v1/forecast"
        )
    );


    QUrlQuery query;


    query.addQueryItem(
        QStringLiteral("latitude"),
        QString::number(
            latitude_,
            'f',
            6
        )
    );


    query.addQueryItem(
        QStringLiteral("longitude"),
        QString::number(
            longitude_,
            'f',
            6
        )
    );

    /*
    * ========================================================
    * 未来7天天气
    * ========================================================
    */
    query.addQueryItem(
        QStringLiteral("daily"),
        QStringLiteral(
            "weather_code,"
            "temperature_2m_max,"
            "temperature_2m_min,"
            "precipitation_probability_max"
        )
    );


    /*
    * 明确请求7天。
    */
    query.addQueryItem(
        QStringLiteral("forecast_days"),
        QStringLiteral("7")
    );

    /*
     * 请求当前天气数据。
     */
    query.addQueryItem(
        QStringLiteral("current"),
        QStringLiteral(
            "temperature_2m,"
            "relative_humidity_2m,"
            "apparent_temperature,"
            "precipitation,"
            "weather_code,"
            "wind_speed_10m"
        )
    );


    /*
     * 自动返回所在地时区。
     */
    query.addQueryItem(
        QStringLiteral("timezone"),
        QStringLiteral("auto")
    );


    /*
     * 摄氏度。
     */
    query.addQueryItem(
        QStringLiteral("temperature_unit"),
        QStringLiteral("celsius")
    );


    /*
     * 风速使用 km/h。
     */
    query.addQueryItem(
        QStringLiteral("wind_speed_unit"),
        QStringLiteral("kmh")
    );


    /*
     * 降水 mm。
     */
    query.addQueryItem(
        QStringLiteral("precipitation_unit"),
        QStringLiteral("mm")
    );


    url.setQuery(
        query
    );


    qDebug()
        << "[WeatherService]"
        << "request:"
        << url.toString();


    QNetworkRequest request(
        url
    );


    /*
     * 请求 JSON。
     */
    request.setRawHeader(
        "Accept",
        "application/json"
    );


    loading_ =
        true;

    errorString_
        .clear();


    emit loadingChanged();

    emit errorChanged();


    /*
     * 异步请求。
     *
     * 不会卡住 Qt UI。
     */
    networkManager_
        .get(
            request
        );
}


void WeatherService::onWeatherReplyFinished(
    QNetworkReply *reply
)
{
    loading_ =
        false;

    emit loadingChanged();


    /*
     * ========================================================
     * 网络错误
     * ========================================================
     */

    if (
        reply->error()
        !=
        QNetworkReply::NoError
    )
    {
        errorString_ =
            reply->errorString();


        qWarning()
            << "[WeatherService]"
            << "network error:"
            << errorString_;


        emit errorChanged();

        emit updateFailed(
            errorString_
        );


        reply->deleteLater();

        return;
    }


    /*
     * 读取 JSON。
     */
    const QByteArray data =
        reply->readAll();


    reply->deleteLater();


    qDebug()
        << "[WeatherService]"
        << "received bytes:"
        << data.size();


    /*
     * ========================================================
     * JSON解析
     * ========================================================
     */

    QJsonParseError parseError;


    const QJsonDocument document =
        QJsonDocument::fromJson(
            data,
            &parseError
        );


    if (
        parseError.error
        !=
        QJsonParseError::NoError
    )
    {
        errorString_ =
            QStringLiteral(
                "天气JSON解析失败: %1"
            )
            .arg(
                parseError.errorString()
            );


        qWarning()
            << "[WeatherService]"
            << errorString_;


        emit errorChanged();

        emit updateFailed(
            errorString_
        );

        return;
    }


    if (
        !document.isObject()
    )
    {
        errorString_ =
            QStringLiteral(
                "天气服务器返回格式异常"
            );


        emit errorChanged();

        emit updateFailed(
            errorString_
        );

        return;
    }


    const QJsonObject root =
        document.object();


    /*
     * Open-Meteo 错误响应。
     */
    if (
        root.value(
            QStringLiteral("error")
        ).toBool(false)
    )
    {
        errorString_ =
            root.value(
                QStringLiteral("reason")
            ).toString(
                QStringLiteral(
                    "天气服务器返回错误"
                )
            );


        qWarning()
            << "[WeatherService]"
            << errorString_;


        emit errorChanged();

        emit updateFailed(
            errorString_
        );

        return;
    }


    /*
     * 当前天气字段。
     */
    const QJsonObject current =
        root.value(
            QStringLiteral("current")
        ).toObject();


    if (
        current.isEmpty()
    )
    {
        errorString_ =
            QStringLiteral(
                "天气服务器没有返回 current 数据"
            );


        emit errorChanged();

        emit updateFailed(
            errorString_
        );

        return;
    }


    temperature_ =
        current.value(
            QStringLiteral(
                "temperature_2m"
            )
        ).toDouble();


    apparentTemperature_ =
        current.value(
            QStringLiteral(
                "apparent_temperature"
            )
        ).toDouble();


    humidity_ =
        current.value(
            QStringLiteral(
                "relative_humidity_2m"
            )
        ).toInt();


    precipitation_ =
        current.value(
            QStringLiteral(
                "precipitation"
            )
        ).toDouble();


    windSpeed_ =
        current.value(
            QStringLiteral(
                "wind_speed_10m"
            )
        ).toDouble();


    weatherCode_ =
        current.value(
            QStringLiteral(
                "weather_code"
            )
        ).toInt(-1);


    weatherText_ =
        weatherCodeToText(
            weatherCode_
        );

    /*
    * ========================================================
    * 解析未来7天天气
    * ========================================================
    */

    dailyForecast_.clear();


    const QJsonObject daily =
        root.value(
            QStringLiteral("daily")
        ).toObject();


    const QJsonArray dates =
        daily.value(
            QStringLiteral("time")
        ).toArray();


    const QJsonArray weatherCodes =
        daily.value(
            QStringLiteral("weather_code")
        ).toArray();


    const QJsonArray maxTemperatures =
        daily.value(
            QStringLiteral("temperature_2m_max")
        ).toArray();


    const QJsonArray minTemperatures =
        daily.value(
            QStringLiteral("temperature_2m_min")
        ).toArray();


    const QJsonArray rainProbabilities =
        daily.value(
            QStringLiteral(
                "precipitation_probability_max"
            )
        ).toArray();


    /*
    * 最多取7天。
    */
    const int dayCount =
        qMin(
            7,
            dates.size()
        );


    for (
        int i = 0;
        i < dayCount;
        ++i
    )
    {
        QVariantMap day;


        const QString date =
            dates
                .at(i)
                .toString();


        const int code =
            weatherCodes
                .at(i)
                .toInt(-1);


        const double maxTemp =
            maxTemperatures
                .at(i)
                .toDouble();


        const double minTemp =
            minTemperatures
                .at(i)
                .toDouble();


        const int rainProbability =
            rainProbabilities
                .at(i)
                .toInt();


        day.insert(
            QStringLiteral("date"),
            date
        );


        day.insert(
            QStringLiteral("weatherCode"),
            code
        );


        day.insert(
            QStringLiteral("weatherText"),
            weatherCodeToText(code)
        );


        day.insert(
            QStringLiteral("maxTemperature"),
            maxTemp
        );


        day.insert(
            QStringLiteral("minTemperature"),
            minTemp
        );


        day.insert(
            QStringLiteral("rainProbability"),
            rainProbability
        );


        dailyForecast_.append(
            day
        );
    }


    qDebug()
        << "[WeatherService]"
        << "forecast days:"
        << dailyForecast_.size();
    /*
     * 清掉旧错误。
     */
    errorString_
        .clear();


    /*
     * ========================================================
     * 调试输出
     * ========================================================
     */

    qDebug()
        << "====================================";

    qDebug()
        << "[WeatherService]"
        << "update success";

    qDebug()
        << "city:"
        << city_;

    qDebug()
        << "temperature:"
        << temperature_
        << "C";

    qDebug()
        << "apparentTemperature:"
        << apparentTemperature_
        << "C";

    qDebug()
        << "humidity:"
        << humidity_
        << "%";

    qDebug()
        << "precipitation:"
        << precipitation_
        << "mm";

    qDebug()
        << "windSpeed:"
        << windSpeed_
        << "km/h";

    qDebug()
        << "weatherCode:"
        << weatherCode_;

    qDebug()
        << "weather:"
        << weatherText_;

    qDebug()
        << "====================================";


    /*
     * 一次性通知 QML。
     */
    emit weatherChanged();

    emit errorChanged();

    emit updateFinished();
}


QString WeatherService::weatherCodeToText(
    int code
) const
{
    switch (code)
    {
        case 0:
            return QStringLiteral("晴");

        case 1:
            return QStringLiteral("大致晴朗");

        case 2:
            return QStringLiteral("局部多云");

        case 3:
            return QStringLiteral("阴");

        case 45:
        case 48:
            return QStringLiteral("雾");

        case 51:
        case 53:
        case 55:
            return QStringLiteral("毛毛雨");

        case 56:
        case 57:
            return QStringLiteral("冻毛毛雨");

        case 61:
            return QStringLiteral("小雨");

        case 63:
            return QStringLiteral("中雨");

        case 65:
            return QStringLiteral("大雨");

        case 66:
        case 67:
            return QStringLiteral("冻雨");

        case 71:
            return QStringLiteral("小雪");

        case 73:
            return QStringLiteral("中雪");

        case 75:
            return QStringLiteral("大雪");

        case 77:
            return QStringLiteral("雪粒");

        case 80:
        case 81:
        case 82:
            return QStringLiteral("阵雨");

        case 85:
        case 86:
            return QStringLiteral("阵雪");

        case 95:
            return QStringLiteral("雷暴");

        case 96:
        case 99:
            return QStringLiteral("雷暴伴冰雹");

        default:
            return QStringLiteral(
                "未知天气"
            );
    }
}


QString WeatherService::city() const
{
    return city_;
}


double WeatherService::temperature() const
{
    return temperature_;
}


double WeatherService::apparentTemperature() const
{
    return apparentTemperature_;
}


int WeatherService::humidity() const
{
    return humidity_;
}


double WeatherService::windSpeed() const
{
    return windSpeed_;
}


double WeatherService::precipitation() const
{
    return precipitation_;
}


int WeatherService::weatherCode() const
{
    return weatherCode_;
}


QString WeatherService::weatherText() const
{
    return weatherText_;
}


bool WeatherService::loading() const
{
    return loading_;
}


QString WeatherService::errorString() const
{
    return errorString_;
}

void WeatherService::searchCity(
    const QString &cityName
)
{
    const QString name =
        cityName.trimmed();


    /*
     * 空城市名直接拒绝。
     */
    if (name.isEmpty())
    {
        emit citySearchFailed(
            QStringLiteral(
                "请输入城市名称"
            )
        );

        return;
    }


    /*
     * 防止连续搜索。
     */
    if (citySearching_)
    {
        qDebug()
            << "[WeatherService]"
            << "city search already running";

        return;
    }


    /*
     * Open-Meteo Geocoding API。
     */
    QUrl url(
        QStringLiteral(
            "https://geocoding-api.open-meteo.com/v1/search"
        )
    );


    QUrlQuery query;


    /*
     * 要搜索的城市名。
     */
    query.addQueryItem(
        QStringLiteral("name"),
        name
    );


    /*
     * 第一版只取第一个结果。
     */
    query.addQueryItem(
        QStringLiteral("count"),
        QStringLiteral("1")
    );


    /*
     * 尽量返回中文名称。
     */
    query.addQueryItem(
        QStringLiteral("language"),
        QStringLiteral("zh")
    );


    /*
     * JSON。
     */
    query.addQueryItem(
        QStringLiteral("format"),
        QStringLiteral("json")
    );


    url.setQuery(
        query
    );


    qDebug()
        << "[WeatherService]"
        << "search city:"
        << name;


    qDebug()
        << "[WeatherService]"
        << "geocoding request:"
        << url.toString();


    QNetworkRequest request(
        url
    );


    request.setRawHeader(
        "Accept",
        "application/json"
    );


    citySearching_ = true;

    emit citySearchingChanged();


    geocodingManager_.get(
        request
    );
}

void WeatherService::onGeocodingReplyFinished(
    QNetworkReply *reply
)
{
    citySearching_ = false;

    emit citySearchingChanged();


    /*
     * =============================
     * 网络错误
     * =============================
     */
    if (
        reply->error()
        !=
        QNetworkReply::NoError
    )
    {
        const QString error =
            reply->errorString();


        qWarning()
            << "[WeatherService]"
            << "city search network error:"
            << error;


        emit citySearchFailed(
            error
        );


        reply->deleteLater();

        return;
    }


    const QByteArray data =
        reply->readAll();


    reply->deleteLater();


    /*
     * =============================
     * JSON解析
     * =============================
     */

    QJsonParseError parseError;


    const QJsonDocument document =
        QJsonDocument::fromJson(
            data,
            &parseError
        );


    if (
        parseError.error
        !=
        QJsonParseError::NoError
    )
    {
        const QString error =
            QStringLiteral(
                "城市搜索 JSON 解析失败: %1"
            )
            .arg(
                parseError.errorString()
            );


        emit citySearchFailed(
            error
        );

        return;
    }


    if (!document.isObject())
    {
        emit citySearchFailed(
            QStringLiteral(
                "城市搜索服务器返回格式异常"
            )
        );

        return;
    }


    const QJsonObject root =
        document.object();


    /*
     * API错误。
     */
    if (
        root.value(
            QStringLiteral("error")
        ).toBool(false)
    )
    {
        emit citySearchFailed(
            root.value(
                QStringLiteral("reason")
            ).toString(
                QStringLiteral(
                    "城市搜索失败"
                )
            )
        );

        return;
    }


    /*
     * =============================
     * results 数组
     * =============================
     */

    const QJsonArray results =
        root.value(
            QStringLiteral("results")
        ).toArray();


    if (results.isEmpty())
    {
        qWarning()
            << "[WeatherService]"
            << "city not found";


        emit citySearchFailed(
            QStringLiteral(
                "没有找到这个城市"
            )
        );

        return;
    }


    /*
     * 第一版取搜索结果第一项。
     */
    const QJsonObject location =
        results.first().toObject();


    const QString name =
        location.value(
            QStringLiteral("name")
        ).toString();


    const QString admin1 =
        location.value(
            QStringLiteral("admin1")
        ).toString();


    const QString country =
        location.value(
            QStringLiteral("country")
        ).toString();


    const double latitude =
        location.value(
            QStringLiteral("latitude")
        ).toDouble();


    const double longitude =
        location.value(
            QStringLiteral("longitude")
        ).toDouble();


    /*
     * =============================
     * 更新当前位置
     * =============================
     */

    latitude_ =
        latitude;

    longitude_ =
        longitude;

    city_ =
        name;


    qDebug()
        << "====================================";

    qDebug()
        << "[WeatherService]"
        << "city found";

    qDebug()
        << "city:"
        << name;

    qDebug()
        << "province:"
        << admin1;

    qDebug()
        << "country:"
        << country;

    qDebug()
        << "latitude:"
        << latitude;

    qDebug()
        << "longitude:"
        << longitude;

    qDebug()
        << "====================================";

    emit citySearchSucceeded(
        city_
    );
    /*
     * 城市改变后，
     * 立即获取该城市天气。
     */
    refresh();
}

bool WeatherService::citySearching() const
{
    return citySearching_;
}

QVariantList WeatherService::dailyForecast() const
{
    return dailyForecast_;
}