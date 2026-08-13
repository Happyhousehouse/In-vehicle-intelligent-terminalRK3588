#include "FatigueVision.h"

#include <fstream>
#include <iostream>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace
{

struct FaceCandidate
{
    cv::Rect2f box;
    float score = 0.0f;
};

}
// ============================================================
// Constructor
// ============================================================

FatigueVision::FatigueVision(
    const FatigueVisionConfig& config
)
    : config_(config)
{
}


// ============================================================
// Destructor
// ============================================================

FatigueVision::~FatigueVision()
{
    std::cout
        << "[FatigueVision] destructor begin"
        << std::endl;

    release();

    std::cout
        << "[FatigueVision] destructor end"
        << std::endl;
}

// ============================================================
// Load model
// ============================================================

bool FatigueVision::loadModelFile(
    const std::string& path,
    std::vector<unsigned char>& data
)
{
    std::ifstream file(
        path,
        std::ios::binary |
        std::ios::ate
    );

    if (!file.is_open())
    {
        std::cerr
            << "[FatigueVision] Failed to open model: "
            << path
            << std::endl;

        return false;
    }


    const std::streamsize size =
        file.tellg();

    if (size <= 0)
    {
        std::cerr
            << "[FatigueVision] Invalid model size: "
            << path
            << std::endl;

        return false;
    }


    file.seekg(
        0,
        std::ios::beg
    );


    data.resize(
        static_cast<size_t>(size)
    );


    if (!file.read(
            reinterpret_cast<char*>(data.data()),
            size))
    {
        std::cerr
            << "[FatigueVision] Failed to read model: "
            << path
            << std::endl;

        data.clear();

        return false;
    }


    std::cout
        << "[FatigueVision] Loaded model: "
        << path
        << " ("
        << size
        << " bytes)"
        << std::endl;


    return true;
}


// ============================================================
// Print RKNN model info
// ============================================================

void FatigueVision::printModelInfo(
    const char* name,
    rknn_context ctx
)
{
    rknn_input_output_num ioNum{};

    int ret = rknn_query(
        ctx,
        RKNN_QUERY_IN_OUT_NUM,
        &ioNum,
        sizeof(ioNum)
    );

    if (ret != RKNN_SUCC)
    {
        std::cerr
            << "[FatigueVision] "
            << name
            << " query IO num failed, ret="
            << ret
            << std::endl;

        return;
    }


    std::cout
        << "\n========== "
        << name
        << " =========="
        << std::endl;

    std::cout
        << "inputs  : "
        << ioNum.n_input
        << std::endl;

    std::cout
        << "outputs : "
        << ioNum.n_output
        << std::endl;


    // ========================================================
    // Input
    // ========================================================

    for (uint32_t i = 0;
         i < ioNum.n_input;
         ++i)
    {
        rknn_tensor_attr attr{};

        attr.index = i;

        ret = rknn_query(
            ctx,
            RKNN_QUERY_INPUT_ATTR,
            &attr,
            sizeof(attr)
        );

        if (ret != RKNN_SUCC)
        {
            continue;
        }


        std::cout
            << "[Input "
            << i
            << "] ";

        std::cout
            << "dims=";

        for (uint32_t d = 0;
             d < attr.n_dims;
             ++d)
        {
            std::cout
                << attr.dims[d];

            if (d + 1 < attr.n_dims)
            {
                std::cout << "x";
            }
        }

        std::cout
            << " type="
            << attr.type
            << " fmt="
            << attr.fmt
            << std::endl;
    }


    // ========================================================
    // Output
    // ========================================================

    for (uint32_t i = 0;
         i < ioNum.n_output;
         ++i)
    {
        rknn_tensor_attr attr{};

        attr.index = i;

        ret = rknn_query(
            ctx,
            RKNN_QUERY_OUTPUT_ATTR,
            &attr,
            sizeof(attr)
        );

        if (ret != RKNN_SUCC)
        {
            continue;
        }


        std::cout
            << "[Output "
            << i
            << "] ";

        std::cout
            << "dims=";

        for (uint32_t d = 0;
             d < attr.n_dims;
             ++d)
        {
            std::cout
                << attr.dims[d];

            if (d + 1 < attr.n_dims)
            {
                std::cout << "x";
            }
        }

        std::cout
            << " type="
            << attr.type
            << " fmt="
            << attr.fmt
            << std::endl;
    }


    std::cout
        << "=============================="
        << std::endl;
}


