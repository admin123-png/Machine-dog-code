#ifndef LCMUTIL_H
#define LCMUTIL_H
#include <iostream>
#include <QString>
#include <lcm/lcm-cpp.hpp>
#include "sport_client_cmd_t.hpp"

class lcmUtil
{
public:
    lcmUtil();
    void send(double velocity[3],double euler_angles[3],double body_height, double step_height);
    void send(int32_t api);
    sport_client_cmd_t ctl;
};

#endif // LCMUTIL_H
