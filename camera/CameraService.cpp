#include "CameraService.h"


#include <fcntl.h>
#include <unistd.h>


#include <sys/ioctl.h>
#include <sys/mman.h>


#include <iostream>



CameraService::CameraService()
{

    fd_=-1;

    width_=0;

    height_=0;

    running_=false;

}




CameraService::~CameraService()
{

    stop();

}





bool CameraService::start(
        const std::string& device,
        int width,
        int height
)
{


    width_=width;

    height_=height;



    fd_=::open(

        device.c_str(),

        O_RDWR

    );



    if(fd_<0)
    {

        perror("open camera");

        return false;

    }





    if(!set_format())
        return false;




    if(!init_mmap())
        return false;






    v4l2_buf_type type =
        V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;




    if(ioctl(
        fd_,
        VIDIOC_STREAMON,
        &type
    )<0)

    {

        perror("streamon");

        return false;

    }






    running_=true;



    captureThread_=std::thread(

        &CameraService::captureLoop,

        this

    );




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






        if(ioctl(
            fd_,
            VIDIOC_DQBUF,
            &buf
        )<0)

        {

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









void CameraService::stop()
{


    running_=false;



    if(captureThread_.joinable())
    {

        captureThread_.join();

    }





    if(fd_>=0)
    {


        v4l2_buf_type type =
            V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;



        ioctl(
            fd_,
            VIDIOC_STREAMOFF,
            &type
        );





        for(auto& b:buffers_)
        {

            if(b.start)
            {

                munmap(
                    b.start,
                    b.length
                );

            }

        }



        ::close(fd_);


        fd_=-1;


    }


}