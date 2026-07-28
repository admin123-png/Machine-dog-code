#include <opencv5/opencv2/opencv.hpp>
#include <sport_client_cmd_t.hpp>
#include <opencv2/wechat_qrcode.hpp>
#include <vector>
#include <unistd.h>
#include "lcmutil.h"
#include <QDebug>
#include <QTime>
#include <sys/time.h>
#include <colorgroup.h>
#include <QTimer>
#include <pthread.h>
#include "mythread.h"
#include <QDir>
#include <wait.h>
#include "udputil.h"
using namespace cv;
using namespace std;
VideoCapture cap;//摄像头实例化
myThread mythread;//实例化计时线程
colorGroup colorgroup;//实例化色彩阈值类
Mat srcImage(Size(640,480),CV_8UC3);//存放摄像头提取的图像
Mat transformImage(Size(640,480),CV_8UC3);//透视变换后的图像
pthread_mutex_t mutex2;      //定义锁
unsigned int roundNumber=0;  //圈数
string nextColor="blue";//下一个需要识别的颜色
string residenceColor="";//
string divergerColor="";
bool QRcodeExternalFlag=false;
//机器狗控制参数
double velocity[3]={0.2,0,0};
double euler_angles[3]={0,0,0};
double body_height=0;
double step_height=0;
int32_t api=1021;

bool judgmentLeft(Mat &image,int i,int j){
    if((int)image.at<uchar>(i,j)==0){
        if((int)image.at<uchar>(i,j+3)==255&&(int)image.at<uchar>(i,j+5)==255)
            return true;
        else
            return false;
    }else if((int)image.at<uchar>(i,j)==255&&j==0){
        if((int)image.at<uchar>(i,j+3)==255&&(int)image.at<uchar>(i,j+5)==255)
            return true;
        else
            return false;
    }else{
        return false;
    }
}
bool judgmentRight(Mat &image,int i,int j){
    if((int)image.at<uchar>(i,j)==0){
        if((int)image.at<uchar>(i,j-3)==255&&(int)image.at<uchar>(i,j-5)==255)
            return true;
        else
            return false;
    }else if((int)image.at<uchar>(i,j)==255&&j==399){
        if((int)image.at<uchar>(i,j-3)==255&&(int)image.at<uchar>(i,j-5)==255)
            return true;
        else
            return false;
    }else{
        return false;
    }
}
int leftPosition=0;
int rightPosition=0;
//递归算法求条带左侧长度
void RecursionLeft(Mat &image,int x,int y){
    if(y>0){
        if(image.at<uchar>(x,y-1)==0){
            RecursionLeft(image,x,y-1);
        }else if(image.at<uchar>(x+1,y-1)==0){
            RecursionLeft(image,x+1,y-1);
        }else if(image.at<uchar>(x-1,y-1)==0){
            RecursionLeft(image,x-1,y-1);
        }else{
            leftPosition=y;
        }
    }else
    {
        leftPosition=y;
    }

}
//递归算法求条带右侧长度
void RecursionRight(Mat &image,int x,int y){
    if(y<399){
        if(image.at<uchar>(x,y+1)==0){
            RecursionRight(image,x,y+1);
        }else if(image.at<uchar>(x+1,y+1)==0){
            RecursionRight(image,x+1,y+1);
        }else if(image.at<uchar>(x-1,y+1)==0){
            RecursionRight(image,x-1,y+1);
        }else{
            rightPosition=y;
        }
    }else{
        rightPosition=y;
    }
}
//识别条带的种类,返回值为0无条带,返回值为1短条带,返回值为2长条带,图像输入为二值图
bool recognizeBlackStripe(Mat &image){
    static int mid=200;
    leftPosition=200;
    rightPosition=200;
    int length=0;
    for(int i=0;i<=150;i++){
        if(image.at<uchar>(i,mid)==0){
            RecursionLeft(image,i,mid);
            RecursionRight(image,i,mid);
        }
        length=rightPosition-leftPosition;
        if(length>=398){
            int number=0;
            for(int j=120;j<280;j++){
                if(image.at<uchar>(0,j)==255)
                    number++;
            }
            if(number>80)
                return true;//判断为长的条带
        }
    }
    return false;
}
//改变图像尺寸，先切割，再缩小
Mat resize2(Mat &image){
    Mat ZoomOutimage;
    Mat ZoomOutimage2;
    Rect rect(119,89, 400, 300);//x,y,w,h
    ZoomOutimage=image(rect);
    resize(ZoomOutimage,ZoomOutimage2,Size(200,150));
    return ZoomOutimage2;
}
int getAverage(Mat &frame){
    int average=200;//中线均值
    int number=0;
    int value[150]={0};//存储每个点对应的x轴对应的值
    float sum=0;
    int left[480]={0};//存储左侧边缘
    float middle[480]={0};//存储中线
    int right[480]={0};//存储右侧边缘
    //提取赛道左侧边缘
    for(int i=0;i<frame.rows;i++){
        for(int j=0;j<frame.cols-3;j++){
            if(judgmentLeft(frame,i,j)){
                if(j==0)
                    left[i]=1;
                else
                    left[i]=j;
                break;
            }
        }
    }
    //提取赛道右侧边缘
    for(int i=0;i<frame.rows;i++){
        for(int j=frame.cols-1;j>=3;j--){
            if(judgmentRight(frame,i,j)){
                right[i]=j;
                break;
            }
        }
    }
    //提取中线,并将中线绘制到图上
    number=0;
    for(int i=0;i<(frame.rows/2.0);i++){
        if((left[i]<(left[i+1]+4))&&(left[i]>(left[i+1]-4))&&(left[i]!=0))
            if((right[i]<(right[i+1]+4))&&(right[i]>(right[i+1]-4))&&(right[i]!=0)){
                middle[i]=(left[i]+right[i])/2.0;
                frame.at<uchar>(i,(int)middle[i])=0;
                value[number]=middle[i];
                number++;
            }
    }
    //计算中线平均值
    sum=0;
    if(number!=0){
        for(int i=0;i<number;i++){
            sum=sum+value[i];
        }
        average=sum/number;
        //限幅
        if(average<=400&&average>=0){

        }else{
            average=200;
        }
    }
    return average;
}