// ============================================================
// Face model
// ============================================================

bool FatigueVision::initFaceModel()
{
    if (!loadModelFile(
            config_.faceModelPath,
            faceModelData_))
    {
        return false;
    }


    int ret = rknn_init(
        &faceCtx_,
        faceModelData_.data(),
        static_cast<uint32_t>(
            faceModelData_.size()),
        0,
        nullptr
    );


    if (ret != RKNN_SUCC)
    {
        std::cerr
            << "[FatigueVision] Face rknn_init failed, ret="
            << ret
            << std::endl;

        faceCtx_ = 0;

        return false;
    }


    faceInitialized_ = true;


    printModelInfo(
        "FACE MODEL",
        faceCtx_
    );


    return true;
}


// ============================================================
// PFLD model
// ============================================================

bool FatigueVision::initPfldModel()
{
    if (!loadModelFile(
            config_.pfldModelPath,
            pfldModelData_))
    {
        return false;
    }


    int ret = rknn_init(
        &pfldCtx_,
        pfldModelData_.data(),
        static_cast<uint32_t>(
            pfldModelData_.size()),
        0,
        nullptr
    );


    if (ret != RKNN_SUCC)
    {
        std::cerr
            << "[FatigueVision] PFLD rknn_init failed, ret="
            << ret
            << std::endl;

        pfldCtx_ = 0;

        return false;
    }


    pfldInitialized_ = true;


    printModelInfo(
        "PFLD MODEL",
        pfldCtx_
    );


    return true;
}


// ============================================================
// Initialize
// ============================================================

bool FatigueVision::initialize()
{
    if (initialized_)
    {
        return true;
    }


    std::cout
        << "[FatigueVision] Initializing..."
        << std::endl;


    // --------------------------------------------------------
    // Face
    // --------------------------------------------------------

    if (!initFaceModel())
    {
        std::cerr
            << "[FatigueVision] Face model initialization failed."
            << std::endl;

        release();

        return false;
    }


    // --------------------------------------------------------
    // PFLD
    // --------------------------------------------------------

    if (!initPfldModel())
    {
        std::cerr
            << "[FatigueVision] PFLD model initialization failed."
            << std::endl;

        release();

        return false;
    }


    initialized_ = true;


    std::cout
        << "[FatigueVision] Initialization SUCCESS."
        << std::endl;


    return true;
}


// ============================================================
// Release
// ============================================================

void FatigueVision::release()
{
    std::cout
        << "[FatigueVision] release begin"
        << std::endl;


    initialized_ = false;


    // ========================================================
    // PFLD
    // ========================================================

    if (pfldInitialized_)
    {
        std::cout
            << "[FatigueVision] destroy PFLD begin"
            << std::endl;

        rknn_destroy(
            pfldCtx_
        );

        std::cout
            << "[FatigueVision] destroy PFLD OK"
            << std::endl;


        pfldCtx_ = 0;

        pfldInitialized_ = false;
    }


    // ========================================================
    // Face
    // ========================================================

    if (faceInitialized_)
    {
        std::cout
            << "[FatigueVision] destroy FACE begin"
            << std::endl;

        rknn_destroy(
            faceCtx_
        );

        std::cout
            << "[FatigueVision] destroy FACE OK"
            << std::endl;


        faceCtx_ = 0;

        faceInitialized_ = false;
    }


    // ========================================================
    // Model file buffer
    // ========================================================

    std::cout
        << "[FatigueVision] clear model buffers"
        << std::endl;


    pfldModelData_.clear();

    faceModelData_.clear();


    std::cout
        << "[FatigueVision] release end"
        << std::endl;
}


// ============================================================
// State
// ============================================================

bool FatigueVision::isInitialized() const
{
    return initialized_;
}


