#include <stdio.h>
#include <string.h>
#include <iostream>

#include "imu.h"

using namespace std;

int main(int argc, char *argv[]){

	int dt;

	string path = "/home/hunter/devel/robo-dev/config/sensors";
	string file = "mpu9250";

     IMU imu(path, file);

	while(1){
		imu.update();
		dt = imu.get_update_period();
		usleep(dt);
		// cout << "Poll Interval: " << dt << endl;
	}

     return 0;
}