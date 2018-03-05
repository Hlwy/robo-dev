#ifndef DUAL_ROBOCLAW_H_
#define DUAL_ROBOCLAW_H_

#include <stdarg.h>
#include <string>
#include <vector>
#include "base/params.h"
#include "devices/roboclaw.h"

using namespace std;

class DualClaw {

private:

     int _pi;
     int _ser_handle;
     int _address[2];
     uint8_t _status[4];

     float _max_speed;
     float _base_width;
     int _qpps_per_meter;

     bool _valid[4];

public:

     // FUNCTIONS
     DualClaw(int pi);
     ~DualClaw();

     uint16_t error[8];
     uint16_t main_battery;
     int16_t currents[4];

     uint32_t qpps[4];
     float kp[4];
     float ki[4];
     float kd[4];

     uint32_t speeds[4];
     uint32_t positions[4];

     RoboClaw* leftclaw;
     RoboClaw* rightclaw;

     vector<int32_t> update_commands(float v, float w);
     void drive(vector<int32_t> cmds);

     // TODO: develop
     void update_status();
     void update_readings();
     void keep_alive();
     void reset_encoders();
};



#endif // DUAL_ROBOCLAW_H_