// ============================================================
// Process
//
// 下一步再实现真正的：
//
// BGR
//  ↓
// Face
//  ↓
// PFLD
//  ↓
// Eye/Mouth Ratio
// ============================================================

// ============================================================
// Face inference
// ============================================================

bool FatigueVision::runFace(
    const cv::Mat& bgrFrame,
    cv::Rect& faceRect,
    float& faceScore
)
{
    faceRect = cv::Rect();
    faceScore = 0.0f;


    if (!faceInitialized_)
    {
        return false;
    }


    if (bgrFrame.empty())
    {
        return false;
    }


    // ========================================================
    // 1. Resize
    //
    // Face model:
    // 320 x 240
    // ========================================================

    cv::Mat resized;

    cv::resize(
        bgrFrame,
        resized,
        cv::Size(
            config_.faceInputWidth,
            config_.faceInputHeight
        ),
        0.0,
        0.0,
        cv::INTER_LINEAR
    );


    // ========================================================
    // 2. BGR -> RGB
    // ========================================================

    cv::Mat rgb;

    cv::cvtColor(
        resized,
        rgb,
        cv::COLOR_BGR2RGB
    );


    // ========================================================
    // 3. Face preprocess
    //
    // 原模型要求：
    //
    // (pixel - 127) / 128
    //
    // 输入：
    // FLOAT32 NHWC
    //
    // RKNN runtime 再转换成模型的 FLOAT16
    // ========================================================

    cv::Mat inputFloat;

    rgb.convertTo(
        inputFloat,
        CV_32FC3
    );


    inputFloat =
        (inputFloat - 127.0f)
        /
        128.0f;


    if (!inputFloat.isContinuous())
    {
        inputFloat =
            inputFloat.clone();
    }


    // ========================================================
    // 4. RKNN input
    // ========================================================

    rknn_input input{};

    input.index = 0;

    input.buf =
        inputFloat.data;

    input.size =
        static_cast<uint32_t>(
            config_.faceInputWidth *
            config_.faceInputHeight *
            3 *
            sizeof(float)
        );

    input.type =
        RKNN_TENSOR_FLOAT32;

    input.fmt =
        RKNN_TENSOR_NHWC;

    input.pass_through = 0;


    int ret =
        rknn_inputs_set(
            faceCtx_,
            1,
            &input
        );


    if (ret != RKNN_SUCC)
    {
        std::cerr
            << "[FatigueVision] Face rknn_inputs_set failed, ret="
            << ret
            << std::endl;

        return false;
    }


    // ========================================================
    // 5. RKNN run
    // ========================================================

    ret =
        rknn_run(
            faceCtx_,
            nullptr
        );


    if (ret != RKNN_SUCC)
    {
        std::cerr
            << "[FatigueVision] Face rknn_run failed, ret="
            << ret
            << std::endl;

        return false;
    }


    // ========================================================
    // 6. Outputs
    //
    // output[0]:
    // 1 x 4420 x 2
    //
    // output[1]:
    // 1 x 4420 x 4
    //
    // 要求 runtime 转成 FLOAT32
    // ========================================================

    rknn_output outputs[2]{};

    outputs[0].index = 0;
    outputs[0].want_float = 1;

    outputs[1].index = 1;
    outputs[1].want_float = 1;


    ret =
        rknn_outputs_get(
            faceCtx_,
            2,
            outputs,
            nullptr
        );


    if (ret != RKNN_SUCC)
    {
        std::cerr
            << "[FatigueVision] Face rknn_outputs_get failed, ret="
            << ret
            << std::endl;

        return false;
    }


    float* scores =
        static_cast<float*>(
            outputs[0].buf
        );

    float* boxes =
        static_cast<float*>(
            outputs[1].buf
        );


    if (scores == nullptr ||
        boxes == nullptr)
    {
        std::cerr
            << "[FatigueVision] Face output buffer is null."
            << std::endl;

        rknn_outputs_release(
            faceCtx_,
            2,
            outputs
        );

        return false;
    }


    // ========================================================
    // 7. Confidence filtering
    //
    // score:
    //
    // [background, face]
    //
    // 第 i 个 anchor:
    //
    // scores[i * 2 + 1]
    // ========================================================

    constexpr int FACE_BOX_COUNT = 4420;


    std::vector<FaceCandidate> candidates;

    candidates.reserve(32);


    for (int i = 0;
         i < FACE_BOX_COUNT;
         ++i)
    {
        const float score =
            scores[i * 2 + 1];


        if (score <
            config_.faceScoreThreshold)
        {
            continue;
        }


        // ----------------------------------------------------
        // 原 Face 模型输出：
        //
        // xmin
        // ymin
        // xmax
        // ymax
        //
        // 坐标已经是归一化 [0,1] 空间。
        // ----------------------------------------------------

        float x1 =
            boxes[i * 4 + 0];

        float y1 =
            boxes[i * 4 + 1];

        float x2 =
            boxes[i * 4 + 2];

        float y2 =
            boxes[i * 4 + 3];


        // ----------------------------------------------------
        // 防止模型略微超出 0~1
        // ----------------------------------------------------

        x1 =
            std::max(
                0.0f,
                std::min(1.0f, x1)
            );

        y1 =
            std::max(
                0.0f,
                std::min(1.0f, y1)
            );

        x2 =
            std::max(
                0.0f,
                std::min(1.0f, x2)
            );

        y2 =
            std::max(
                0.0f,
                std::min(1.0f, y2)
            );


        if (x2 <= x1 ||
            y2 <= y1)
        {
            continue;
        }


        // ----------------------------------------------------
        // 直接映射到原始 Camera frame
        // ----------------------------------------------------

        const float px1 =
            x1 *
            static_cast<float>(
                bgrFrame.cols
            );

        const float py1 =
            y1 *
            static_cast<float>(
                bgrFrame.rows
            );

        const float px2 =
            x2 *
            static_cast<float>(
                bgrFrame.cols
            );

        const float py2 =
            y2 *
            static_cast<float>(
                bgrFrame.rows
            );


        FaceCandidate candidate;

        candidate.score = score;

        candidate.box =
            cv::Rect2f(
                px1,
                py1,
                px2 - px1,
                py2 - py1
            );


        candidates.push_back(
            candidate
        );
    }


    // ========================================================
    // 8. Outputs 已经复制到 candidates
    // 可以释放 RKNN output
    // ========================================================

    rknn_outputs_release(
        faceCtx_,
        2,
        outputs
    );


    if (candidates.empty())
    {
        return false;
    }


    // ========================================================
    // 9. Score descending
    // ========================================================

    std::sort(
        candidates.begin(),
        candidates.end(),

        [](const FaceCandidate& a,
           const FaceCandidate& b)
        {
            return a.score > b.score;
        }
    );


    // ========================================================
    // 10. NMS
    //
    // 我们最终只需要驾驶员一个主要人脸。
    //
    // 但仍然按照原算法：
    //
    // score sort
    // ↓
    // IoU 0.3
    // ↓
    // NMS
    // ↓
    // 取最高 score
    // ========================================================

    std::vector<FaceCandidate> kept;

    kept.reserve(
        candidates.size()
    );


    for (const auto& candidate :
         candidates)
    {
        bool suppressed = false;


        for (const auto& selected :
             kept)
        {
            const float iou =
                boxIoU(
                    candidate.box,
                    selected.box
                );


            if (iou >
                config_.faceNmsThreshold)
            {
                suppressed = true;
                break;
            }
        }


        if (!suppressed)
        {
            kept.push_back(
                candidate
            );
        }
    }


    if (kept.empty())
    {
        return false;
    }


    // ========================================================
    // 11. 当前只取最高分人脸
    // ========================================================

    const FaceCandidate& best =
        kept.front();


    int x =
        static_cast<int>(
            std::round(best.box.x)
        );

    int y =
        static_cast<int>(
            std::round(best.box.y)
        );

    int w =
        static_cast<int>(
            std::round(best.box.width)
        );

    int h =
        static_cast<int>(
            std::round(best.box.height)
        );


    // ========================================================
    // 12. 防越界
    // ========================================================

    x =
        std::max(
            0,
            std::min(
                x,
                bgrFrame.cols - 1
            )
        );

    y =
        std::max(
            0,
            std::min(
                y,
                bgrFrame.rows - 1
            )
        );


    w =
        std::min(
            w,
            bgrFrame.cols - x
        );

    h =
        std::min(
            h,
            bgrFrame.rows - y
        );


    if (w <= 0 ||
        h <= 0)
    {
        return false;
    }


    faceRect =
        cv::Rect(
            x,
            y,
            w,
            h
        );


    faceScore =
        best.score;


    return true;
}