void imageProcess(Mat &image){
    Mat ZoomOutimage;//尺寸缩小图
    Mat frame;//二值化黑色图
    Mat frame2;//二值化蓝色绿色黄色值图

    Mat Blurimage;//滤波后的图像
    int average=200;//中线均值
    static int goalAverage=200;//目标中线均值
    int number=0;
    resize(image,ZoomOutimage,Size(400,300));//改变图像尺寸为400*300
    medianBlur(ZoomOutimage,Blurimage,5);//中值滤波
    inRange(Blurimage,colorgroup.whiteMin,colorgroup.whiteMax,frame);//输出图像为单通道二值图
    /******************************************************住户任务***************************************************************/
//    if(mythread.mode==residenceright){
//        if(mythread.residenceTransientProcess==1){
//            velocity[0]=0.06;
//        }else if(mythread.residenceTransientProcess==2){
//            api=1006;
//            velocity[0]=0.0;
//        }else if(mythread.residenceTransientProcess==3){
//            api=1002;
//            euler_angles[0]=1.5;
//        }else if(mythread.residenceTransientProcess==4){
//            api=1002;
//            euler_angles[0]=0.0;
//        }else if(mythread.residenceTransientProcess==5){
//            api=1021;
//            velocity[0]=0.15;//前进
//            velocity[2]=0.002*(goalAverage-average);//转向
//        }else if(mythread.residenceTransientProcess==6){
//                        mythread.mode=track;
//               mythread.residenceTransientProcess=1;
//                }
//    }
//    if(mythread.mode==residenceleft){
//        if(mythread.residenceTransientProcess==1){
//            velocity[0]=0.08;
//        }else if(mythread.residenceTransientProcess==2){
//            api=1006;
//            velocity[0]=0.0;
//        }else if(mythread.residenceTransientProcess==3){
//            api=1002;
//            euler_angles[0]=-1.5;
//        }else if(mythread.residenceTransientProcess==4){
//            api=1002;
//            euler_angles[0]=0.0;
//        }else if(mythread.residenceTransientProcess==5){
//            api=1021;
//            velocity[0]=0.15;//前进
//            velocity[2]=0.002*(goalAverage-average);//转向
//        }else if(mythread.residenceTransientProcess==6){
//                        mythread.mode=track;
//               mythread.residenceTransientProcess=1;
//                }
//    }
    if(mythread.mode==residenceright){
        if(mythread.residenceTransientProcess==1){
            velocity[0]=0.06;
        }else if(mythread.residenceTransientProcess==2){
            api=1006;
            velocity[0]=0.0;
        }else if(mythread.residenceTransientProcess==3){
            api=1002;
            euler_angles[0]=1.5;
        }else if(mythread.residenceTransientProcess==4){
            api=1002;
            euler_angles[0]=0.0;
        }else if(mythread.residenceTransientProcess==5){
                        mythread.mode=track;
               mythread.residenceTransientProcess=1;
                }
    }
    if(mythread.mode==residenceleft){
        if(mythread.residenceTransientProcess==1){
            velocity[2]=-0.30;
        }else if(mythread.residenceTransientProcess==2){
            velocity[0]=0.08;
        }else if(mythread.residenceTransientProcess==2){
            api=1006;
            velocity[0]=0.0;
        }else if(mythread.residenceTransientProcess==3){
            api=1002;
            euler_angles[0]=-1.5;
        }else if(mythread.residenceTransientProcess==4){
            api=1002;
            euler_angles[0]=0.0;
        }else if(mythread.residenceTransientProcess==5){
                        mythread.mode=track;
               mythread.residenceTransientProcess=1;
                }
    }
    if(mythread.mode==divergeright){
        if(mythread.divergeTransientProcess==1){
            goalAverage=200;
            average=getAverage(frame);
            api=1021;
            velocity[0]=0.2;//前进
            velocity[1]=0.0005*(average-goalAverage);//横移
            velocity[2]=0.004*(goalAverage-average);//转向
        }
        if(mythread.divergeTransientProcess==2){
        goalAverage=180;
        average=getAverage(frame);
        api=1021;
        velocity[0]=0.2;
        velocity[2]=-0.20;
        }
    }if(mythread.mode==divergeleft){
        if(mythread.divergeTransientProcess==1){
            goalAverage=200;
            average=getAverage(frame);
            api=1021;
            velocity[0]=0.2;//前进
            velocity[1]=0.0005*(average-goalAverage);//横移
            velocity[2]=0.004*(goalAverage-average);//转向
        } if(mythread.divergeTransientProcess==2){
            goalAverage=180;
            average=getAverage(frame);
            api=1021;
            velocity[0]=0.2;
            velocity[2]=0.08;
            }
    }
    /******************************************************循迹模式****************************************************************/
    else if(mythread.mode==track){
       api=1021;
        /************************彩色条带识别**************************/
        goalAverage=200;
        if(nextColor=="blue"){
            inRange(ZoomOutimage,colorgroup.blueMin,colorgroup.blueMax,frame2);//二值化蓝色
            number=0;
            for(int i=0;i<frame2.rows;i++){
                for(int j=0;j<frame2.cols;j++){
                    if(frame2.at<uchar>(i,j)==255){
                        number++;
                    }
                }
            }
            //出现蓝色条带
            if(number>=500){
                mythread.mode=limitHeight;
                nextColor="null";
                cout<<"recognize blue"<<endl;
                return;
            }
        }  
        /************************识别分岔路、住户区颜色****************************/
        //绿色分岔路
        if(divergerColor=="green"){
            inRange(ZoomOutimage,colorgroup.greenMin,colorgroup.greenMax,frame2);//二值化绿色
            number=0;
            for(int i=0;i<frame2.rows;i++){
                for(int j=0;j<frame2.cols;j++){
                    if(frame2.at<uchar>(i,j)==255){
                        number++;
                    }
                }
            }
            //出现绿色条带
            if(number>=500){
                mythread.mode=divergeleft;
                cout<<"recognize green"<<endl;
                divergerColor="null";
                return;
            }
        }
        //红色住户识别
        else if(divergerColor=="red"){
                inRange(ZoomOutimage,colorgroup.redMin,colorgroup.redMax,frame2);//二值化红色
                number=0;
                for(int i=0;i<frame2.rows;i++){
                    for(int j=0;j<frame2.cols;j++){
                        if(frame2.at<uchar>(i,j)==255){
                            number++;
                        }
                    }
                }
                //出现红色条带
                if(number>=500){
                    mythread.mode=divergeright;
                    cout<<"recognize red"<<endl;
                    divergerColor="null";
                    return;
                 }
         }
        //棕色住户识别
        else if(residenceColor=="brown"){
            inRange(ZoomOutimage,colorgroup.brownMin,colorgroup.brownMax,frame2);//二值化棕色
            number=0;
            for(int i=0;i<frame2.rows;i++){
                for(int j=0;j<frame2.cols;j++){
                    if(frame2.at<uchar>(i,j)==255){
                        number++;
                    }
                }
            }
            //出现棕色条带
            if(number>=500){
                mythread.mode=residenceleft;
                cout<<"recognize brown"<<endl;
                residenceColor="null";
                return;
            }
            //紫色住户识别
        }else if(residenceColor=="violet"){
            inRange(ZoomOutimage,colorgroup.violetMin,colorgroup.violetMax,frame2);//二值化红色
            number=0;
            for(int i=0;i<frame2.rows;i++){
                for(int j=0;j<frame2.cols;j++){
                    if(frame2.at<uchar>(i,j)==255){
                        number++;
                    }
                }
            }
            //出现紫色条带
            if(number>=500){
                mythread.mode=residenceright;
                cout<<"recognize violet"<<endl;
                residenceColor="null";
                return;
            }
        }
        //提取赛道两侧边缘，求均值
        average=getAverage(frame);
        api=1021;
        euler_angles[0]=0;euler_angles[1]=0;euler_angles[2]=0;
        velocity[0]=0.2;//前进
        velocity[1]=0.0005*(average-goalAverage);//横移
        velocity[2]=0.004*(goalAverage-average);//转向
    }
    /******************************************************位于集散区矫正自身位置********************************************************/
    else if(mythread.mode==revise){
        int left[480]={0};//存储左侧边缘
        float middle[480]={0};//存储中线
        int right[480]={0};//存储右侧边缘
        int value[150]={0};//存储每个点对应的x轴对应的值
        int position[150]={0};//存储每个点对应的y轴对应的值
        float k=0;//中线斜率
        float sum=0;
        float goalK=0;//集散区矫正模式下预设k值,//目标中线斜率
        goalAverage=200;//集散区矫正模式下预设均值
        /************************提取赛道两侧边缘，并求取中线**************************/
        //提取赛道左侧边缘
        for(int i=0;i<frame.rows;i++){
            for(int j=0;j<frame.cols-3;j++){
                if(judgmentLeft(frame,i,j)){
                    if(j==0)
                        left[i]=1;
                    else
                        left[i]=j;
                    break;
                }
            }
        }
        //提取赛道右侧边缘
        for(int i=0;i<frame.rows;i++){
            for(int j=frame.cols-1;j>=3;j--){
                if(judgmentRight(frame,i,j)){
                    right[i]=j;
                    break;
                }
            }
        }
        //提取中线,并将中线绘制到图上
        number=0;
        for(int i=0;i<(frame.rows/2.0);i++){
            if((left[i]<(left[i+1]+4))&&(left[i]>(left[i+1]-4))&&(left[i]!=0))
                if((right[i]<(right[i+1]+4))&&(right[i]>(right[i+1]-4))&&(right[i]!=0)){
                    middle[i]=(left[i]+right[i])/2.0;
                    frame.at<uchar>(i,(int)middle[i])=0;
                    value[number]=middle[i];
                    position[number]=i;
                    number++;
                }
            // cout<<middle[i]<<endl;
        }
        //计算中线平均值,and k
        sum=0;
        k=0;
        if(number!=0){
            if(number%2==0){//偶数
                for(int i=0;i<=(number/2-1);i++){
                    sum=sum+(value[i]-value[i+number/2])/(float)(position[i]-position[i+number/2]);
                }
                k=sum/(number/2.0);
            }else if(number%2==1&&k>=3){//奇数
                for(int i=0;i<((number-1)/2);i++){
                    sum=sum+(value[i]-value[i+(number-1)/2+1])/(float)(position[i]-position[i+(number-1)/2+1]);
                }
                k=sum/((number-1)/2.0);
            }
            //限幅
            if(k>=0.5){
                k=0.5;
            }else if(k<=-0.5){
                k=-0.5;
            }

            for(int i=0;i<number;i++){
                sum=sum+value[i];
            }
            average=sum/number;
            //限幅
            if(average>=400){
                average=200;
            }else if(average<0){
                average=200;
            }
        }
        //    cout<<"average:"<<average<<endl;
        //   cout<<"k:"<<k<<endl;
        //计算输出
        velocity[0]=0;//前进
        velocity[1]=0.002*(goalAverage-average);//横移
        velocity[2]=0.8*(k-goalK);//转向
        if(abs(k-goalK)<0.15&&abs(goalAverage-average)<0.15){
            mythread.mode=import;
            api=1006;
        }
    }

    /******************************************************位于限高区****************************************************************/
    else if(mythread.mode==limitHeight){
        if(mythread.limitHeightTransientProcess==1){
           body_height=-0.10;
        }else if(mythread.limitHeightTransientProcess==2){
            body_height=-0.15;
        }else if(mythread.limitHeightTransientProcess==3){
            body_height=0;
        }
        //提取赛道两侧边缘，并求取中线
        average=getAverage(frame);
        api=1021;
        euler_angles[0]=0;euler_angles[1]=0;euler_angles[2]=0;
        velocity[0]=0.2;//前进
        velocity[1]=0.0005*(average-goalAverage);//横移
        velocity[2]=0.003*(goalAverage-average);//转向
    }
    /******************************************************上楼梯****************************************************************/
    else if(mythread.mode==upstair){
        if(mythread.upstairTransientProcess==1){
           api=1002;
        }else if(mythread.upstairTransientProcess==2){
           api=1101;
        }else if(mythread.upstairTransientProcess==3){
           api=1021;
        }
        //提取赛道两侧边缘，并求取中线
        average=getAverage(frame);
        goalAverage=200;
        euler_angles[0]=0;euler_angles[1]=0;euler_angles[2]=0;
        velocity[0]=0.2;//前进
        velocity[1]=0.0005*(average-goalAverage);//横移
        velocity[2]=0.004*(goalAverage-average);//转向
       }
    /******************************************************停止**************************************************************/
    else if(mythread.mode==stop){
        api=1006;
        velocity[0]=0.0;
    }
}
//打印程序当前模式
void logMode(){
    static int count=0;
    count++;
    if(count%20==1){
        switch (mythread.mode) {
        case import:
            cout<<"mode:"<<"import"<<endl;
            break;
        case residenceright:
            cout<<"mode:"<<"residenceright"<<endl;
            break;
        case residenceleft:
            cout<<"mode:"<<"residenceleft"<<endl;
            break;
        case divergeright:
            cout<<"mode:"<<"divergeright"<<endl;
            break;
        case divergeleft:
            cout<<"mode:"<<"divergeleft"<<endl;
            break;
        case track:
            cout<<"mode:"<<"track"<<endl;
            break;
        case revise:
            cout<<"mode:"<<"revise"<<endl;
            break;
        case limitHeight:
            cout<<"mode:"<<"limitHeight"<<endl;
            break;
        case upslope:
            cout<<"mode:"<<"upslope"<<endl;
            break;
        case obstruct:
            cout<<"Mode:"<<"obstruct"<<endl;
            break;
        }
        cout<<"count:"<<count/20<<endl;
        cout<<"residenceColor:"<<residenceColor<<endl;
        cout<<"divergeColor:"<<residenceColor<<endl;
        cout<<"nextColor:"<<nextColor<<endl;
        cout<<"velocity[0]:"<<velocity[0]<<endl;
        cout<<"velocity[1]:"<<velocity[1]<<endl;
        cout<<"velocity[2]:"<<velocity[2]<<endl;
        cout<<"euler_angles[0]:"<<euler_angles[0]<<endl;
        cout<<"euler_angles[1]:"<<euler_angles[1]<<endl;
        cout<<"euler_angles[2]:"<<euler_angles[2]<<endl;
        cout<<"api:"<<api<<endl;
    }
}

