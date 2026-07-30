#include "camera/CameraService.h"

#include "photo/GalleryService.h"


#include <opencv2/opencv.hpp>

#include <iostream>



int main()
{


    CameraService camera;



    if(!camera.start(
        "/dev/video11",
        1920,
        1080
    ))
    {

        std::cout
        <<"camera start failed"
        <<std::endl;

        return -1;

    }




    GalleryService gallery;


    gallery.init(
        "/home/firefly/Pictures"
    );



    std::cout
    <<"camera running..."
    <<std::endl;




    cv::namedWindow(
        "camera",
        cv::WINDOW_AUTOSIZE
    );





    while(true)
    {


        cv::Mat frame =
            camera.getFrame();




        if(!frame.empty())
        {


            cv::imshow(
                "camera",
                frame
            );


        }
        else
        {

            std::cout
            <<"frame empty"
            <<std::endl;

        }






        int key =
            cv::waitKey(10);





        if(key=='p')
        {


            cv::Mat photo =
                camera.capture();



            if(!photo.empty())
            {

                gallery.save(photo);

                std::cout
                <<"photo saved"
                <<std::endl;

            }

        }






        if(key==27)
        {

            break;

        }



    }





    camera.stop();



    cv::destroyAllWindows();



    std::cout
    <<"camera stopped"
    <<std::endl;



    return 0;

}