// ============================================================
// IoU
// ============================================================

float FatigueVision::boxIoU(
    const cv::Rect2f& a,
    const cv::Rect2f& b
)
{
    const float x1 =
        std::max(a.x, b.x);

    const float y1 =
        std::max(a.y, b.y);

    const float x2 =
        std::min(
            a.x + a.width,
            b.x + b.width
        );

    const float y2 =
        std::min(
            a.y + a.height,
            b.y + b.height
        );


    const float w =
        std::max(0.0f, x2 - x1);

    const float h =
        std::max(0.0f, y2 - y1);


    const float intersection =
        w * h;


    const float areaA =
        a.width * a.height;

    const float areaB =
        b.width * b.height;


    const float unionArea =
        areaA + areaB - intersection;


    if (unionArea <= 0.0f)
    {
        return 0.0f;
    }


    return intersection / unionArea;
}

// ============================================================
// PFLD inference
// ============================================================

bool FatigueVision::runPfld(
    const cv::Mat& bgrFrame,
    const cv::Rect& faceRect,
    std::vector<cv::Point2f>& landmarks
)
{
    landmarks.clear();


    if (!pfldInitialized_)
    {
        return false;
    }


    if (bgrFrame.empty())
    {
        return false;
    }


    if (faceRect.width <= 0 ||
        faceRect.height <= 0)
    {
        return false;
    }


    // ========================================================
    // 1. 将 Face Rect 扩展成正方形
    //
    // 对应原项目：
    //
    // facebox_max_length = max(facebox_w, facebox_h)
    //
    // 然后让人脸框位于正方形中央。
    // ========================================================

    const int faceW =
        faceRect.width;

    const int faceH =
        faceRect.height;


    const int side =
        std::max(
            faceW,
            faceH
        );


    if (side <= 0)
    {
        return false;
    }


    const int widthMargin =
        (side - faceW) / 2;

    const int heightMargin =
        (side - faceH) / 2;


    const int squareX =
        faceRect.x -
        widthMargin;

    const int squareY =
        faceRect.y -
        heightMargin;


    // ========================================================
    // 2. 创建黑色正方形
    //
    // 如果人脸框靠近图像边缘，
    // 超出 Camera Frame 的部分使用黑色填充。
    //
    // 对应原项目 cut_resize_letterbox()。
    // ========================================================

    cv::Mat squareFace =
        cv::Mat::zeros(
            side,
            side,
            CV_8UC3
        );


    const cv::Rect imageRect(
        0,
        0,
        bgrFrame.cols,
        bgrFrame.rows
    );


    const cv::Rect wantedRect(
        squareX,
        squareY,
        side,
        side
    );


    const cv::Rect srcRect =
        wantedRect &
        imageRect;


    if (srcRect.width <= 0 ||
        srcRect.height <= 0)
    {
        return false;
    }


    // srcRect 在原图中的坐标
    //
    // dstX/dstY 是它应该复制到
    // squareFace 的哪个位置。
    const int dstX =
        srcRect.x -
        squareX;

    const int dstY =
        srcRect.y -
        squareY;


    const cv::Rect dstRect(
        dstX,
        dstY,
        srcRect.width,
        srcRect.height
    );


    bgrFrame(
        srcRect
    ).copyTo(
        squareFace(
            dstRect
        )
    );


    // ========================================================
    // 3. Resize 到 PFLD 输入 112x112
    //
    // 原项目 PIL 使用 BICUBIC，
    // OpenCV 对应使用 INTER_CUBIC。
    // ========================================================

    cv::Mat resized;

    cv::resize(
        squareFace,
        resized,
        cv::Size(
            config_.pfldInputWidth,
            config_.pfldInputHeight
        ),
        0.0,
        0.0,
        cv::INTER_CUBIC
    );


    // ========================================================
    // 4. BGR -> RGB
    // ========================================================

    cv::Mat rgb;

    cv::cvtColor(
        resized,
        rgb,
        cv::COLOR_BGR2RGB
    );


    // ========================================================
    // 5. PFLD preprocess
    //
    // 我们前面已经对当前重新转换的 RK3588 PFLD
    // 做过实际 ONNX / RKNN 对比验证。
    //
    // 当前模型输入使用：
    //
    // pixel / 127.5 - 1
    //
    // 即：
    //
    // [0,255] -> [-1,1]
    //
    // FLOAT32
    // NHWC
    // ========================================================

    cv::Mat inputFloat;

    rgb.convertTo(
        inputFloat,
        CV_32FC3,
        1.0 / 127.5,
        -1.0
    );


    if (!inputFloat.isContinuous())
    {
        inputFloat =
            inputFloat.clone();
    }


    // ========================================================
    // 6. RKNN input
    // ========================================================

    rknn_input input{};

    input.index = 0;

    input.buf =
        inputFloat.data;

    input.size =
        static_cast<uint32_t>(
            config_.pfldInputWidth *
            config_.pfldInputHeight *
            3 *
            sizeof(float)
        );

    input.type =
        RKNN_TENSOR_FLOAT32;

    input.fmt =
        RKNN_TENSOR_NHWC;

    input.pass_through = 0;


    int ret =
        rknn_inputs_set(
            pfldCtx_,
            1,
            &input
        );


    if (ret != RKNN_SUCC)
    {
        std::cerr
            << "[FatigueVision] PFLD rknn_inputs_set failed, ret="
            << ret
            << std::endl;

        return false;
    }


    // ========================================================
    // 7. RKNN run
    // ========================================================

    ret =
        rknn_run(
            pfldCtx_,
            nullptr
        );


    if (ret != RKNN_SUCC)
    {
        std::cerr
            << "[FatigueVision] PFLD rknn_run failed, ret="
            << ret
            << std::endl;

        return false;
    }


    // ========================================================
    // 8. 获取 PFLD 输出
    //
    // 模型输出：
    //
    // 1 x 196
    //
    // 即：
    //
    // x0 y0
    // x1 y1
    // ...
    // x97 y97
    // ========================================================

    rknn_output output{};

    output.index = 0;

    output.want_float = 1;


    ret =
        rknn_outputs_get(
            pfldCtx_,
            1,
            &output,
            nullptr
        );


    if (ret != RKNN_SUCC)
    {
        std::cerr
            << "[FatigueVision] PFLD rknn_outputs_get failed, ret="
            << ret
            << std::endl;

        return false;
    }


    float* preds =
        static_cast<float*>(
            output.buf
        );


    if (preds == nullptr)
    {
        std::cerr
            << "[FatigueVision] PFLD output buffer is null."
            << std::endl;


        rknn_outputs_release(
            pfldCtx_,
            1,
            &output
        );


        return false;
    }


    // ========================================================
    // 9. 恢复 98 个点到原始 Camera 坐标
    //
    // 原项目：
    //
    // center_x =
    //     pred_x * 112 * scale_l + x_offset
    //
    // scale_l =
    //     side / 112
    //
    // 所以：
    //
    // center_x =
    //     pred_x * side + squareX
    //
    // center_y =
    //     pred_y * side + squareY
    // ========================================================

    landmarks.reserve(98);


    bool outputValid = true;


    for (int i = 0;
         i < 98;
         ++i)
    {
        const float normalizedX =
            preds[i * 2 + 0];

        const float normalizedY =
            preds[i * 2 + 1];


        // 防止模型异常输出 NaN / Inf
        if (!std::isfinite(normalizedX) ||
            !std::isfinite(normalizedY))
        {
            outputValid = false;
            break;
        }


        const float x =
            normalizedX *
            static_cast<float>(side)
            +
            static_cast<float>(squareX);


        const float y =
            normalizedY *
            static_cast<float>(side)
            +
            static_cast<float>(squareY);


        landmarks.emplace_back(
            x,
            y
        );
    }


    // ========================================================
    // 10. 释放 RKNN output
    // ========================================================

    rknn_outputs_release(
        pfldCtx_,
        1,
        &output
    );


    if (!outputValid ||
        landmarks.size() != 98)
    {
        landmarks.clear();

        return false;
    }


    return true;
}

