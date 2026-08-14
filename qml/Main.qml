import QtQuick 2.15
import QtQuick.Controls 2.15


ApplicationWindow {

    id:mainWindow

    width:2160

    height:1080


    visible:true
    
    Loader {

        id: pageLoader

        anchors.fill: parent

        z: 100


        onLoaded: {

            /*
            * Weather.qml 如果有 backRequested 信号，
            * 就连接返回操作。
            */
            if (
                item
                &&
                item.backRequested
            )
            {
                item.backRequested.connect(
                    function()
                    {
                        console.log(
                            "[Main.qml] Weather back"
                        )

                        pageLoader.source = ""
                    }
                )
            }
        }
    }

    /*
    * ============================================================
    * 全局驾驶安全警告
    *
    * 优先级：
    *
    * 1. 即时长时间闭眼
    * 2. DEEP_FATIGUE
    * 3. FATIGUE
    *
    * NORMAL / UNKNOWN 不显示。
    * ============================================================
    */

    Rectangle {

        id: fatigueWarningBanner

        z: 200


        width: 1100
        height: 135


        anchors.top:
            parent.top

        anchors.topMargin:
            80


        anchors.horizontalCenter:
            parent.horizontalCenter


        radius:
            24


        visible:
            fatigueService.monitoringEnabled
            &&
            (
                fatigueService.eyeWarningActive
                ||
                fatigueService.fatigueLevel === "FATIGUE"
                ||
                fatigueService.fatigueLevel === "DEEP_FATIGUE"
            )


        color:
            fatigueService.eyeWarningActive
            ? "#D92D20"
            : fatigueService.fatigueLevel === "DEEP_FATIGUE"
            ? "#C62828"
            : "#E58A18"


        border.width:
            3


        border.color:
            fatigueService.eyeWarningActive
            ? "#FFB4AB"
            : fatigueService.fatigueLevel === "DEEP_FATIGUE"
            ? "#FF8A80"
            : "#FFD180"


        Row {

            anchors.centerIn:
                parent

            spacing:
                35


            /*
            * 警告图标
            */
            Text {

                text:
                    "⚠"

                color:
                    "white"

                font.pixelSize:
                    62

                font.bold:
                    true
            }


            /*
            * 警告文字
            */
            Column {

                anchors.verticalCenter:
                    parent.verticalCenter

                spacing:
                    8


                Text {

                    text:
                        fatigueService.eyeWarningActive
                        ? "危险：检测到长时间闭眼"
                        : fatigueService.fatigueLevel === "DEEP_FATIGUE"
                        ? "深度疲劳警告"
                        : "疲劳提醒"


                    color:
                        "white"


                    font.pixelSize:
                        34

                    font.bold:
                        true
                }


                Text {

                    text:
                        fatigueService.eyeWarningActive
                        ?
                        "请保持清醒，并在确保安全的前提下尽快停车休息"
                        :
                        fatigueService.fatigueLevel === "DEEP_FATIGUE"
                        ?
                        "检测到明显疲劳状态，建议在确保安全的前提下尽快停车休息"
                        :
                        "检测到疲劳趋势，请提高警惕并合理安排休息"


                    color:
                        "#FFF4F0"


                    font.pixelSize:
                        22
                }
            }
        }
    }

    title:"Vehicle AI Assistant"


    property string currentTime:"00:00:00"

    property string previousPage:""

    property int keyboardLiftDistance: 190


    /*
        聊天数据模型

        role:
        User / AI

        text:
        内容

    */

    ListModel {

        id:chatModel

    }

    /*
    * =========================
    * 历史会话列表
    * =========================
    */
    ListModel {
        id: historyModel
    }


    /*
    * =========================
    * 当前查看的历史消息
    * =========================
    */
    ListModel {
        id: historyMessageModel
    }


    property string historyDetailTitle:
        "请选择一个历史对话"

    /*
    * 当前在历史窗口中选中的会话。
    */
    property string selectedHistoryConversationId: ""


    function weatherSymbol(code)
    {
        if (code === 0)
            return "☀"

        if (code === 1 || code === 2)
            return "⛅"

        if (code === 3)
            return "☁"

        if (code === 45 || code === 48)
            return "≋"

        if (code >= 51 && code <= 67)
            return "☂"

        if (code >= 71 && code <= 77)
            return "❄"

        if (code >= 80 && code <= 82)
            return "☂"

        if (code >= 85 && code <= 86)
            return "❄"

        if (code >= 95)
            return "⚡"

        return "☁"
    }

    //========================
    // C++信号连接
    //========================


    function reloadConversationHistory()
    {
        historyModel.clear()


        var list =
            vehicleAssistant
                .conversationHistory()


        console.log(
            "[Main.qml] history count =",
            list.length
        )


        for (
            var i = 0;
            i < list.length;
            ++i
        )
        {
            historyModel.append({

                "conversationId":
                    list[i].conversationId,

                "title":
                    list[i].title,

                "createdAt":
                    list[i].createdAt,

                "updatedAt":
                    list[i].updatedAt,

                "messageCount":
                    list[i].messageCount,

                "isCurrent":
                    list[i].isCurrent
            })
        }
    }

    Connections {


        target:vehicleAssistant

        /*
        * ============================================================
        * 历史会话删除成功
        * ============================================================
        */
        function onConversationDeleted(
            conversationId,
            wasCurrent
        )
        {
            console.log(
                "[Main.qml] conversation deleted:",
                conversationId,
                "wasCurrent:",
                wasCurrent
            )


            selectedHistoryConversationId = ""

            historyDetailTitle =
                "请选择一个历史对话"


            historyMessageModel.clear()


            reloadConversationHistory()


            if (wasCurrent)
            {
                historyPopup.close()
            }
        }


        /*
        * ============================================================
        * 历史会话已切换成功。
        * ============================================================
        */
        function onConversationLoaded(
            conversationId,
            messages
        )
        {
            console.log(
                "[Main.qml] conversation loaded:",
                conversationId,
                "messages:",
                messages.length
            )


            /*
            * 清掉原当前聊天。
            */
            chatModel.clear()


            /*
            * 把历史消息重新放到主聊天区域。
            */
            for (
                var i = 0;
                i < messages.length;
                ++i
            )
            {
                var item =
                    messages[i]


                var uiRole =
                    item.role === "assistant"
                    ?
                    "AI"
                    :
                    "User"


                var uiMessage =
                    item.content


                /*
                * 如果历史中这条 AI 回答曾经被停止，
                * 恢复 UI 标记。
                */
                if (
                    item.role === "assistant"
                    &&
                    item.status === "interrupted"
                )
                {
                    uiMessage +=
                        "\n\n[回答已停止]"
                }


                chatModel.append({

                    "role":
                        uiRole,

                    "message":
                        uiMessage
                })
            }


            /*
            * 清空未发送草稿。
            */
            questionInput.text = ""


            /*
            * 隐藏软键盘。
            */
            onboardController
                .hideKeyboard()


            mainWindow
                .contentItem
                .forceActiveFocus()


            /*
            * 主聊天滚到最后。
            */
            if (chatModel.count > 0)
            {
                chatList
                    .positionViewAtEnd()
            }


            /*
            * 关闭历史窗口。
            */
            historyPopup.close()


            console.log(
                "[Main.qml] history restored to main chat"
            )
        }

        function onNewConversationStarted(conversationId)
        {
            console.log(
                "[Main.qml] ★ 收到 newConversationStarted"
            )

            console.log(
                "[Main.qml] new conversationId =",
                conversationId
            )

            /*
            * 清空旧聊天内容。
            */
            chatModel.clear()

            /*
            * 清空输入框。
            */
            questionInput.text = ""

            /*
            * 隐藏键盘。
            */
            onboardController.hideKeyboard()

            /*
            * 取消输入框焦点。
            */
            mainWindow.contentItem.forceActiveFocus()

            console.log(
                "[Main.qml] ★ 新对话界面清理完成"
            )
        }


        function onConversationOperationRejected(reason)
        {
            console.log(
                "[Main.qml] ★ 新对话失败:",
                reason
            )
        }

        function onAnswerStopped()
        {
            console.log(
                "[Main.qml] AI回答已停止"
            )

            var index =
                chatModel.count - 1


            if (index < 0)
                return


            if (
                chatModel.get(index).role !== "AI"
            )
            {
                return
            }


            var oldText =
                chatModel.get(index).message


            /*
            * 保留已经生成的答案，
            * 只在最后增加停止标记。
            */
            chatModel.setProperty(
                index,
                "message",
                oldText + "\n\n[回答已停止]"
            )


            chatList.positionViewAtEnd()
        }

        function onChatRejected(reason)
        {
            console.log(
                "[VehicleAssistant] 请求被拒绝:",
                reason
            )
        }

        function onOpenCameraPage()
        {

            onboardController.hideKeyboard()

            mainWindow.contentItem.forceActiveFocus()

            pageLoader.source =

                "qrc:/qml/Camera.qml"


        }

        function onOpenGalleryPage()
        {


            console.log(
                "AI open gallery"
            )

            onboardController.hideKeyboard()

            mainWindow.contentItem.forceActiveFocus()

            previousPage="Main"



            pageLoader.source =

                "qrc:/qml/Gallery.qml"


        }

        /*
            AI流式token

        */


        function onAnswerToken(token)
        {


            var index =
            chatModel.count-1



            if(chatModel.get(index).role !== "AI")
            {

                return;

            }



            var oldText =
                chatModel.get(index).message;



            chatModel.setProperty(

                index,

                "message",

                oldText + token

            );


            chatList.positionViewAtEnd();


        }



        function onAnswerFinished()
        {

            chatList.positionViewAtEnd()

        }


    }









    //========================
    // 背景
    //========================


    Image {


        anchors.fill:parent


        source:"qrc:/image/backgroundimage.png"



        fillMode:
        Image.PreserveAspectCrop



        enabled:false


    }






    //暗色遮罩


    Rectangle {


        anchors.fill:parent


        color:"#66000A18"


        enabled:false


    }









    //========================
    //顶部状态栏
    //========================


    Rectangle {


        width:parent.width


        height:60



        color:"#33000000"



        Text {


            anchors.left:parent.left


            anchors.leftMargin:40



            anchors.verticalCenter:
            parent.verticalCenter



            text:"◉ Vehicle AI Assistant"



            color:"#00BFFF"



            font.pixelSize:40


        }





        Text {


            anchors.right:parent.right


            anchors.rightMargin:40



            anchors.verticalCenter:
            parent.verticalCenter



            text:
            "CAN ✓     4G ✓     "
            +
            currentTime



            color:"white"



            font.pixelSize:22


        }


    }








    Timer {


        interval:1000


        running:true


        repeat:true



        onTriggered:{


            currentTime =
            Qt.formatDateTime(

                new Date(),

                "hh:mm:ss"

            )


        }

    }









    //========================
    // AI核心窗口
    //========================


    Rectangle {
        id: aiCorePanel

        width:1300


        height:650



        radius:30



        anchors.centerIn:parent

        /*
        * 每次 Onboard 显示时都主动上移，
        * 不再依赖窗口管理器自动调整。
        */
        anchors.verticalCenterOffset:
            onboardController.keyboardVisible
            ? -mainWindow.keyboardLiftDistance
            : 0

        Behavior on anchors.verticalCenterOffset {
            NumberAnimation {
                duration: 180
                easing.type: Easing.OutCubic
            }
        }

        color:"#66101828"



        border.width:1



        border.color:"#008CFF"



        /*
        * =========================
        * 会话控制
        * =========================
        */
        Row {
            id: conversationButtons

            anchors.top:
                parent.top

            anchors.right:
                parent.right

            anchors.topMargin: 18
            anchors.rightMargin: 20

            spacing: 14

            z: 10


            /*
            * =========================
            * 历史对话
            * =========================
            */
            Button {
                width: 150
                height: 48

                text: "历史对话"

                font.pixelSize: 20

                onClicked: {

                    /*
                    * 每次打开历史窗口，
                    * 都重新读取磁盘。
                    */
                    historyModel.clear()

                    historyMessageModel.clear()

                    selectedHistoryConversationId = ""

                    historyDetailTitle =
                        "请选择一个历史对话"


                    var list =
                        vehicleAssistant
                            .conversationHistory()


                    console.log(
                        "[Main.qml] history count =",
                        list.length
                    )


                    for (
                        var i = 0;
                        i < list.length;
                        ++i
                    )
                    {
                        historyModel.append({
                            "conversationId":
                                list[i].conversationId,

                            "title":
                                list[i].title,

                            "createdAt":
                                list[i].createdAt,

                            "updatedAt":
                                list[i].updatedAt,

                            "messageCount":
                                list[i].messageCount,

                            "isCurrent":
                                list[i].isCurrent
                        })
                    }


                    historyPopup.open()
                }
            }


            /*
            * =========================
            * 新对话
            * =========================
            */
            Button {
                width: 130
                height: 48

                text: "新对话"

                font.pixelSize: 20

                enabled:
                    !vehicleAssistant.generating

                onClicked: {
                    vehicleAssistant
                        .newConversation()
                }
            }
        }

        Column {


            anchors.fill:parent



            anchors.margins:25



            spacing:15






            // AI头像


            Rectangle {


                width:70


                height:70



                radius:35



                anchors.horizontalCenter:
                parent.horizontalCenter



                color:"#0066FF"




                Text {


                    anchors.centerIn:parent



                    text:"AI"



                    color:"white"



                    font.pixelSize:28


                }


            }









            Text {


                anchors.horizontalCenter:
                parent.horizontalCenter



                text:
                "车辆AI助手"



                color:"white"



                font.pixelSize:26


            }









            //========================
            // 聊天窗口
            //========================


            ScrollView {


                width:parent.width


                height:190



                clip:true





                ListView {


                    id:chatList



                    width:parent.width



                    model:chatModel



                    spacing:8





                    delegate: Rectangle {


                        width:chatList.width


                        height:
                        messageText.height+20



                        radius:10



                        color:

                        model.role==="User"

                        ?

                        "#335577FF"

                        :

                        "#22333333"




                        Text {


                            id:messageText


                            width:
                            parent.width-30



                            anchors.centerIn:parent



                            text:

                            model.role
                            +
                            ":\n"
                            +
                            model.message



                            color:"white"


                            font.pixelSize:30


                            wrapMode:
                            Text.WordWrap


                        }


                    }


                }


            }









            //========================
            // 输入框
            //========================


                        /*
            * 输入区域：
            * 左边是输入框，右边是中英文切换按钮。
            */
            Row {
                id: inputRow

                anchors.horizontalCenter:
                    parent.horizontalCenter

                spacing: 15

                Rectangle {
                    width: 680
                    height: 82

                    radius: 30

                    color: "#3300A8FF"

                    border.width: 1
                    border.color: "#3388CCFF"

                    TextField {
                        id: questionInput

                        anchors.fill: parent

                        /*
                        * 不再通过 anchors.margins 压缩上下高度，
                        * 使用左右内边距。
                        */
                        leftPadding: 20
                        rightPadding: 20
                        topPadding: 0
                        bottomPadding: 0

                        placeholderText:
                            "请输入您的问题..."

                        placeholderTextColor:
                            "#A0B8D0"

                        color: "white"

                        font.pixelSize: 32

                        verticalAlignment:
                            TextInput.AlignVCenter

                        cursorVisible:
                            activeFocus

                        selectByMouse: true

                        background: Rectangle {
                            color: "transparent"
                        }

                        /*
                        * 用户明确点击输入框时显示 Onboard。
                        *
                        * 不再使用 onActiveFocusChanged 自动弹出，
                        * 防止 Onboard 隐藏后因焦点恢复而再次弹出。
                        */
                        TapHandler {
                            onTapped: function(eventPoint, button) {

                                /*
                                * 记录用户实际点击的位置。
                                */
                                var clickX =
                                    eventPoint.position.x

                                var clickY =
                                    eventPoint.position.y


                                /*
                                * 先让输入框获得焦点。
                                */
                                questionInput.forceActiveFocus()


                                /*
                                * 根据触摸位置计算对应的字符位置。
                                *
                                * 例如：
                                *
                                * 我的摄像头|IMX415
                                *
                                * 点击这里以后，
                                * cursorPosition 就会移动到这里。
                                */
                                var pos =
                                    questionInput.positionAt(
                                        clickX,
                                        clickY
                                    )


                                if (pos >= 0) {
                                    questionInput.cursorPosition =
                                        pos
                                }


                                console.log(
                                    "[Main.qml] cursorPosition =",
                                    questionInput.cursorPosition
                                )


                                /*
                                * 最后显示 Onboard。
                                */
                                onboardController.showKeyboard()
                            }
                        }

                        onAccepted: {
                            sendButton.sendMessage()
                        }
                    }
                }

                /*
                * 中英文切换按钮。
                *
                * 不再依赖 Onboard 模拟 Ctrl+Space。
                */
                Button {
                    id: inputMethodButton

                    width: 110
                    height: 82

                    text: "中/英"

                    font.pixelSize: 24

                    onClicked: {
                        /*
                        * Fcitx5 切换时需要输入框处于焦点状态。
                        */
                        questionInput.forceActiveFocus()

                        onboardController.showKeyboard()

                        onboardController.toggleInputMethod()
                    }
                }
            }







            //========================
            //发送按钮
            //========================


            //========================
            // 发送 / 停止回答
            //========================

            Row {
                id: actionButtons

                anchors.horizontalCenter:
                    parent.horizontalCenter

                spacing: 20


                /*
                * =========================
                * 发送按钮
                * =========================
                */
                Button {
                    id: sendButton

                    width: 160
                    height: 55

                    enabled:
                        !vehicleAssistant.generating

                    text:
                        vehicleAssistant.generating
                        ? "回答中..."
                        : "发送"

                    font.pixelSize: 22


                    function sendMessage()
                    {
                        var question =
                            questionInput.text.trim()


                        if (question.length === 0)
                        {
                            return
                        }


                        /*
                        * C++ 之外再做一层保护。
                        */
                        if (vehicleAssistant.generating)
                        {
                            console.log(
                                "[Main.qml] AI正在回答，拒绝重复发送"
                            )

                            return
                        }


                        /*
                        * 隐藏键盘。
                        */
                        onboardController.hideKeyboard()


                        /*
                        * 输入框失去焦点。
                        */
                        mainWindow.contentItem
                            .forceActiveFocus()


                        /*
                        * 清空输入。
                        */
                        questionInput.text = ""


                        /*
                        * 用户消息。
                        */
                        chatModel.append({
                            "role": "User",
                            "message": question
                        })


                        /*
                        * AI 占位。
                        */
                        chatModel.append({
                            "role": "AI",
                            "message": ""
                        })


                        chatList.positionViewAtEnd()


                        /*
                        * 调用 C++。
                        */
                        vehicleAssistant.chat(
                            question
                        )
                    }


                    onClicked: {
                        sendMessage()
                    }
                }


                /*
                * =========================
                * 停止回答按钮
                * =========================
                */
                Button {
                    id: stopButton

                    width: 160
                    height: 55


                    /*
                    * 只有生成过程中显示。
                    */
                    visible:
                        vehicleAssistant.generating


                    /*
                    * stop 已经发出以后禁止再次点击。
                    */
                    enabled:
                        vehicleAssistant.generating
                        &&
                        !vehicleAssistant.stopping


                    text:
                        vehicleAssistant.stopping
                        ? "停止中..."
                        : "停止回答"


                    font.pixelSize: 22


                    onClicked: {
                        console.log(
                            "[Main.qml] 请求停止AI回答"
                        )


                        vehicleAssistant.stopGeneration()
                    }
                }
            }



        }


    }

    /*
    * ============================================================
    * 驾驶员疲劳状态
    * ============================================================
    */

    Rectangle {

        id: fatigueStatusCard


        // 与右侧天气卡保持对称
        width: 340
        height: 390


        anchors.right:
            aiCorePanel.left

        anchors.rightMargin:
            35


        anchors.verticalCenter:
            aiCorePanel.verticalCenter


        radius:
            28


        color:
            "#66101828"


        border.width:
            2


        border.color:
            !fatigueService.monitoringEnabled
            ? "#607080"
            : fatigueService.fatigueLevel === "NORMAL"
            ? "#35D07F"
            : fatigueService.fatigueLevel === "FATIGUE"
                ? "#FFB020"
                : fatigueService.fatigueLevel === "DEEP_FATIGUE"
                ? "#FF4D4F"
                : "#708090"


        // 只有主界面显示
        visible:
            pageLoader.source.toString() === ""


        /*
        * ========================================
        * 标题
        * ========================================
        */

        Text {

            anchors.top:
                parent.top

            anchors.topMargin:
                28


            anchors.horizontalCenter:
                parent.horizontalCenter


            text:
                "驾驶员状态"


            color:
                "white"


            font.pixelSize:
                29

            font.bold:
                true
        }


        /*
        * ========================================
        * 状态灯
        * ========================================
        */

        Rectangle {

            anchors.top:
                parent.top

            anchors.topMargin:
                92


            anchors.horizontalCenter:
                parent.horizontalCenter


            width:
                44

            height:
                44

            radius:
                22


            color:
                !fatigueService.monitoringEnabled
                ? "#607080"
                : fatigueService.fatigueLevel === "NORMAL"
                ? "#35D07F"
                : fatigueService.fatigueLevel === "FATIGUE"
                    ? "#FFB020"
                    : fatigueService.fatigueLevel === "DEEP_FATIGUE"
                    ? "#FF4D4F"
                    : "#8090A0"
        }


        /*
        * ========================================
        * 最终疲劳等级
        * ========================================
        */

        Text {

            anchors.top:
                parent.top

            anchors.topMargin:
                155


            anchors.horizontalCenter:
                parent.horizontalCenter


            text:
                !fatigueService.monitoringEnabled
                ? "已关闭"
                : fatigueService.fatigueLevel === "NORMAL"
                ? "正常"
                : fatigueService.fatigueLevel === "FATIGUE"
                    ? "疲劳"
                    : fatigueService.fatigueLevel === "DEEP_FATIGUE"
                    ? "深度疲劳"
                    : "学习中"


            color:
                !fatigueService.monitoringEnabled
                ? "#A0A8B0"
                : fatigueService.fatigueLevel === "NORMAL"
                ? "#35D07F"
                : fatigueService.fatigueLevel === "FATIGUE"
                    ? "#FFB020"
                    : fatigueService.fatigueLevel === "DEEP_FATIGUE"
                    ? "#FF4D4F"
                    : "#C0CBD5"


            font.pixelSize:
                46

            font.bold:
                true
        }


        /*
        * ========================================
        * 运行状态
        * ========================================
        */

        Text {

            anchors.top:
                parent.top

            anchors.topMargin:
                235


            anchors.horizontalCenter:
                parent.horizontalCenter


            text:
                fatigueService.monitoringEnabled
                ?
                (
                    fatigueService.fatigueLevel === "UNKNOWN"
                    ? "正在建立驾驶员基准"
                    : "疲劳监测运行中"
                )
                :
                "疲劳监测未运行"


            color:
                "#AFC4D5"


            font.pixelSize:
                20
        }


        /*
        * ========================================
        * 开启 / 关闭
        * ========================================
        */

        Button {

            width:
                220

            height:
                62


            anchors.bottom:
                parent.bottom

            anchors.bottomMargin:
                30


            anchors.horizontalCenter:
                parent.horizontalCenter


            text:
                fatigueService.monitoringEnabled
                ? "关闭疲劳检测"
                : "开启疲劳检测"


            font.pixelSize:
                22


            onClicked: {

                fatigueService
                    .setMonitoringEnabled(
                        !fatigueService.monitoringEnabled
                    )
            }
        }
    }

    /*
    * ============================================================
    * 主界面当天实时天气
    * ============================================================
    */

    Rectangle {

        id: homeWeatherCard


        /*
        * 放在AI主窗口右侧。
        */
        width: 340
        height: 390


        anchors.left:
            aiCorePanel.right

        anchors.leftMargin:
            35


        anchors.verticalCenter:
            aiCorePanel.verticalCenter


        radius:
            28


        color:
            "#66101828"


        border.width:
            1

        border.color:
            "#008CFF"


        /*
        * 只有主界面显示。
        *
        * 如果 Weather.qml / Camera.qml 等页面
        * 被 Loader 打开，本卡片本身会被 Loader 覆盖。
        */
        visible:
            pageLoader.source.toString() === ""


        /*
        * ========================================
        * 城市
        * ========================================
        */
        Text {

            anchors.top:
                parent.top

            anchors.topMargin:
                25


            anchors.left:
                parent.left

            anchors.leftMargin:
                28


            text:
                weatherService.city


            color:
                "white"


            font.pixelSize:
                29

            font.bold:
                true
        }


        /*
        * ========================================
        * 天气图标
        * ========================================
        */
        Text {

            anchors.top:
                parent.top

            anchors.topMargin:
                75


            anchors.horizontalCenter:
                parent.horizontalCenter


            text:
                mainWindow.weatherSymbol(
                    weatherService.weatherCode
                )


            color:
                "white"


            font.pixelSize:
                65
        }


        /*
        * ========================================
        * 当前温度
        * ========================================
        */
        Text {

            anchors.top:
                parent.top

            anchors.topMargin:
                145


            anchors.horizontalCenter:
                parent.horizontalCenter


            text:
                weatherService.temperature.toFixed(0)
                +
                "℃"


            color:
                "#00BFFF"


            font.pixelSize:
                72
        }


        /*
        * ========================================
        * 天气状态
        * ========================================
        */
        Text {

            anchors.top:
                parent.top

            anchors.topMargin:
                225


            anchors.horizontalCenter:
                parent.horizontalCenter


            text:
                weatherService.weatherText


            color:
                "white"


            font.pixelSize:
                27
        }


        /*
        * ========================================
        * 今日最高 / 最低
        * ========================================
        */
        Text {

            anchors.top:
                parent.top

            anchors.topMargin:
                270


            anchors.horizontalCenter:
                parent.horizontalCenter


            text:
                weatherService.dailyForecast.length > 0
                ?
                "最高 "
                +
                Number(
                    weatherService.dailyForecast[0].maxTemperature
                ).toFixed(0)
                +
                "°  最低 "
                +
                Number(
                    weatherService.dailyForecast[0].minTemperature
                ).toFixed(0)
                +
                "°"
                :
                "最高 --°  最低 --°"


            color:
                "#C6D5E2"


            font.pixelSize:
                21
        }


        /*
        * ========================================
        * 湿度
        * ========================================
        */
        Text {

            anchors.top:
                parent.top

            anchors.topMargin:
                310


            anchors.horizontalCenter:
                parent.horizontalCenter


            text:
                "湿度 "
                +
                weatherService.humidity
                +
                "%"


            color:
                "#AFC4D5"


            font.pixelSize:
                20
        }


        /*
        * ========================================
        * 查看详细天气
        * ========================================
        */
        Text {

            anchors.bottom:
                parent.bottom

            anchors.bottomMargin:
                18


            anchors.horizontalCenter:
                parent.horizontalCenter


            text:
                "查看未来7天天气  ›"


            color:
                "#00BFFF"


            font.pixelSize:
                18
        }


        /*
        * 整张天气卡片可点击。
        */
        MouseArea {

            anchors.fill:
                parent


            cursorShape:
                Qt.PointingHandCursor


            onClicked: {

                onboardController
                    .hideKeyboard()


                mainWindow
                    .contentItem
                    .forceActiveFocus()


                pageLoader.source =
                    "qrc:/qml/Weather.qml"
            }
        }
    }







    //========================
    //底部导航
    //========================


    Rectangle {


        width:parent.width


        height:150



        anchors.bottom:parent.bottom



        color:"#55000000"




        Row {


            anchors.centerIn:parent



            spacing:100






            NavItem{


                icon:"AI"


                title:"AI助手"



                onClicked:{
                    onboardController.hideKeyboard()

                    mainWindow.contentItem.forceActiveFocus()

                    pageLoader.source=""

                }

            }







            NavItem{


                icon:"🚗"


                title:"车辆状态"



                onClicked:{

                    onboardController.hideKeyboard()

                    mainWindow.contentItem.forceActiveFocus()

                    console.log("Vehicle")

                }

            }







            NavItem{


                icon:"📷"


                title:"摄像头"


                onClicked:{
                    
                    onboardController.hideKeyboard()

                    mainWindow.contentItem.forceActiveFocus()

                    pageLoader.source =
                        "qrc:/qml/Camera.qml"

                }

            }

            NavItem{


                icon:"🖼"


                title:"相册"



                onClicked:{

                    onboardController.hideKeyboard()

                    mainWindow.contentItem.forceActiveFocus()

                    mainWindow.previousPage = "Main"

                    pageLoader.source =

                        "qrc:/qml/Gallery.qml"



                }


            }

            NavItem {

                icon: "☀"

                title: "天气"


                onClicked: {

                    onboardController.hideKeyboard()

                    mainWindow
                        .contentItem
                        .forceActiveFocus()


                    pageLoader.source =
                        "qrc:/qml/Weather.qml"
                }
            }



            NavItem{


                icon:"☁"


                title:"云服务"



                onClicked:{

                    onboardController.hideKeyboard()

                    mainWindow.contentItem.forceActiveFocus()

                    console.log("Cloud")

                }

            }







            NavItem{


                icon:"⚙"


                title:"设置"



                onClicked:{

                    onboardController.hideKeyboard()

                    mainWindow.contentItem.forceActiveFocus()

                    console.log("Setting")

                }

            }



        }


    }

    /*
    * ============================================================
    * 历史对话窗口
    * ============================================================
    */
    Popup {
        id: historyPopup

        parent:
            Overlay.overlay

        width: 1180
        height: 760

        x:
            (parent.width - width) / 2

        y:
            (parent.height - height) / 2

        modal: true

        focus: true

        closePolicy:
            Popup.CloseOnEscape
            |
            Popup.CloseOnPressOutside


        background: Rectangle {

            radius: 24

            color:
                "#F0182433"

            border.width: 1

            border.color:
                "#008CFF"
        }


        contentItem: Column {

            spacing: 15


            /*
            * =========================
            * 标题栏
            * =========================
            */
            Row {

                width:
                    parent.width

                height: 60


                Text {

                    width:
                        parent.width - 120

                    anchors.verticalCenter:
                        parent.verticalCenter

                    text:
                        "历史对话"

                    color:
                        "white"

                    font.pixelSize:
                        30

                    font.bold:
                        true
                }


                Button {

                    width: 100
                    height: 48

                    text:
                        "关闭"

                    font.pixelSize:
                        19

                    onClicked: {
                        historyPopup.close()
                    }
                }
            }


            /*
            * =========================
            * 主体区域
            * 左：历史列表
            * 右：消息内容
            * =========================
            */
            Row {

                width:
                    parent.width

                height:
                    parent.height - 80

                spacing: 18


                /*
                * =================================================
                * 左侧历史列表
                * =================================================
                */
                Rectangle {

                    width: 390

                    height:
                        parent.height

                    radius: 16

                    color:
                        "#55111C2A"

                    border.width: 1

                    border.color:
                        "#406B8AAA"


                    ListView {

                        id:
                            historyListView

                        anchors.fill:
                            parent

                        anchors.margins:
                            10

                        clip:
                            true

                        spacing:
                            8

                        model:
                            historyModel


                        delegate: Rectangle {

                            width:
                                historyListView.width

                            height: 100

                            radius: 12


                            color:
                                historyMouseArea.pressed
                                ?
                                "#604A90E2"
                                :
                                "#4024384D"


                            border.width:
                                isCurrent
                                ?
                                2
                                :
                                1


                            border.color:
                                isCurrent
                                ?
                                "#00BFFF"
                                :
                                "#405D7890"


                            Column {

                                anchors.fill:
                                    parent

                                anchors.margins:
                                    12

                                spacing: 5


                                Text {

                                    width:
                                        parent.width

                                    text:
                                        title
                                        +
                                        (
                                            isCurrent
                                            ?
                                            "  [当前]"
                                            :
                                            ""
                                        )

                                    color:
                                        "white"

                                    font.pixelSize:
                                        20

                                    font.bold:
                                        true

                                    elide:
                                        Text.ElideRight
                                }


                                Text {

                                    width:
                                        parent.width

                                    text:
                                        "消息 "
                                        +
                                        messageCount
                                        +
                                        " 条"

                                    color:
                                        "#B8C7D9"

                                    font.pixelSize:
                                        16
                                }


                                Text {

                                    width:
                                        parent.width

                                    text:
                                        updatedAt

                                    color:
                                        "#8FA6BC"

                                    font.pixelSize:
                                        14

                                    elide:
                                        Text.ElideRight
                                }
                            }


                            MouseArea {

                                id:
                                    historyMouseArea

                                anchors.fill:
                                    parent


                                onClicked: {
                                    
                                    /*
                                    * 保存当前选中的历史会话 ID。
                                    */
                                    selectedHistoryConversationId =
                                        conversationId

                                    historyDetailTitle =
                                        title


                                    historyMessageModel
                                        .clear()


                                    var messages =
                                        vehicleAssistant
                                            .historyMessages(
                                                conversationId
                                            )


                                    console.log(
                                        "[Main.qml] history messages =",
                                        messages.length
                                    )


                                    for (
                                        var i = 0;
                                        i < messages.length;
                                        ++i
                                    )
                                    {
                                        historyMessageModel
                                            .append({

                                                "role":
                                                    messages[i].role,

                                                "content":
                                                    messages[i].content,

                                                "time":
                                                    messages[i].time,

                                                "status":
                                                    messages[i].status
                                            })
                                    }


                                    historyMessageList
                                        .positionViewAtBeginning()
                                }
                            }
                        }


                        /*
                        * 空历史提示。
                        */
                        Text {

                            anchors.centerIn:
                                parent

                            visible:
                                historyModel.count === 0

                            text:
                                "暂无历史对话"

                            color:
                                "#AAB7C5"

                            font.pixelSize:
                                22
                        }
                    }
                }


                /*
                * =================================================
                * 右侧历史消息
                * =================================================
                */
                Rectangle {

                    width:
                        parent.width
                        -
                        408

                    height:
                        parent.height

                    radius: 16

                    color:
                        "#44101928"

                    border.width: 1

                    border.color:
                        "#405D7890"


                    Column {

                        anchors.fill:
                            parent

                        anchors.margins:
                            15

                        spacing:
                            10


                        /*
                        * 当前历史会话标题。
                        */
                        Row {
                            width:
                                parent.width

                            height:
                                52

                            spacing:
                                12


                            /*
                            * =========================
                            * 历史标题
                            * =========================
                            */
                            Text {
                                width:
                                    parent.width
                                    -
                                    310

                                height:
                                    parent.height


                                verticalAlignment:
                                    Text.AlignVCenter


                                text:
                                    historyDetailTitle


                                color:
                                    "#00BFFF"


                                font.pixelSize:
                                    24


                                font.bold:
                                    true


                                elide:
                                    Text.ElideRight
                            }


                            /*
                            * =========================
                            * 继续此对话
                            * =========================
                            */
                            Button {
                                width:
                                    170

                                height:
                                    48


                                enabled:
                                    selectedHistoryConversationId.length
                                    >
                                    0
                                    &&
                                    !vehicleAssistant.generating
                                    &&
                                    selectedHistoryConversationId
                                    !==
                                    vehicleAssistant.currentConversationId


                                text:
                                    selectedHistoryConversationId
                                    ===
                                    vehicleAssistant.currentConversationId
                                    ?
                                    "当前对话"
                                    :
                                    "继续此对话"


                                font.pixelSize:
                                    19


                                onClicked: {

                                    if (
                                        selectedHistoryConversationId.length
                                        ===
                                        0
                                    )
                                    {
                                        return
                                    }


                                    vehicleAssistant
                                        .openConversation(
                                            selectedHistoryConversationId
                                        )
                                }
                            }


                            /*
                            * =========================
                            * 删除
                            * =========================
                            */
                            Button {
                                width:
                                    110

                                height:
                                    48


                                enabled:
                                    selectedHistoryConversationId.length
                                    >
                                    0
                                    &&
                                    !vehicleAssistant.generating


                                text:
                                    "删除"


                                font.pixelSize:
                                    19


                                onClicked: {

                                    if (
                                        selectedHistoryConversationId.length
                                        ===
                                        0
                                    )
                                    {
                                        return
                                    }


                                    /*
                                    * 不直接删除。
                                    * 先弹确认窗口。
                                    */
                                    deleteConversationPopup.open()
                                }
                            }
                        }


                        /*
                        * 历史消息。
                        */
                        ListView {

                            id:
                                historyMessageList

                            width:
                                parent.width

                            height:
                                parent.height - 55

                            clip:
                                true

                            spacing:
                                12

                            model:
                                historyMessageModel


                            delegate: Item {

                                width:
                                    historyMessageList.width

                                height:
                                    historyBubble.height + 10


                                Rectangle {

                                    id:
                                        historyBubble


                                    width:
                                        Math.min(
                                            historyMessageList.width
                                            *
                                            0.82,

                                            historyText.implicitWidth
                                            +
                                            38
                                        )


                                    height:
                                        historyText.implicitHeight
                                        +
                                        34


                                    anchors.left:
                                        role === "assistant"
                                        ?
                                        parent.left
                                        :
                                        undefined


                                    anchors.right:
                                        role === "user"
                                        ?
                                        parent.right
                                        :
                                        undefined


                                    radius: 14


                                    color:
                                        role === "user"
                                        ?
                                        "#305A8DEE"
                                        :
                                        "#30425A6D"


                                    border.width: 1


                                    border.color:
                                        role === "user"
                                        ?
                                        "#4D8FE8"
                                        :
                                        "#57748D"


                                    Text {

                                        id:
                                            historyText

                                        width:
                                            Math.min(
                                                historyMessageList.width
                                                *
                                                0.76,

                                                implicitWidth
                                            )


                                        anchors.centerIn:
                                            parent


                                        text:
                                            content
                                            +
                                            (
                                                status
                                                ===
                                                "interrupted"
                                                ?
                                                "\n\n[回答已停止]"
                                                :
                                                ""
                                            )


                                        color:
                                            "white"


                                        font.pixelSize:
                                            19


                                        wrapMode:
                                            Text.Wrap


                                        textFormat:
                                            Text.PlainText
                                    }
                                }
                            }


                            Text {

                                anchors.centerIn:
                                    parent

                                visible:
                                    historyMessageModel.count
                                    ===
                                    0

                                text:
                                    historyModel.count === 0
                                    ?
                                    "暂无历史记录"
                                    :
                                    "请选择左侧历史对话"

                                color:
                                    "#AAB7C5"

                                font.pixelSize:
                                    22
                            }
                        }
                    }
                }
            }
        }
    }

    /*
    * ============================================================
    * 删除历史会话确认窗口
    * ============================================================
    */
    Popup {
        id:
            deleteConversationPopup


        parent:
            Overlay.overlay


        width:
            560

        height:
            260


        x:
            (parent.width - width) / 2

        y:
            (parent.height - height) / 2


        modal:
            true


        focus:
            true


        closePolicy:
            Popup.NoAutoClose


        background: Rectangle {

            radius:
                20


            color:
                "#F0182433"


            border.width:
                1


            border.color:
                "#D05A5A"
        }


        contentItem: Column {

            anchors.fill:
                parent

            anchors.margins:
                25

            spacing:
                24


            Text {

                width:
                    parent.width


                text:
                    selectedHistoryConversationId
                    ===
                    vehicleAssistant.currentConversationId
                    ?
                    "确定删除当前对话吗？\n删除后将自动创建一个新的空白对话。"
                    :
                    "确定删除这个历史对话吗？"


                color:
                    "white"


                font.pixelSize:
                    23


                horizontalAlignment:
                    Text.AlignHCenter


                wrapMode:
                    Text.Wrap
            }


            Row {

                anchors.horizontalCenter:
                    parent.horizontalCenter

                spacing:
                    30


                /*
                * 取消。
                */
                Button {

                    width:
                        150

                    height:
                        52


                    text:
                        "取消"


                    font.pixelSize:
                        20


                    onClicked: {
                        deleteConversationPopup.close()
                    }
                }


                /*
                * 确定删除。
                */
                Button {

                    width:
                        150

                    height:
                        52


                    text:
                        "确定删除"


                    font.pixelSize:
                        20


                    onClicked: {

                        if (
                            selectedHistoryConversationId.length
                            ===
                            0
                        )
                        {
                            deleteConversationPopup.close()

                            return
                        }


                        console.log(
                            "[Main.qml] delete conversation:",
                            selectedHistoryConversationId
                        )


                        vehicleAssistant
                            .deleteConversation(
                                selectedHistoryConversationId
                            )


                        /*
                        * 此处只关闭确认框。
                        *
                        * 不立即修改 historyModel，
                        * 等 C++ 真正删除成功后
                        * onConversationDeleted() 再刷新。
                        */
                        deleteConversationPopup.close()
                    }
                }
            }
        }
    }


}