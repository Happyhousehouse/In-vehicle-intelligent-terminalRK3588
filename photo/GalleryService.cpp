#include "GalleryService.h"


#include <sys/stat.h>

#include <dirent.h>

#include <sstream>

#include <iomanip>

#include <regex>

#include <iostream>

#include <algorithm>





GalleryService::GalleryService()
{

    photoIndex_=0;

}









bool GalleryService::init(
        const std::string& path
)
{

    galleryPath_=path;



    mkdir(
        galleryPath_.c_str(),
        0755
    );



    scanIndex();



    return true;

}









void GalleryService::scanIndex()
{


    DIR* dir =
        opendir(
            galleryPath_.c_str()
        );


    if(!dir)
        return;



    struct dirent* entry;


    int maxIndex=-1;





    while(
        (entry=readdir(dir))
    )
    {


        std::string name =
            entry->d_name;



        std::regex pattern(
            "photo_([0-9]+)\\.jpg"
        );



        std::smatch match;



        if(std::regex_match(
            name,
            match,
            pattern
        ))
        {

            int index =
                std::stoi(
                    match[1]
                );


            if(index>maxIndex)
                maxIndex=index;

        }

    }




    closedir(dir);



    photoIndex_=maxIndex+1;



}









std::string GalleryService::save(
        const cv::Mat& image
)
{


    if(image.empty())
        return "";






    std::stringstream ss;



    ss
    <<galleryPath_
    <<"/photo_"
    <<std::setw(6)
    <<std::setfill('0')
    <<photoIndex_++
    <<".jpg";






    std::string filename =
        ss.str();






    if(cv::imwrite(
        filename,
        image
    ))
    {


        std::cout
        <<"saved:"
        <<filename
        <<std::endl;



        return filename;

    }





    return "";

}









std::vector<std::string> GalleryService::getPhotos()
{


    std::vector<std::pair<int,std::string>> temp;



    DIR* dir =

        opendir(

            galleryPath_.c_str()

        );



    if(!dir)

        return {};







    struct dirent* entry;





    while(

        (entry=readdir(dir))

    )
    {



        std::string name =

            entry->d_name;






        std::regex pattern(

            "photo_([0-9]+)\\.jpg"

        );





        std::smatch match;





        if(std::regex_match(

            name,

            match,

            pattern

        ))
        {


            int index =

                std::stoi(

                    match[1]

                );





            std::string path =

                galleryPath_

                +

                "/"

                +

                name;





            temp.push_back(

                {

                    index,

                    path

                }

            );



        }



    }





    closedir(dir);







    /*
        按编号倒序

        最新照片在前

    */


    std::sort(

        temp.begin(),

        temp.end(),

        [](

            const auto& a,

            const auto& b

        )

        {

            return a.first > b.first;

        }

    );







    std::vector<std::string> photos;






    for(auto &p:temp)

    {

        photos.push_back(

            p.second

        );

    }






    return photos;



}

bool GalleryService::removePhoto(
        const std::string& path
)
{


    if(
        path.empty()
    )
        return false;



    int ret =

        remove(
            path.c_str()
        );



    if(ret==0)
    {

        std::cout
        <<"deleted:"
        <<path
        <<std::endl;



        return true;

    }



    return false;


}