FatigueVisionResult FatigueVision::process(
    const cv::Mat& bgrFrame
)
{
    FatigueVisionResult result;


    if (!initialized_)
    {
        return result;
    }


    if (bgrFrame.empty())
    {
        return result;
    }


    // ========================================================
    // 1. Face
    // ========================================================

    cv::Rect faceRect;

    float faceScore = 0.0f;


    if (!runFace(
            bgrFrame,
            faceRect,
            faceScore))
    {
        // 没找到人脸
        return result;
    }


    result.faceDetected = true;

    result.faceRect =
        faceRect;

    result.faceScore =
        faceScore;


    // ========================================================
    // 2. PFLD
    // ========================================================

    std::vector<cv::Point2f> landmarks;


    if (!runPfld(
            bgrFrame,
            faceRect,
            landmarks))
    {
        // Face 找到了，但 PFLD 失败
        return result;
    }


    // ========================================================
    // 3. 保存 98 个关键点
    // ========================================================

    result.landmarksValid = true;

    result.landmarks =
        landmarks;


    // ========================================================
    // 4. Eye Ratio
    //
    // 60 ~ 67：右眼
    // 68 ~ 75：左眼
    // ========================================================

    result.rightEyeRatio =
        calculateEyeRatio(
            landmarks,
            60
        );

    result.leftEyeRatio =
        calculateEyeRatio(
            landmarks,
            68
        );


    // ========================================================
    // 5. Mouth Ratio
    // ========================================================

    result.mouthRatio =
        calculateMouthRatio(
            landmarks
        );


    // ========================================================
    // 6. 完整单帧视觉处理成功
    // ========================================================

    result.valid = true;


    return result;
}