int main(int argc, char* argv[]){
    //udp接收线程，用于接收上位机发送来的信息
    UdpUtil udpsocket;
    /*以下程序用于根据输入参数的选择更改运行模式，以及选择是否将图像发送至上位机*/
    //according to the parameters passed in,choose mode
    //inputParameters1与inputParameters2是输入的两个参数
    QString inputParameters1;
    QString inputParameters2;
    bool flag=false;//该标志位用于是否显示图像
    //无参数输入
    if(argc==1){
        printf("using default parameter mode is track\n");
        printf("mode is track\n");
        printf("default residenceColor is red\n");
        mythread.mode=track;
        residenceColor="brown";//******
    }
    //输入参数大于等于两个
    else if(argc>=2){
        inputParameters1=argv[1];
        //track 模式
        if(inputParameters1=="track"){
            printf("mode is track\n");
            printf("default residenceColor is null\n");
            mythread.mode=track;
        }else if(inputParameters1=="brown"){
            printf("mode is track\n");
            printf("input residenceColor is brown\n");
            mythread.mode=track;
            residenceColor="brown";
        }else if(inputParameters1=="violet"){
            printf("mode is track\n");
            printf("input residenceColor is violet\n");
            mythread.mode=track;
            residenceColor="violet";
        }
        else if(inputParameters1=="stop"){
            printf("mode is stop\n");
            mythread.mode=stop;
        }else{
            mythread.mode=track;
            printf("parameters error\n");
            printf("mode is track\n");
        }

     if(argc==3){
            inputParameters2=argv[2];
            if(inputParameters2=="red"){
            printf("mode is track\n");
            printf("input divergerColor is red\n");
            mythread.mode=track;
            divergerColor="red";
             }
            else if(inputParameters2=="green"){
                printf("mode is track\n");
                printf("input divergerColor is green\n");
                mythread.mode=track;
                divergerColor="green";
        }
            else if(inputParameters2=="showImage"){
                    printf("start to show image\n");
                    flag=true;
                    udpsocket.start();}
            }
    }
    //打开摄像头
    for(int i=0;i<=3;i++){
        cap.open(i);
        if(!cap.isOpened())
        {
            printf("打开摄像头失败:number=%d\n",i);
            if(i==3)
                return 0;
        }else {
            printf("打开摄像头成功:number=%d\n",i);
            break;
        }
    }

    //创建lcm发送对象
    lcmUtil *lcmutil=new lcmUtil;
    //计时线程开启
    mythread.start();
    //计算程序运行时间
    QTime time;
    api=1006;
    lcmutil->send(api);
    lcmutil->send(velocity,euler_angles,body_height, step_height);
    QThread::msleep(2000);
    api=1002;
    lcmutil->send(api);
    lcmutil->send(velocity,euler_angles,body_height, step_height);
    QThread::msleep(2000);
    QTime time2;
    //图像处理以及通过lcm发送指令
    while(1){
        //摄像头重启
        if(cap.get(CAP_PROP_HUE)==-1){
            //cout<<"摄像头断开"<<endl;
            while (1) {
                QThread::msleep(100);
                for(int i=0;i<=3;i++){
                    cap.open(i);
                    if(cap.isOpened()){
                        break;
                    }
                }
                if(cap.isOpened()){
                    break;
                }
            }
        }
 //       time.start();
        //导入图像
        cap >> srcImage;
        //进行图像处理,并发送指令
        if( ! srcImage.empty()){
            imageProcess(srcImage);
            //发送步态指令,v_des为全局变量,在imageprocess中发生改变
            lcmutil->send(api);
            lcmutil->send(velocity,euler_angles,body_height, step_height);
            logMode();//打印状态
            //show or send picture
            if(flag){
                //first parameter is imput image
                //second parameter  is output destination
                //second parameter is 0 ,output destination is native computer
                //second parameter is 1 ,output destination is upper computer
                colorgroup.showPicture(srcImage,1);
                colorgroup.start();//thread1
                if(udpsocket.ifReceiveInfoFlag!=0){
                    switch (udpsocket.ifReceiveInfoFlag){
                    case 1://choose color and return this color threadhold
                        std::cout<<"choose color and return this color threadhold"<<std::endl;
                        colorgroup.chooseColor(udpsocket.color);
                        colorgroup.sendColorThreadhold();
                        colorgroup.ifRunContinueFlag=true;
                        break;
                    case 2://set color threadhold
                        std::cout<<"set color threadhold"<<std::endl;
                        colorgroup.setColorThreadhold(udpsocket.colorThreadhold);
                        break;
                    case 3://save color threadhold
                        std::cout<<"save color threadhold"<<std::endl;
                        colorgroup.save();
                        break;
                    }
                    udpsocket.ifReceiveInfoFlag=0;
                }
            }
        }
        //增加延时，增加至70ms,以降低对cpu资源的占用
        QThread::msleep(20);
        // std::cout<<"主线程"<<time2.elapsed()<<"ms"<<std::endl;
    }

    printf("程序运行结束\n");
    cap.release();
    return 0;
}
