#include "qrcode_recognize_thread.h"
#include <opencv5/opencv2/opencv.hpp>
#include <opencv5/opencv2/wechat_qrcode.hpp>
#include <iostream>
#include <QThread>
#include <QTime>
using namespace cv;
QRcode_recognize_thread::QRcode_recognize_thread(){
    
}

void QRcode_recognize_thread::setParameter(Mat &picture){
    image=picture;
}
//透视变换
Mat QRcode_recognize_thread::transformimage(Mat &originalImage){


    Mat perspectiveImage;
    // perspective transform
    Point2f objectivePoints[4], imagePoints[4];
    // original image points.
    imagePoints[0]=Point2f(0,410);
    imagePoints[1]=Point2f(145, 60);
    imagePoints[2]=Point2f(495, 60);
    imagePoints[3]=Point2f(640, 410);
    // objective points of perspective image.
    // move up the perspective image : objectivePoints.y - value .
    // move left the perspective image : objectivePoints.x - value.
    objectivePoints[0]=Point2f(0, 480);
    objectivePoints[1]=Point2f(0, 0);
    objectivePoints[2]=Point2f(640, 0);
    objectivePoints[3]=Point2f(640, 480);
    cv::Mat transform = cv::getPerspectiveTransform(imagePoints, objectivePoints);
    // perspective.
    cv::warpPerspective(originalImage,
                        perspectiveImage,
                        transform,
                        cv::Size(originalImage.cols, originalImage.rows),
                        cv::INTER_LINEAR );
    return perspectiveImage;



}
void QRcode_recognize_thread::init_QRcode(){
    std::string detect_prototxt = "/home/user/opencv/opencv_3rdparty-wechat_qrcode/detect.prototxt";
    std::string detect_caffe_model = "/home/user/opencv/opencv_3rdparty-wechat_qrcode/detect.caffemodel";
    std::string sr_prototxt = "/home/user/opencv/opencv_3rdparty-wechat_qrcode/sr.prototxt";
    std::string sr_caffe_model = "/home/user/opencv/opencv_3rdparty-wechat_qrcode/sr.caffemodel";
    try {
        detector = makePtr<wechat_qrcode::WeChatQRCode>(detect_prototxt,detect_caffe_model,sr_prototxt,sr_caffe_model);
    } catch (const std::exception& e) {
        std::cout <<
                     "\n---------------------------------------------------------------\n"
                     "Failed to initialize WeChatQRCode.\n"
                     "Please, download 'detector.*' and 'sr.*' from\n"
                     "https://github.com/WeChatCV/opencv_3rdparty/tree/wechat_qrcode\n"
                     "and put them into the current directory.\n"
                     "---------------------------------------------------------------\n";
        std::cout << e.what() << std::endl;
    }
}
void QRcode_recognize_thread::run(){
    QTime time;
    QTime time2;
    while(1){
        time2.start();
        time.start();
        if(!image.empty()){
            auto res = detector->detectAndDecode(image, points);
            static auto code=res;
            for (const auto& t : res) {
                std::cout << t << std::endl;
            }
            if(!res.empty()){
                if(!StateSwitchFlag){
                    StateSwitchFlag=true;
                    code=res;
                    std::cout<<"二维码导入成功"<<std::endl;
                    QThread::msleep(5000);
                }else if(code==res&&StateSwitchFlag){
                    static int i=0;
                    i++;
                    std::cout<<"识别到住户"<<i<<std::endl;
                    QThread::msleep(500);
                    StateFlag=true;
                }
            }
        }
        if(time.elapsed()<=100){
            QThread::msleep(100-time.elapsed());
        }

        std::cout<<"子线程"<<time2.elapsed()<<"ms"<<std::endl;
    }
}