float FatigueVision::pointDistance(
    const cv::Point2f& a,
    const cv::Point2f& b
)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;

    return std::sqrt(
        dx * dx +
        dy * dy
    );
}

float FatigueVision::calculateEyeRatio(
    const std::vector<cv::Point2f>& landmarks,
    int startIndex
)
{
    if (landmarks.size() < 98)
    {
        return 0.0f;
    }


    const cv::Point2f& p0 =
        landmarks[startIndex + 0];

    const cv::Point2f& p1 =
        landmarks[startIndex + 1];

    const cv::Point2f& p2 =
        landmarks[startIndex + 2];

    const cv::Point2f& p3 =
        landmarks[startIndex + 3];

    const cv::Point2f& p4 =
        landmarks[startIndex + 4];

    const cv::Point2f& p5 =
        landmarks[startIndex + 5];

    const cv::Point2f& p6 =
        landmarks[startIndex + 6];

    const cv::Point2f& p7 =
        landmarks[startIndex + 7];


    const float horizontal =
        pointDistance(
            p0,
            p4
        );


    if (horizontal < 1e-6f)
    {
        return 0.0f;
    }


    const float vertical1 =
        pointDistance(
            p1,
            p7
        );

    const float vertical2 =
        pointDistance(
            p2,
            p6
        );

    const float vertical3 =
        pointDistance(
            p3,
            p5
        );


    return
        (
            vertical1 +
            vertical2 +
            vertical3
        )
        /
        (
            3.0f *
            horizontal
        );
}

float FatigueVision::calculateMouthRatio(
    const std::vector<cv::Point2f>& landmarks
)
{
    if (landmarks.size() < 98)
    {
        return 0.0f;
    }


    const float horizontal =
        pointDistance(
            landmarks[76],
            landmarks[82]
        );


    if (horizontal < 1e-6f)
    {
        return 0.0f;
    }


    const float vertical =
        pointDistance(
            landmarks[79],
            landmarks[85]
        );


    return
        vertical /
        horizontal;
}