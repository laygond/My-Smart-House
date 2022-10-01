// https://forum.opencv.org/t/slow-frame-read-from-webcam/6880
// https://stereopi.com/blog/opencv-comparing-speed-c-and-python-code-raspberry-pi-stereo-vision
// https://docs.opencv.org/2.4/doc/tutorials/core/how_to_scan_images/how_to_scan_images.html#the-efficient-way
// https://learnopencv.com/getting-started-opencv-cuda-module/#disqus_thread
// https://pyimagesearch.com/2015/12/21/increasing-webcam-fps-with-python-and-opencv/
// https://forum.opencv.org/t/slow-frame-read-from-webcam/6880/4
// https://stackoverflow.com/questions/35037723/stdthread-constructor-with-no-parameter

// Compare frames per second using fmmpeg or magic? from terminal 
    //(vl4v2 for linux and camera app settings in windows)
// Speed up opencv if needed
    //CPU Threading for reading frames worked  
// run neural network (cpu only like mediapipe)
// make three cams: esp32, webcam, motioneyeOS single and dual webcam
// make multi cam display
// get opencv gpu and contrib version and check FPS improvement for reading image only and neural network

#include <opencv2/opencv.hpp>
#include <iostream>
#include <thread> //https://www.geeksforgeeks.org/multithreading-in-cpp/
#include <typeinfo>

using namespace std;

// // A dummy function
// void foo(int Z)
// {
// 	for (int i = 0; i < Z; i++) {
// 		cout << "Thread using function"
// 			" pointer as callable\n";
// 	}
// }

// int main(int, char**) {
//     // open the first webcam plugged in the computer
//     cv::VideoCapture camera(0); // in linux check $ ls /dev/video0
//     if (!camera.isOpened()) {
//         std::cerr << "ERROR: Could not open camera" << std::endl;
//         return 1;
//     }

//     // create a window to display the images from the webcam
//     cv::namedWindow("Webcam", cv::WINDOW_AUTOSIZE);

//     // array to hold image
//     cv::Mat frame;
    
//     //keep track of time
//     double t; 

//     // display the frame until you press a key
//     while (1) {
//         //Start timer
//         t = (double)cv::getTickCount();
        
//         // capture the next frame from the webcam
//         camera >> frame;
//         // show the image on the window
//         cv::imshow("Webcam", frame);
//         // wait (10ms) for esc key to be pressed to stop
//         if (cv::waitKey(1) == 27)
//             break;

//         // End Timer
//         t = ((double)cv::getTickCount() - t) / cv::getTickFrequency(); //time per image
//         cout << "FPS: " << 1/t << endl;
//     }
//     return 0;
// }

// //(EDUCATIONAL) JUST RECREATING SAME MEMBERS OF CLASS
// class VideoCapture {
// private:
//     cv::VideoCapture cam;
// public:
//     //constructors
//     VideoCapture(int src)
//         :cam(src)
//     {}
//     VideoCapture(const string& src)
//         :cam(src)
//     {}
//     bool isOpened(){
//         return cam.isOpened();
//     }
//     cv::VideoCapture& operator >> (cv::Mat& image){
//         return cam>>image;
//     }
// };


// // Verbose Draft
// class VideoCapture {
// private:
//     cv::VideoCapture cam;
//     cv::Mat frame;
//     thread th_read_frames;
//     void cb_read_frames(){ 
//         while(1){
//             cam >> frame;
//         }
//     }
// public:
//     //Constructor
//     VideoCapture(int src)
//         : cam(src)
//     {
//         if (cam.isOpened())
//         {
//             cam >> frame;
//             //th_read_frames = thread{cb_read_frames,this}; //start thread
//         }
//     }
//     //Destructor
//     ~VideoCapture()
//     {
//         //th_read_frames.detach();
//     }

//     // Member functions
//     bool isOpened(){
//         return cam.isOpened();
//     }
//     void operator >> (cv::Mat& image){
//         cout<<"Type for input: "<<typeid(image).name()<<endl;
//         cout<<"Type for frame: "<<typeid(frame).name()<<endl;

