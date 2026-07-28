// file: send_message.cpp
//
// LCM example program.
//
// compile with:
//  $ g++ -o send_message send_message.cpp -llcm
//
// On a system with pkg-config, you can also use:
//  $ g++ -o send_message send_message.cpp `pkg-config --cflags --libs lcm`

#include <iostream>
#include <unistd.h>//include sleep module
#include "lcmutil.h"

using namespace std;

lcm::LCM SDUog_Parameter_LCM("udpm://239.255.76.67:7667?ttl=1");

lcmUtil::lcmUtil()
{
    //lcm::LCM lcm("udpm://239.255.76.67:7667?ttl=1");

    if( ! SDUog_Parameter_LCM.good()){
        cout << "LCM 初始化失败" << endl;
    }else{
        cout << "LCM 初始化成功" << endl;
    }
}

void lcmUtil::send(double velocity[3],double euler_angles[3],double body_height, double step_height){

    ctl.velocity[0]=velocity[0];//前进 <0.5
    ctl.velocity[1]=velocity[1];//横移 <abs(0.4)
    ctl.velocity[2]=velocity[2];//旋转 <abs(2)
    ctl.step_height=step_height;//提升至0.14即可
    ctl.body_height=body_height;//降低至0.2即可
    ctl.euler_angles[0]=euler_angles[0];//rpy_des[0](横滚角)
    ctl.euler_angles[1]=euler_angles[1];//rpy_des[1](俯仰角)
    ctl.euler_angles[2]=euler_angles[2];//rpy_des[1](俯仰角)
    SDUog_Parameter_LCM.publish("UPPER_dogCONTROL",&ctl);
}
void lcmUtil::send(int32_t api){
    ctl.api=api;
    SDUog_Parameter_LCM.publish("UPPER_dogCONTROL",&ctl);
}

