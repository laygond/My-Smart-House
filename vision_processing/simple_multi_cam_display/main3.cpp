
#include <stdio.h>
#include <opencv2/opencv.hpp>
using namespace cv;
int main(int argc, char** argv )
{
    Mat image;
    image = imread("C:/Users/laygo/LAYGOND_GITHUB/My-Smart-House/vision_processing/simple_multi_cam_display/no_signal.png");
    if ( !image.data )
    {
        printf("No image data \n");
        return -1;
    }
    namedWindow("Display Image", WINDOW_AUTOSIZE );
    imshow("Display Image", image);
    waitKey(0);
    return 0;
}