#include <iostream>
#include "base/definitions.h"
#include "sensors/bno055.h"
#include <pigpiod_if2.h>
using namespace std;

int main(int argc, char *argv[]){
	int pi;
	float angle;
	float dt = 0.05;
	int imu_status[3];
	int imu_revision[5];
	string ttyDev = "/dev/serial0";
	int baud = B115200;

	BNO055 imu;
	imu.init(ttyDev, baud);

	int err = imu.begin();
	if(err < 0)
		printf("[ERROR] BNO055::begin] ---- %d.\r\n", err);
	else
		printf("[SUCCESS] BNO-055 Initialized \r\n\r\n");
	// Check Status
	imu.get_system_status(&imu_status[0]);

	printf("===========    BNO-055 Status Response    =================\r\n");
     printf("BNO-055 Status:\r\n");
     printf("\tSystem Status: %d (%#x)\r\n", imu_status[0], imu_status[0]);
     printf("\tSelf-Test Result: %d (%#x)\r\n", imu_status[1], imu_status[1]);
     printf("\tSystem Error Status: %d (%#x)\r\n", imu_status[2], imu_status[2]);

	imu.get_revision(&imu_revision[0]);
	printf("===========    BNO-055 Software Revision Response    =================\r\n");
     printf("BNO-055 Software Revision:\r\n");
     printf("\tSoftware Version: %d (%#x)\r\n", imu_revision[0], imu_revision[0]);
     printf("\tBootloader Version: %d (%#x)\r\n", imu_revision[1], imu_revision[1]);
     printf("\tAccelerometer ID: %d (%#x)\r\n", imu_revision[2], imu_revision[2]);
     printf("\tGyroscope ID: %d (%#x)\r\n", imu_revision[3], imu_revision[3]);
     printf("\tMagnetometer ID: %d (%#x)\r\n", imu_revision[4], imu_revision[4]);

	printf("===========    BNO-055 Reading Euler   =================\r\n");
	float angs[3];
	while(1){
		imu.get_euler(&angs[0]);
		printf(" Euler Angles: %f, %f, %f\r\n", angs[0], angs[1] , angs[2]);
		// imu.update(true);
		usleep(dt * 1000000);
	}


     return 0;
}
