#pragma once


#include <string>

#include <vector>


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



    /*
        获取所有照片路径

    */

    std::vector<std::string> getPhotos();

    bool removePhoto(
        const std::string& path
    );


private:


    void scanIndex();



private:


    std::string galleryPath_;


    int photoIndex_;



};