#include <iostream>

#include <pigpiod_if2.h>
#include "utilities/utils.h"
#include "robots/swansonV2.h"

using namespace std;

int flag_exit = 0;
SwansonV2* bot;

void funExit(int s){
     printf("[Ctrl+C] Shutting Down...\r\n");
	flag_exit = 1;
	usleep(1 * 1000000);
}

int main(int argc, char *argv[]){
     float max_vel = 1.5;
     float max_omega = max_vel / 0.381;

     int pi = pigpio_start(NULL, NULL); /* Connect to Pi. */
     attach_CtrlC(funExit);

     if (pi >= 0){

          bot = new SwansonV2(pi);
          bot->rc_in->set_verbose(0);

          while(1){

               // bot->read_udp_header();
               bot->receiveUdpMessage();

               // bot->read_udp_commands();
               // float accel = (float) bot->controls.speed / 1000000;
               // float omega = (float) bot->controls.yaw / 1000000;
               // float gain = (float) bot->controls.limit / 1000000;
               // accel = max_vel * accel;
               // omega = max_omega * omega;
               // cout << "Controls: " << accel << ",          " << omega << ",          " << gain << endl;

               // bot->drive(accel, omega);
               // bot->update_sensors();
               // vector<float> data = bot->get_sensor_data();
               // bot->add_datalog_entry(data);
               usleep(0.01 * 1000000);
               if(flag_exit == 1){
                    break;
               }
          }
          delete bot;
          pigpio_stop(pi);
     }

     return 0;
}
