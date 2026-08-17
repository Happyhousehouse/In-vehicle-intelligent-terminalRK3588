#include "CameraService.h"


#include <fcntl.h>
#include <unistd.h>


#include <sys/ioctl.h>
#include <sys/mman.h>


#include <iostream>



CameraService::CameraService()
{
    fd_ = -1;

    width_ = 0;

    height_ = 0;

    running_ = false;

    streaming_ = false;
}

bool CameraService::isRunning() const
{
    return running_.load();
}

CameraService::~CameraService()
{
    std::cout
        << "[CameraService] destructor begin"
        << std::endl;

    stop();

    std::cout
        << "[CameraService] destructor end"
        << std::endl;
}



bool CameraService::start(
    const std::string& device,
    int width,
    int height
)
{
    std::lock_guard<std::mutex> lifecycleLock(
        lifecycleMutex_
    );


    /*
     * 已经运行。
     */
    if (running_.load())
    {
        std::cout
            << "[CameraService] already running"
            << std::endl;

        return true;
    }


    /*
     * 理论上 stop() 后这里应该已经干净。
     *
     * 为了保险，如果还残留 fd，
     * 先清理。
     */
    if (fd_ >= 0)
    {
        releaseResources();
    }


    width_ = width;

    height_ = height;


    fd_ = ::open(
        device.c_str(),
        O_RDWR
    );


    if (fd_ < 0)
    {
        perror("open camera");

        return false;
    }


    if (!set_format())
    {
        releaseResources();

        return false;
    }


    if (!init_mmap())
    {
        releaseResources();

        return false;
    }


    v4l2_buf_type type =
        V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;


    if (
        ioctl(
            fd_,
            VIDIOC_STREAMON,
            &type
        )
        < 0
    )
    {
        perror("streamon");

        releaseResources();

        return false;
    }


    streaming_.store(true);

    running_.store(true);


    captureThread_ =
        std::thread(
            &CameraService::captureLoop,
            this
        );


    std::cout
        << "[CameraService] started"
        << std::endl;


    return true;
}








bool CameraService::set_format()
{


    v4l2_format fmt{};



    fmt.type =
        V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;



    fmt.fmt.pix_mp.width =
        width_;



    fmt.fmt.pix_mp.height =
        height_;




    fmt.fmt.pix_mp.pixelformat =
        V4L2_PIX_FMT_NV12;



    fmt.fmt.pix_mp.num_planes=2;




    if(ioctl(
        fd_,
        VIDIOC_S_FMT,
        &fmt
    )<0)

    {

        perror("set format");

        return false;

    }



    return true;

}








bool CameraService::init_mmap()
{


    v4l2_requestbuffers req{};



    req.count=4;



    req.type =
        V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;



    req.memory =
        V4L2_MEMORY_MMAP;




    if(ioctl(
        fd_,
        VIDIOC_REQBUFS,
        &req
    )<0)

    {

        perror("request buffer");

        return false;

    }





    buffers_.resize(req.count);






    for(unsigned int i=0;i<req.count;i++)
    {


        v4l2_buffer buf{};



        v4l2_plane planes[2]{};




        buf.type =
            V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;



        buf.memory =
            V4L2_MEMORY_MMAP;



        buf.index=i;



        buf.length=2;



        buf.m.planes=planes;





        if(ioctl(
            fd_,
            VIDIOC_QUERYBUF,
            &buf
        )<0)

        {

            perror("querybuf");

            return false;

        }







        buffers_[i].length =
            planes[0].length;





        buffers_[i].start =
            mmap(

                nullptr,

                planes[0].length,

                PROT_READ|PROT_WRITE,

                MAP_SHARED,

                fd_,

                planes[0].m.mem_offset

            );





        if(buffers_[i].start==MAP_FAILED)

        {

            perror("mmap");

            return false;

        }






        if(ioctl(
            fd_,
            VIDIOC_QBUF,
            &buf
        )<0)

        {

            perror("qbuf");

            return false;

        }


    }


    return true;

}









void CameraService::captureLoop()
{


    while(running_)
    {


        v4l2_buffer buf{};



        v4l2_plane planes[2]{};




        buf.type =
            V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;



        buf.memory =
            V4L2_MEMORY_MMAP;



        buf.length=2;



        buf.m.planes=planes;






        if (
            ioctl(
                fd_,
                VIDIOC_DQBUF,
                &buf
            )
            < 0
        )
        {
            if (!running_.load())
            {
                break;
            }


            continue;
        }





        unsigned char* nv12 =
            static_cast<unsigned char*>(
                buffers_[buf.index].start
            );





        cv::Mat yuv(

            height_*3/2,

            width_,

            CV_8UC1,

            nv12

        );






        cv::Mat bgr;



        cv::cvtColor(

            yuv,

            bgr,

            cv::COLOR_YUV2BGR_NV12

        );






        {

            std::lock_guard<std::mutex> lock(
                frameMutex_
            );


            latestFrame_ =
                bgr.clone();

        }






        if (!running_.load())
        {
            break;
        }


        ioctl(
            fd_,
            VIDIOC_QBUF,
            &buf
        );


    }

}









cv::Mat CameraService::getFrame()
{


    std::lock_guard<std::mutex> lock(
        frameMutex_
    );



    if(latestFrame_.empty())
        return cv::Mat();




    return latestFrame_.clone();

}








cv::Mat CameraService::capture()
{

    return getFrame();

}





void CameraService::releaseResources()
{
    /*
     * mmap
     */
    for (auto& b : buffers_)
    {
        if (
            b.start != nullptr
            &&
            b.start != MAP_FAILED
        )
        {
            munmap(
                b.start,
                b.length
            );


            b.start = nullptr;

            b.length = 0;
        }
    }


    buffers_.clear();


    /*
     * fd
     */
    if (fd_ >= 0)
    {
        ::close(
            fd_
        );

        fd_ = -1;
    }


    streaming_.store(false);
}



void CameraService::stop()
{
    std::lock_guard<std::mutex> lifecycleLock(
        lifecycleMutex_
    );


    std::cout
        << "[CameraService] stop begin"
        << std::endl;


    /*
     * 首先告诉采集线程退出。
     */
    running_.store(false);


    /*
     * STREAMOFF 可以让阻塞中的 DQBUF 返回，
     * 避免 join 长时间卡住。
     */
    if (
        fd_ >= 0
        &&
        streaming_.exchange(false)
    )
    {
        std::cout
            << "[CameraService] streamoff"
            << std::endl;


        v4l2_buf_type type =
            V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;


        ioctl(
            fd_,
            VIDIOC_STREAMOFF,
            &type
        );
    }


    /*
     * 此时采集线程应该可以退出。
     */
    if (captureThread_.joinable())
    {
        std::cout
            << "[CameraService] joining capture thread"
            << std::endl;


        captureThread_.join();


        std::cout
            << "[CameraService] capture thread joined"
            << std::endl;
    }


    releaseResources();


    /*
     * 摄像头关闭后，
     * latestFrame 也必须清掉。
     *
     * 否则上层可能拿到关闭前最后一帧，
     * 误认为摄像头仍然工作。
     */
    {
        std::lock_guard<std::mutex> frameLock(
            frameMutex_
        );

        latestFrame_.release();
    }


    std::cout
        << "[CameraService] stop end"
        << std::endl;
}