//         cout<<"input empty?: "<<image.empty()<<endl;
//         image = frame;
//         cout<<"input empty?: "<<image.empty()<<endl;
        
//         cout<<"Address for input: "<<&image<<endl;
//         cout<<"Address for frame: "<<&frame<<endl;

//         return;
//     }
// };


// REAL DEAL
class VideoCapture {
private:
    cv::VideoCapture cam;
    cv::Mat frame;
    thread th_read_frames;
    void cb_read_frames(){ 
        while(1){
            cam >> frame;
        }
    }
public:
    //Constructor
    VideoCapture(int src)
        : cam(src)
    {
        if (cam.isOpened())
        {
            th_read_frames = thread{cb_read_frames,this}; //start thread
        }
    }
    //Destructor
    ~VideoCapture()
    {
        th_read_frames.detach();
    }

    // Member functions
    bool isOpened(){
        return cam.isOpened();
    }
    void operator >> (cv::Mat& image){
        image = frame;
        return;
    }
};


int main(int, char**) {
    // open the first webcam plugged in the computer
    VideoCapture camera(0); // in linux check $ ls /dev/video0
    if (!camera.isOpened()) {
        std::cerr << "ERROR: Could not open camera" << std::endl;
        return -1;
    }

    // create a window to display the images from the webcam
    cv::namedWindow("Webcam", cv::WINDOW_AUTOSIZE);

    // array to hold image
    cv::Mat frame;
    
    //keep track of time
    double t;   // in between iterations
    double T;   // total cumulative time
    int frame_counter = 0;

    // display the frame until you press ESC key
    while (1) {
        //Start timer
        t = (double)cv::getTickCount();
        
        // capture the next frame from the webcam
        camera >> frame;
        // show the image on the window
        cv::imshow("Webcam", frame);
        // wait (10ms) for esc key to be pressed to stop
        if (cv::waitKey(1) == 27)
            break;

        // End Timer (FPS AVG over 100)
        T += ((double)cv::getTickCount() - t) / cv::getTickFrequency(); //time per image
        frame_counter++;
        if (frame_counter == 100)
        {
            cout << "AVG FPS: " << frame_counter/T << endl;
            frame_counter = 0;
            T = 0;
        }
    }
    return 0;
}


// // CPP program to demonstrate multithreading
// // using three different callables.
// #include <iostream>
// #include <thread> //https://www.geeksforgeeks.org/multithreading-in-cpp/
// #include <opencv2/opencv.hpp>
// using namespace std;

// // A dummy function
// void foo(int Z)
// {
// 	for (int i = 0; i < Z; i++) {
// 		cout << "Thread using function"
// 			" pointer as callable\n";
// 	}
// }

// void foofoo()
// {
// 	for (int i = 0; i < 3; i++) {
// 		cout << "Thread using function foo foo"
// 			" pointer as callable\n";
// 	}
// }

// // A callable object
// class thread_obj {
// public:
// 	void operator()(int x)
// 	{
// 		for (int i = 0; i < x; i++)
// 			cout << "Thread using function"
// 				" object as callable\n";
// 	}
// };

// int main()
// {
// 	cout << "Threads 1 and 2 and 3 "
// 		"operating independently" << endl;

// 	// This thread is launched by using
// 	// function pointer as callable
// 	thread th4(foofoo,nullptr);
//     thread th1(foo, 3);
    
// 	// This thread is launched by using
// 	// function object as callable
// 	thread th2(thread_obj(), 3);

// 	// Define a Lambda Expression
// 	auto f = [](int x) {
// 		for (int i = 0; i < x; i++)
// 			cout << "Thread using lambda"
// 			" expression as callable\n";
// 	};

// 	// This thread is launched by using
// 	// lamda expression as callable
// 	thread th3(f, 3);

//     while (1) {
//         if (cv::waitKey(1) == 27)
//             break;
//     }
// 	// Wait for the threads to finish
// 	// Wait for thread t1 to finish
// 	// th1.join();

// 	// // Wait for thread t2 to finish
// 	// th2.join();

// 	// // Wait for thread t3 to finish
// 	// th3.join();

// 	return 0;
// }
