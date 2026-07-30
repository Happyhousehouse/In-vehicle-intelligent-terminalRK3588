#pragma once


#include <string>


#include <opencv2/opencv.hpp>



class GalleryService
{


public:


    GalleryService();



    bool init(
        const std::string& path
    );



    std::string save(
        const cv::Mat& image
    );



private:


    void scanIndex();



private:


    std::string galleryPath_;


    int photoIndex_;


};