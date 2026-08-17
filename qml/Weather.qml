import QtQuick 2.15
import QtQuick.Controls 2.15


Rectangle {

    id: weatherPage

    anchors.fill: parent

    color: "#101820"


    signal backRequested()


    /*
     * 城市搜索提示。
     */
    property string citySearchMessage: ""

    property bool citySearchError: false
    /*
     * ============================================================
     * 语音查询城市天气
     * ============================================================
     *
     * city:
     *     "广州"
     *     "深圳"
     *     "北京"
     *
     * 如果 city 为空：
     *     刷新当前城市天气。
     * ============================================================
     */
    function queryCityFromVoice(city)
    {
        var cityName =
            city.trim()


        console.log(
            "[Weather.qml] voice weather query:",
            cityName
        )


        /*
         * 用户明确说出了城市。
         */
        if (cityName.length > 0)
        {
            /*
             * 先显示到原来的城市输入框。
             */
            cityInput.text =
                cityName


            /*
             * 直接调用页面原来的查询逻辑。
             *
             * 不重新实现 WeatherService 查询。
             */
            searchButton
                .searchCity()


            return
        }


        /*
         * 没说具体城市：
         *
         * “今天天气怎么样”
         * “刷新天气”
         *
         * 就刷新 Weather.qml 当前城市。
         */
        weatherService
            .refresh()
    }

    /*
     * C++的7天天气。
     */
    property var forecastData:
        weatherService.dailyForecast


    /*
     * =========================
     * 天气符号
     * =========================
     */
    function weatherSymbol(code)
    {
        if (code === 0)
            return "☀"

        if (
            code === 1
            ||
            code === 2
        )
            return "⛅"

        if (code === 3)
            return "☁"

        if (
            code === 45
            ||
            code === 48
        )
            return "≋"

        if (
            code >= 51
            &&
            code <= 67
        )
            return "☂"

        if (
            code >= 71
            &&
            code <= 77
        )
            return "❄"

        if (
            code >= 80
            &&
            code <= 82
        )
            return "☂"

        if (
            code >= 85
            &&
            code <= 86
        )
            return "❄"

        if (code >= 95)
            return "⚡"

        return "☁"
    }


    /*
     * 今天 / 明天 / 后天。
     */
    function dayTitle(index)
    {
        if (index === 0)
            return "今天"

        if (index === 1)
            return "明天"

        if (index === 2)
            return "后天"

        return "第" + (index + 1) + "天"
    }


    /*
     * =========================
     * WeatherService 信号
     * =========================
     */
    Connections {

        target:
            weatherService


        function onCitySearchFailed(error)
        {
            console.log(
                "[Weather.qml] city search failed:",
                error
            )


            citySearchError =
                true


            citySearchMessage =
                "未找到该城市，请检查城市名称后重新输入"
        }


        function onCitySearchSucceeded(city)
        {
            console.log(
                "[Weather.qml] city found:",
                city
            )


            citySearchError =
                false


            citySearchMessage =
                "已切换到 " + city


            cityInput.text =
                ""
        }
    }


    Component.onCompleted: {

        weatherService.refresh()
    }


    /*
     * ============================================================
     * 返回
     * ============================================================
     */
    Button {

        width: 160
        height: 60

        anchors.left:
            parent.left

        anchors.leftMargin:
            40

        anchors.top:
            parent.top

        anchors.topMargin:
            25


        text:
            "← 返回"


        font.pixelSize:
            24


        onClicked: {

            onboardController
                .hideKeyboard()

            weatherPage
                .backRequested()
        }
    }


    Button {

        width: 110
        height: 50

        anchors.right:
            parent.right

        anchors.rightMargin:
            45

        anchors.top:
            parent.top

        anchors.topMargin:
            28


        text:
            weatherService.loading
            ?
            "..."
            :
            "↻ 刷新"


        font.pixelSize:
            19


        enabled:
            !weatherService.loading


        onClicked: {

            weatherService.refresh()
        }
    }

    /*
     * ============================================================
     * 标题
     * ============================================================
     */
    Text {

        anchors.horizontalCenter:
            parent.horizontalCenter

        anchors.top:
            parent.top

        anchors.topMargin:
            30


        text:
            "天气"


        color:
            "white"


        font.pixelSize:
            44
    }


    /*
     * ============================================================
     * 城市输入
     * ============================================================
     */
    Row {

        id:
            searchRow


        anchors.horizontalCenter:
            parent.horizontalCenter


        anchors.top:
            parent.top

        anchors.topMargin:
            105


        spacing:
            15


        /*
         * 城市输入框
         */
        Rectangle {

            width:
                520

            height:
                76


            radius:
                28


            color:
                "#3300A8FF"


            border.width:
                1

            border.color:
                "#3388CCFF"


            TextField {

                id:
                    cityInput


                anchors.fill:
                    parent


                leftPadding:
                    20

                rightPadding:
                    20


                placeholderText:
                    "请输入城市，例如：深圳"


                placeholderTextColor:
                    "#A0B8D0"


                color:
                    "white"


                font.pixelSize:
                    28


                verticalAlignment:
                    TextInput.AlignVCenter


                background:
                    Rectangle {
                        color:
                            "transparent"
                    }


                /*
                 * 与 AI 输入框一致：
                 * 点击后启动 Onboard。
                 */
                TapHandler {

                    onTapped:
                        function(
                            eventPoint,
                            button
                        )
                    {
                        cityInput
                            .forceActiveFocus()


                        var pos =
                            cityInput.positionAt(
                                eventPoint.position.x,
                                eventPoint.position.y
                            )


                        if (pos >= 0)
                        {
                            cityInput.cursorPosition =
                                pos
                        }


                        onboardController
                            .showKeyboard()
                    }
                }


                onAccepted: {

                    searchButton
                        .searchCity()
                }
            }
        }


        /*
         * ========================================================
         * 中英文切换
         *
         * 与 AI 页面保持一致。
         * ========================================================
         */
        Button {

            width:
                110

            height:
                76


            text:
                "中/英"


            font.pixelSize:
                24


            onClicked: {

                cityInput
                    .forceActiveFocus()


                onboardController
                    .showKeyboard()


                onboardController
                    .toggleInputMethod()
            }
        }


        /*
         * ========================================================
         * 查询
         * ========================================================
         */
        Button {

            id:
                searchButton


            width:
                150

            height:
                76


            text:
                weatherService.citySearching
                ?
                "查询中..."
                :
                "查询"


            enabled:
                !weatherService.citySearching
                &&
                cityInput.text.trim().length > 0


            font.pixelSize:
                24


            function searchCity()
            {
                var cityName =
                    cityInput
                        .text
                        .trim()


                if (
                    cityName.length === 0
                )
                {
                    citySearchError =
                        true

                    citySearchMessage =
                        "请输入城市名称"

                    return
                }


                citySearchMessage =
                    ""

                citySearchError =
                    false


                onboardController
                    .hideKeyboard()


                weatherPage
                    .forceActiveFocus()


                weatherService
                    .searchCity(
                        cityName
                    )
            }


            onClicked: {

                searchCity()
            }
        }
    }


    /*
     * ============================================================
     * 城市错误 / 成功提示
     *
     * 独立占一行，不再和天气卡片重叠。
     * ============================================================
     */
    Text {

        anchors.top:
            searchRow.bottom

        anchors.topMargin:
            10


        anchors.horizontalCenter:
            parent.horizontalCenter


        text:
            citySearchMessage


        color:
            citySearchError
            ?
            "#FF7070"
            :
            "#70D7FF"


        font.pixelSize:
            21


        visible:
            citySearchMessage.length > 0
    }


    /*
     * ============================================================
     * 今天重点天气
     * ============================================================
     */
    Rectangle {

        id:
            currentWeatherCard


        width:
            1680

        height:
            340


        anchors.horizontalCenter:
            parent.horizontalCenter


        anchors.top:
            parent.top

        anchors.topMargin:
            235


        radius:
            30


        color:
            "#66101828"


        border.width:
            2

        border.color:
            "#008CFF"


        /*
         * 城市
         */
        Text {

            anchors.left:
                parent.left

            anchors.leftMargin:
                50


            anchors.top:
                parent.top

            anchors.topMargin:
                30


            text:
                weatherService.city
                +
                " · 今天"


            color:
                "white"


            font.pixelSize:
                34
        }


        /*
         * 天气图标
         */
        Text {

            anchors.left:
                parent.left

            anchors.leftMargin:
                70


            anchors.verticalCenter:
                parent.verticalCenter


            text:
                weatherSymbol(
                    weatherService.weatherCode
                )


            color:
                "#FFFFFF"


            font.pixelSize:
                105
        }


        /*
         * 当前温度
         */
        Text {

            anchors.left:
                parent.left

            anchors.leftMargin:
                245


            anchors.verticalCenter:
                parent.verticalCenter


            text:
                weatherService
                    .temperature
                    .toFixed(1)
                +
                "℃"


            color:
                "#00BFFF"


            font.pixelSize:
                96
        }


        /*
         * 当前天气
         */
        Text {

            anchors.left:
                parent.left

            anchors.leftMargin:
                260


            anchors.bottom:
                parent.bottom

            anchors.bottomMargin:
                35


            text:
                weatherService.weatherText


            color:
                "white"


            font.pixelSize:
                30
        }


        /*
         * ========================================================
         * 今日详细信息
         * ========================================================
         */
        Column {

            anchors.right:
                parent.right

            anchors.rightMargin:
                70


            anchors.verticalCenter:
                parent.verticalCenter


            spacing:
                19


            Text {

                text:
                    forecastData.length > 0
                    ?
                    "最高 "
                    +
                    Number(
                        forecastData[0].maxTemperature
                    ).toFixed(1)
                    +
                    "℃    最低 "
                    +
                    Number(
                        forecastData[0].minTemperature
                    ).toFixed(1)
                    +
                    "℃"
                    :
                    "最高 --℃    最低 --℃"


                color:
                    "white"


                font.pixelSize:
                    28
            }


            Text {

                text:
                    "体感温度    "
                    +
                    weatherService
                        .apparentTemperature
                        .toFixed(1)
                    +
                    "℃"


                color:
                    "#D8E5F0"


                font.pixelSize:
                    27
            }


            Text {

                text:
                    "湿度        "
                    +
                    weatherService.humidity
                    +
                    "%"


                color:
                    "#D8E5F0"


                font.pixelSize:
                    27
            }


            Text {

                text:
                    "风速        "
                    +
                    weatherService
                        .windSpeed
                        .toFixed(1)
                    +
                    " km/h"


                color:
                    "#D8E5F0"


                font.pixelSize:
                    27
            }


            Text {

                text:
                    forecastData.length > 0
                    ?
                    "降雨概率    "
                    +
                    forecastData[0].rainProbability
                    +
                    "%"
                    :
                    "降雨概率    --%"


                color:
                    "#D8E5F0"


                font.pixelSize:
                    27
            }
        }
    }


    /*
     * ============================================================
     * 未来7天标题
     * ============================================================
     */
    Text {

        anchors.top:
            currentWeatherCard.bottom

        anchors.topMargin:
            25


        anchors.left:
            currentWeatherCard.left


        text:
            "未来7天天气"


        color:
            "white"


        font.pixelSize:
            30
    }


    /*
     * ============================================================
     * 未来7天
     * ============================================================
     */
    ListView {

        id:
            forecastList


        width:
            1760

        height:
            270


        anchors.horizontalCenter:
            parent.horizontalCenter


        anchors.top:
            currentWeatherCard.bottom

        anchors.topMargin:
            70


        orientation:
            ListView.Horizontal


        spacing:
            18


        clip:
            true


        model:
            weatherService.dailyForecast


        delegate:

            Rectangle {

                property var day:
                    modelData


                width:
                    230

                height:
                    245


                radius:
                    22


                /*
                 * 今天突出显示。
                 */
                color:
                    index === 0
                    ?
                    "#553078A8"
                    :
                    "#4415212E"


                border.width:
                    index === 0
                    ?
                    2
                    :
                    1


                border.color:
                    index === 0
                    ?
                    "#00BFFF"
                    :
                    "#405D7890"


                Column {

                    anchors.centerIn:
                        parent


                    spacing:
                        11


                    Text {

                        anchors.horizontalCenter:
                            parent.horizontalCenter


                        text:
                            dayTitle(index)


                        color:
                            index === 0
                            ?
                            "#00BFFF"
                            :
                            "white"


                        font.pixelSize:
                            25
                    }


                    Text {

                        anchors.horizontalCenter:
                            parent.horizontalCenter


                        text:
                            day.date
                                .substring(5)


                        color:
                            "#AABFD0"


                        font.pixelSize:
                            18
                    }


                    Text {

                        anchors.horizontalCenter:
                            parent.horizontalCenter


                        text:
                            weatherSymbol(
                                day.weatherCode
                            )


                        color:
                            "white"


                        font.pixelSize:
                            47
                    }


                    Text {

                        anchors.horizontalCenter:
                            parent.horizontalCenter


                        text:
                            day.weatherText


                        color:
                            "white"


                        font.pixelSize:
                            21
                    }


                    Text {

                        anchors.horizontalCenter:
                            parent.horizontalCenter


                        text:
                            Number(
                                day.maxTemperature
                            ).toFixed(0)
                            +
                            "° / "
                            +
                            Number(
                                day.minTemperature
                            ).toFixed(0)
                            +
                            "°"


                        color:
                            "white"


                        font.pixelSize:
                            22
                    }


                    Text {

                        anchors.horizontalCenter:
                            parent.horizontalCenter


                        text:
                            "降雨 "
                            +
                            day.rainProbability
                            +
                            "%"


                        color:
                            "#7FD9FF"


                        font.pixelSize:
                            18
                    }
                }
            }
    }
}