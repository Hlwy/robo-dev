#include <string.h>
#include <iostream>
#include <unistd.h>

#include "base/definitions.h"
#include "imu.h"

static const float G_TO_MPSS = 9.80665;
static const int uT_TO_T = 1000000;

using namespace std;

IMU::IMU(){}

IMU::IMU(string path, string file){

     num_updates = 0;
     declination_offset = -0.6;

     int err = init(path, file);
     if(err < 0){
          exit(1);
     }

}

IMU::~IMU(){

     delete _imu;
     delete _settings;

}

int IMU::init(string path, string file){

     calib_path = path;
     calib_file = file;

     RTIMUSettings* tmpSettings = new RTIMUSettings(path.c_str(), file.c_str());
     RTIMU* tmpImu = RTIMU::createIMU(tmpSettings);

     if( (tmpImu == NULL) || (tmpImu->IMUType() == RTIMU_TYPE_NULL)){
          printf("ERROR: No Imu found, or could not be opened!\r\n");
          delete tmpSettings;
          delete tmpImu;
          return -1;
     }else{
          _settings = tmpSettings;
          _imu = tmpImu;
          printf("SUCCESS: Imu opened, attempting to initialize...\r\n");
     }

     if(!_imu->IMUInit()){
          printf("ERROR: Imu could not be initialized!\r\n");
          delete _settings;
          delete _imu;
          return -2;
     }

     // Set the Fusion coefficient
     _imu->setSlerpPower(0.02);
     // Enable the sensors
     _imu->setGyroEnable(true);
     _imu->setAccelEnable(true);
     _imu->setCompassEnable(true);

     // Get current system time for later
     time_start = RTMath::currentUSecsSinceEpoch();

     printf("SUCCESS: Imu initialized!\r\n");
     return 1;
}

int IMU::get_update_period(){
     return _imu->IMUGetPollInterval() * 1000;
}

void IMU::update(){

	while(_imu->IMURead()){
          // TODO: Potentially need to update timestamp for this section
          // TODO: Add if-statements for valid readings

          num_updates++;
          RTIMU_DATA data = _imu->getIMUData();

          now = RTMath::currentUSecsSinceEpoch();

          accel[0] = data.accel.x() * G_TO_MPSS;
          accel[1] = data.accel.y() * G_TO_MPSS;
          accel[2] = data.accel.z() * G_TO_MPSS;

          gyro[0] = data.gyro.x();
          gyro[1] = data.gyro.y();
          gyro[2] = data.gyro.z();

          quats[0] = data.fusionQPose.x();
		quats[1] = data.fusionQPose.y();
		quats[2] = data.fusionQPose.z();
		quats[3] = data.fusionQPose.scalar();

		if(data.compassValid){
			// TODO: Potentially need to update timestamp for this section

			mag[0] = data.compass.x() / uT_TO_T;
               mag[1] = data.compass.y() / uT_TO_T;
			mag[2] = data.compass.z() / uT_TO_T;
		}

          euler[0] = data.fusionPose.x();
          euler[1] = data.fusionPose.y();
          euler[2] = data.fusionPose.z();

          corrected_yaw = euler[2] - declination_offset;
	}

}

vector<float> IMU::get_raw_data(){
     vector<float> out;
     out.reserve(9);

     out.push_back(accel[0]);
     out.push_back(accel[1]);
     out.push_back(accel[2]);
     out.push_back(gyro[0]);
     out.push_back(gyro[1]);
     out.push_back(gyro[2]);
     out.push_back(mag[0]);
     out.push_back(mag[1]);
     out.push_back(mag[2]);

     return out;
}

vector<float> IMU::get_all_data(){
     vector<float> out;
     out.reserve(16);

     out.push_back(accel[0]);
     out.push_back(accel[1]);
     out.push_back(accel[2]);
     out.push_back(gyro[0]);
     out.push_back(gyro[1]);
     out.push_back(gyro[2]);
     out.push_back(mag[0]);
     out.push_back(mag[1]);
     out.push_back(mag[2]);

     out.push_back(quats[0]);
     out.push_back(quats[1]);
     out.push_back(quats[2]);
     out.push_back(quats[3]);

     float roll = fmod((euler[0]*M_RAD2DEG + 360.0),360.0);
     float pitch = fmod((euler[1]*M_RAD2DEG + 360.0),360.0);
     float yaw = fmod((euler[2]*M_RAD2DEG + 360.0),360.0);

     out.push_back(roll);
     out.push_back(pitch);
     out.push_back(yaw);

     return out;
}

void IMU::print_settings(){
     // Print out all configured parameters for debugging
     printf("IMU CONFIGURATION SETTINGS: \r\n");
     // printf("       Device Address: %s\r\n", _add);
     // printf("       Baud Rate: %d\r\n", _baud);
     // printf("       Imu Type: %.4f\r\n", _type);
     printf("\r\n");
}

void IMU::print_data(){
     fflush(stdout);
     printf("IMU DATA: \r\n");
     printf("       Accelerations (m/s^2): %.4f        %.4f      %.4f\r\n", accel[0], accel[1], accel[2]);
     printf("       Angular Velocities (rad/sec): %.4f        %.4f      %.4f\r\n", gyro[0], gyro[1], gyro[2]);
     printf("       Magnetometer (μT): %.4f        %.4f      %.4f\r\n", mag[0], mag[1], mag[2]);
     printf("       Fused Euler Angles (deg): %.4f        %.4f      %.4f\r\n\r\n", euler[0]*M_RAD2DEG, euler[1]*M_RAD2DEG, euler[2]*M_RAD2DEG);
}

void IMU::print_angles(){
     float tmpAng[3];

     for(int i = 0; i<3;i++){
          tmpAng[i] = euler[i] * M_RAD2DEG;
     }

     fflush(stdout);
     printf("       Fused Euler Angles (deg): %.4f        %.4f      %.4f\r\n\r\n", tmpAng[0], tmpAng[1], tmpAng[2]);
}
