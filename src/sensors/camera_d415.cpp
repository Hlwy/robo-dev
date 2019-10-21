#include <iostream>
#include <unistd.h>                // For usleep
#include <limits>                  // For infinity
#include "sensors/camera_d415.h"

using namespace std;

CameraD415::CameraD415(bool show_features){
     int err;
     bool verbose = true;
     vector<rs2::device> devList = this->get_available_devices(show_features, true);
     this->_dev = devList[0];
     this->_device_name = this->_dev.get_info(RS2_CAMERA_INFO_NAME);

     RS_STREAM_CFG rgb_cfg = {RS2_STREAM_COLOR, _cwidth, _cheight, RS2_FORMAT_BGR8, _cfps};
     RS_STREAM_CFG depth_cfg = {RS2_STREAM_DEPTH, _dwidth, _dheight, RS2_FORMAT_Z16, _dfps};
     std::vector<RS_STREAM_CFG> cfgs = {rgb_cfg, depth_cfg};

     if(this->start(cfgs)) printf("SUCCESS: CameraD415 initialized!\r\n");
     else exit(0);

     // rs2_stream align_to = RS2_STREAM_COLOR;
     // rs2::align align(align_to);
     this->_align = new rs2::align(RS2_STREAM_COLOR);
     this->_Dmat = cv::Mat::zeros(5, 1, CV_64F);

     err = this->get_intrinsics(RS2_STREAM_DEPTH,&this->_Kdepth, &this->_Pdepth);
     this->_fxd = _Kdepth.at<double>(0);
     this->_fyd = _Kdepth.at<double>(4);
     this->_ppxd = _Kdepth.at<double>(2);
     this->_ppyd = _Kdepth.at<double>(5);

     err = this->get_intrinsics(RS2_STREAM_COLOR,&this->_Krgb, &this->_Prgb);
     this->_fxc = _Krgb.at<double>(0);
     this->_fyc = _Krgb.at<double>(4);
     this->_ppxc = _Krgb.at<double>(2);
     this->_ppyc = _Krgb.at<double>(5);

     float dscale = this->get_depth_scale(verbose);
     float baseline = this->get_baseline(verbose);
}

CameraD415::CameraD415(int rgb_fps, int rgb_resolution[2], int depth_fps, int depth_resolution[2], bool show_features){
     int err;
     bool verbose = true;
     vector<rs2::device> devList = this->get_available_devices(show_features, true);
     this->_dev = devList[0];
     this->_device_name = this->_dev.get_info(RS2_CAMERA_INFO_NAME);

     this->_cfps = rgb_fps;
     this->_cwidth = rgb_resolution[0];
     this->_cheight = rgb_resolution[1];
     this->_dfps = depth_fps;
     this->_dwidth = depth_resolution[0];
     this->_dheight = depth_resolution[1];

     RS_STREAM_CFG rgb_cfg = {RS2_STREAM_COLOR, _cwidth, _cheight, RS2_FORMAT_BGR8, _cfps};
     RS_STREAM_CFG depth_cfg = {RS2_STREAM_DEPTH, _dwidth, _dheight, RS2_FORMAT_Z16, _dfps};
     std::vector<RS_STREAM_CFG> cfgs = {rgb_cfg, depth_cfg};

     if(this->start(cfgs)) printf("SUCCESS: CameraD415 initialized!\r\n");
     else exit(0);

     // rs2_stream align_to = RS2_STREAM_COLOR;
     // rs2::align align(align_to);
     // this->_align = align;
     this->_align = new rs2::align(RS2_STREAM_COLOR);
     this->_Dmat = cv::Mat::zeros(5, 1, CV_64F);

     err = this->get_intrinsics(RS2_STREAM_DEPTH,&this->_Kdepth, &this->_Pdepth);
     this->_fxd = _Kdepth.at<double>(0);
     this->_fyd = _Kdepth.at<double>(4);
     this->_ppxd = _Kdepth.at<double>(2);
     this->_ppyd = _Kdepth.at<double>(5);

     err = this->get_intrinsics(RS2_STREAM_COLOR,&this->_Krgb, &this->_Prgb);
     this->_fxc = _Krgb.at<double>(0);
     this->_fyc = _Krgb.at<double>(4);
     this->_ppxc = _Krgb.at<double>(2);
     this->_ppyc = _Krgb.at<double>(5);

     float dscale = this->get_depth_scale(verbose);
     float baseline = this->get_baseline(verbose);
}

CameraD415::~CameraD415(){
     this->stop();
     delete this->_align;
}

bool CameraD415::stop(){
     this->_pipe.stop();
     return true;
}

bool CameraD415::start(std::vector<RS_STREAM_CFG> stream_cfgs){
     bool err = this->reset(stream_cfgs, false);
     usleep(5.0 * 1000000);
     // Try initializing camera hardware
     if(!this->hardware_startup(stream_cfgs)){
          // Attempt to initialize hardware with a reset, if unsuccessful initialization
          if(!this->reset(stream_cfgs, true)){
               // if unsuccessful initialization after two reset attempts give up
               if(!this->reset(stream_cfgs, true)){
                    printf("[ERROR] Could not initialize CameraD415 object!\r\n");
                    return false;
               }
          }
     }
     return true;
}

bool CameraD415::sensors_startup(std::vector<RS_STREAM_CFG> stream_cfgs){
     try{
          rs2::config cfg;
          for(std::vector<RS_STREAM_CFG>::iterator it = stream_cfgs.begin(); it != stream_cfgs.end(); ++it){
               cfg.enable_stream((*it).stream_type, (*it).width, (*it).height, (*it).format, (*it).fps);
          }
          this->_cfg = cfg;
     } catch(const rs2::error & e){
          std::cerr << "[ERROR] CameraD415::start() --- Could not initialize rs2::config. RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
          return false;
     }
     return true;
}

bool CameraD415::hardware_startup(std::vector<RS_STREAM_CFG> stream_cfgs){
     bool success = this->sensors_startup(stream_cfgs);
     if(!success) return false;

     try{
          rs2::pipeline pipe;
          this->_pipe = pipe;
     } catch(const rs2::error & e){
          std::cerr << "[ERROR] CameraD415::start() --- Could not initialize rs2::pipeline. RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
          return false;
     }

     try{
          rs2::pipeline_profile profile = this->_pipe.start(this->_cfg);
          this->_profile = profile;
     } catch(const rs2::error & e){
          std::cerr << "[ERROR] CameraD415::start() --- Could not initialize rs2::pipeline_profile. RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
          return false;
     }
     return true;
}

bool CameraD415::reset(std::vector<RS_STREAM_CFG> stream_cfgs, bool with_startup){
     if(this->_dev) this->_dev.hardware_reset();
     else{
          printf("[ERROR] CameraD415::reset() --- No device to reset.\r\n");
          return false;
     }
     if(with_startup) return this->start(stream_cfgs);
     else return true;
}

int CameraD415::get_intrinsics(rs2_stream stream_type, cv::Mat* K, cv::Mat* P, bool verbose){
     int err;
     cv::Mat _K = cv::Mat::zeros(3, 3, CV_64F);
     cv::Mat _P = cv::Mat::zeros(3, 4, CV_64F);

     if(this->_profile){
          rs2::video_stream_profile tmp_stream = this->_profile.get_stream(stream_type).as<rs2::video_stream_profile>();
          rs2_intrinsics intr = tmp_stream.get_intrinsics();

          _K.at<double>(0) = intr.fx;
          _K.at<double>(2) = intr.ppx;
          _K.at<double>(4) = intr.fy;
          _K.at<double>(5) = intr.ppy;
          _K.at<double>(8) = 1.0;

          _P.at<double>(0) = _K.at<double>(0);
          _P.at<double>(2) = _K.at<double>(2);
          _P.at<double>(5) = _K.at<double>(4);
          _P.at<double>(6) = _K.at<double>(5);
          _P.at<double>(10) = 1;

          if(verbose){
               printf("[INFO] CameraD415::get_intrinsics() --- Intrinsic Properties:\r\n");
               printf("\tSize ---------- [w, h]: %d, %d\r\n",tmp_stream.width(),tmp_stream.height());
               printf("\tFocal Length -- [X, Y]: %.2f, %.2f\r\n",intr.fx,intr.fy);
               printf("\tPrinciple Point [X, Y]: %.2f, %.2f\r\n",intr.ppx,intr.ppy);
               std::cout << "K = "<< std::endl << " "  << _K << std::endl << std::endl;
          }
          err = 1;
     } else{ err = -1; }
     *K = _K;
     *P = _P;
     return err;
}

void CameraD415::get_extrinsics(bool verbose){
     if(this->_profile){
          rs2::stream_profile depth_stream = this->_profile.get_stream(RS2_STREAM_DEPTH);
          rs2::stream_profile rgb_stream = this->_profile.get_stream(RS2_STREAM_COLOR);
          rs2_extrinsics extr = depth_stream.get_extrinsics_to(rgb_stream);
     }
}

float CameraD415::get_baseline(bool verbose){
     if(this->_profile){
          // rs2::stream_profile ir1_stream = this->_profile.get_stream(RS2_STREAM_INFRARED, 1);
          // rs2::stream_profile ir2_stream = this->_profile.get_stream(RS2_STREAM_INFRARED, 2);
          // rs2_extrinsics extr = ir1_stream.get_extrinsics_to(ir2_stream);
          rs2::stream_profile depth_stream = this->_profile.get_stream(RS2_STREAM_DEPTH);
          rs2::stream_profile rgb_stream = this->_profile.get_stream(RS2_STREAM_COLOR);
          rs2_extrinsics extr = depth_stream.get_extrinsics_to(rgb_stream);

          float baseline = extr.translation[0];
          this->_baseline = baseline;
          if(verbose) printf("[INFO] CameraD415::get_baseline() --- Baseline = %f\r\n",baseline);
          return baseline;
     }else{ return -1.0;}
}

float CameraD415::get_depth_scale(bool verbose){
     if(!this->_profile){
          printf("[ERROR] CameraD415::get_depth_scale() --- Camera Profile is not initialized.\r\n");
          return -1.0;
     } else{
          rs2::depth_sensor sensor = this->_profile.get_device().first<rs2::depth_sensor>();
          float scale = sensor.get_depth_scale();
          this->_dscale = scale;
          if(verbose) printf("[INFO] CameraD415::get_depth_scale() --- Depth Scale = %f\r\n",scale);
          return scale;
     }
}

rs2::frame CameraD415::get_rgb_frame(bool flag_aligned){
     rs2::frame output;
     if((flag_aligned) && (this->_aligned_frames)) output = this->_aligned_frames.first(RS2_STREAM_COLOR);
     else if(this->_frames) output = this->_frames.first(RS2_STREAM_COLOR);
     return output;
}

int CameraD415::get_rgb_image(rs2::frame frame, cv::Mat* image){
     if(frame){
          this->_color_frame = frame;
          cv::Mat tmp(cv::Size(_cwidth, _cheight), CV_8UC3, (void*)frame.get_data(), cv::Mat::AUTO_STEP);
          *image = tmp;
          return 0;
     } else{
          printf("[WARN] CameraD415::get_rgb_image() ---- Retrieved frame is empty.\r\n");
          cv::Mat tmp = cv::Mat::zeros(_cwidth, _cheight, CV_8UC3);
          *image = tmp;
          return -1;
     }
}

int CameraD415::get_rgb_image(cv::Mat* image, bool flag_aligned){
     rs2::frame tmpFrame = this->get_rgb_frame(flag_aligned);
     if(tmpFrame){
          this->_color_frame = tmpFrame;
          cv::Mat tmp(cv::Size(_cwidth, _cheight), CV_8UC3, (void*)tmpFrame.get_data(), cv::Mat::AUTO_STEP);
          *image = tmp;
          return 0;
     } else{
          printf("[WARN] CameraD415::get_rgb_image() ---- Retrieved frame is empty.\r\n");
          cv::Mat tmp = cv::Mat::zeros(_cwidth, _cheight, CV_8UC3);
          *image = tmp;
          return -1;
     }
}

rs2::frame CameraD415::get_depth_frame(bool flag_aligned){
     rs2::frame output;
     if((flag_aligned) && (this->_aligned_frames)) output = this->_aligned_frames.first(RS2_STREAM_DEPTH);
     else if(this->_frames) output = this->_frames.first(RS2_STREAM_DEPTH);
     return output;
}

int CameraD415::get_depth_image(rs2::frame frame, cv::Mat* image){
     if(frame){
          this->_depth_frame = frame;
          cv::Mat tmp(cv::Size(_dwidth, _dheight), CV_16UC1, (void*)frame.get_data(), cv::Mat::AUTO_STEP);
          *image = tmp;
          return 0;
     } else{
          printf("[WARN] CameraD415::get_depth_image() ---- Retrieved frame is empty.\r\n");
          cv::Mat tmp = cv::Mat::zeros(_dwidth, _dheight, CV_16UC1);
          *image = tmp;
          return -1;
     }
}

int CameraD415::get_depth_image(cv::Mat* image, bool flag_aligned){
     rs2::frame tmpFrame = this->get_depth_frame(flag_aligned);
     if(tmpFrame){
          this->_depth_frame = tmpFrame;
          cv::Mat tmp(cv::Size(_dwidth, _dheight), CV_16UC1, (void*)tmpFrame.get_data(), cv::Mat::AUTO_STEP);
          *image = tmp;
          return 0;
     } else{
          printf("[WARN] CameraD415::get_depth_image() ---- Retrieved frame is empty.\r\n");
          cv::Mat tmp = cv::Mat::zeros(_dwidth, _dheight, CV_16UC1);
          *image = tmp;
          return -1;
     }
}

int CameraD415::get_pointcloud(){
     // rs2::pointcloud pc;
     // // We want the points object to be persistent so we can display the last cloud when a frame drops
     // rs2::points points;
     // pc.map_to(color);
     // auto depth = frames.get_depth_frame();
     //
     // // Generate the pointcloud and texture mappings
     // points = pc.calculate(depth);
     return 0;
}

vector<cv::Mat> CameraD415::read(bool flag_aligned){
     cv::Mat _rgb, _depth;
     vector<cv::Mat> imgs;
     // printf("[INFO] CameraD415::update_frames() --- Updating frames...\r\n");
     this->_frames = this->_pipe.wait_for_frames();
     if(flag_aligned) this->_aligned_frames = this->_align->process(this->_frames);

     int errRgb = this->get_rgb_image(&_rgb, flag_aligned);
     if(errRgb >= 0) this->_nRgbFrames++;
     int errDepth = this->get_depth_image(&_depth, flag_aligned);
     if(errDepth >= 0) this->_nDepthFrames++;

     int err = errRgb + errDepth;
     if(err < 0) printf("[WARN] CameraD415::read() ---- One or more of the retrieved images are empty.\r\n");
     imgs.push_back(_rgb);
     imgs.push_back(_depth);
     return imgs;
}

int CameraD415::read(cv::Mat* rgb, cv::Mat* depth, bool flag_aligned){
     cv::Mat _rgb, _depth;
     // printf("[INFO] CameraD415::update_frames() --- Updating frames...\r\n");
     this->_frames = this->_pipe.wait_for_frames();
     if(flag_aligned) this->_aligned_frames = this->_align->process(this->_frames);

     int errRgb = this->get_rgb_image(&_rgb, flag_aligned);
     if(errRgb >= 0) this->_nRgbFrames++;
     int errDepth = this->get_depth_image(&_depth, flag_aligned);
     if(errDepth >= 0) this->_nDepthFrames++;

     *rgb = _rgb;
     *depth = _depth;
     int err = errRgb + errDepth;
     if(err < 0) printf("[WARN] CameraD415::read() ---- One or more of the retrieved images are empty.\r\n");
     return err;
}

cv::Mat CameraD415::convert_to_disparity(const cv::Mat depth, double* conversion_gain){
     cv::Mat dm, disparity8;
     double minVal, maxVal;
     // std::cout << depth.type() << std::endl;
     depth.convertTo(dm, CV_64F);
     cv::Mat tmp = dm*this->_dscale;
     cv::Mat mask = cv::Mat(tmp == 0);
     tmp.setTo(1, mask);
     cv::Mat disparity = (this->_fxd * this->_baseline) / tmp;

     disparity.setTo(0, mask);
     // disparity.setTo(0, disparity == std::numeric_limits<int>::quiet_NaN());
     // disparity.setTo(0, disparity == std::numeric_limits<int>::infinity());
     minMaxLoc(disparity, &minVal, &maxVal);
     double gain = 256.0 / maxVal;

     disparity.convertTo(disparity8,CV_8U,gain);
     *conversion_gain = gain;
     return disparity8;
}


void CameraD415::update(){
     // if (!aligned_depth_frame || !other_frame){ continue;}
}

vector<rs2::device> CameraD415::get_available_devices(bool show_features, bool verbose){
     vector<rs2::device> devs;
     int index = 0;
     rs2::device_list devices = this->_ctx.query_devices();
     for(rs2::device device : devices){
          std::string name = "Unknown Device";
          std::string sn = "########";
          if(device.supports(RS2_CAMERA_INFO_NAME)) name = device.get_info(RS2_CAMERA_INFO_NAME);
          if(device.supports(RS2_CAMERA_INFO_SERIAL_NUMBER)) sn = std::string("#") + device.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
          if(verbose) std::cout << "  " << index++ << " : " << name + " " + sn << std::endl;
          if(show_features) get_available_sensors(device);
          devs.push_back(device);
     }
     return devs;
}

std::string CameraD415::get_sensor_name(const rs2::sensor& sensor){
     // Sensors support additional information, such as a human readable name
     if (sensor.supports(RS2_CAMERA_INFO_NAME)) return sensor.get_info(RS2_CAMERA_INFO_NAME);
     else return "Unknown Sensor";
}

void CameraD415::get_available_sensors(rs2::device dev){
     // Given a device, we can query its sensors using:
     std::vector<rs2::sensor> sensors = dev.query_sensors();

     std::cout << "Device consists of " << sensors.size() << " sensors:\n" << std::endl;
     int index = 0;
     // We can now iterate the sensors and print their names
     for(rs2::sensor sensor : sensors){
          std::cout << "  " << index++ << " : " << get_sensor_name(sensor) << std::endl;
     }

     uint32_t selected_sensor_index = get_user_selection("Select a sensor by index: ");

     // The second way is using the subscript ("[]") operator:
     if(selected_sensor_index >= sensors.size()){
          throw std::out_of_range("Selected sensor index is out of range");
     }

     get_sensor_option(sensors[selected_sensor_index]);
}

void CameraD415::get_sensor_option(const rs2::sensor& sensor){
     // Sensors usually have several options to control their properties
     //  such as Exposure, Brightness etc.

     std::cout << "Sensor supports the following options:\n" << std::endl;

     // The following loop shows how to iterate over all available options
     // Starting from 0 until RS2_OPTION_COUNT (exclusive)
     for(int i = 0; i < static_cast<int>(RS2_OPTION_COUNT); i++){
          rs2_option option_type = static_cast<rs2_option>(i);
          //SDK enum types can be streamed to get a string that represents them
          std::cout << "  " << i << ": " << option_type;

          // To control an option, use the following api:

          // First, verify that the sensor actually supports this option
          if(sensor.supports(option_type)){
               std::cout << std::endl;

               // Get a human readable description of the option
               const char* description = sensor.get_option_description(option_type);
               std::cout << "       Description   : " << description << std::endl;

               // Get the current value of the option
               float current_value = sensor.get_option(option_type);
               std::cout << "       Current Value : " << current_value << std::endl;

               //To change the value of an option, please follow the change_sensor_option() function
          } else{
               std::cout << " is not supported" << std::endl;
          }
     }

     uint32_t selected_sensor_option = get_user_selection("Select an option by index: ");
     if (selected_sensor_option >= static_cast<int>(RS2_OPTION_COUNT)){
          throw std::out_of_range("Selected option is out of range");
     }
     // return static_cast<rs2_option>(selected_sensor_option);
}

// ===================================================================================

// static void get_field_of_view(const rs2::stream_profile& stream)
//     {
//         // A sensor's stream (rs2::stream_profile) is in general a stream of data with no specific type.
//         // For video streams (streams of images), the sensor that produces the data has a lens and thus has properties such
//         //  as a focal point, distortion, and principal point.
//         // To get these intrinsics parameters, we need to take a stream and first check if it is a video stream
//         if (auto video_stream = stream.as<rs2::video_stream_profile>())
//         {
//             try
//             {
//                 //If the stream is indeed a video stream, we can now simply call get_intrinsics()
//                 rs2_intrinsics intrinsics = video_stream.get_intrinsics();
//
//                 auto principal_point = std::make_pair(intrinsics.ppx, intrinsics.ppy);
//                 auto focal_length = std::make_pair(intrinsics.fx, intrinsics.fy);
//                 rs2_distortion model = intrinsics.model;
//
//                 std::cout << "Principal Point         : " << principal_point.first << ", " << principal_point.second << std::endl;
//                 std::cout << "Focal Length            : " << focal_length.first << ", " << focal_length.second << std::endl;
//                 std::cout << "Distortion Model        : " << model << std::endl;
//                 std::cout << "Distortion Coefficients : [" << intrinsics.coeffs[0] << "," << intrinsics.coeffs[1] << "," <<
//                     intrinsics.coeffs[2] << "," << intrinsics.coeffs[3] << "," << intrinsics.coeffs[4] << "]" << std::endl;
//             }
//             catch (const std::exception& e)
//             {
//                 std::cerr << "Failed to get intrinsics for the given stream. " << e.what() << std::endl;
//             }
//         }
//         else if (auto motion_stream = stream.as<rs2::motion_stream_profile>())
//         {
//             try
//             {
//                 //If the stream is indeed a motion stream, we can now simply call get_motion_intrinsics()
//                 rs2_motion_device_intrinsic intrinsics = motion_stream.get_motion_intrinsics();
//
//                 std::cout << " Scale X      cross axis      cross axis  Bias X \n";
//                 std::cout << " cross axis    Scale Y        cross axis  Bias Y  \n";
//                 std::cout << " cross axis    cross axis     Scale Z     Bias Z  \n";
//                 for (int i = 0; i < 3; i++)
//                 {
//                     for (int j = 0; j < 4; j++)
//                     {
//                         std::cout << intrinsics.data[i][j] << "    ";
//                     }
//                     std::cout << "\n";
//                 }
//
//                 std::cout << "Variance of noise for X, Y, Z axis \n";
//                 for (int i = 0; i < 3; i++)
//                     std::cout << intrinsics.noise_variances[i] << " ";
//                 std::cout << "\n";
//
//                 std::cout << "Variance of bias for X, Y, Z axis \n";
//                 for (int i = 0; i < 3; i++)
//                     std::cout << intrinsics.bias_variances[i] << " ";
//                 std::cout << "\n";
//             }
//             catch (const std::exception& e)
//             {
//                 std::cerr << "Failed to get intrinsics for the given stream. " << e.what() << std::endl;
//             }
//         }
//         else
//         {
//             std::cerr << "Given stream profile has no intrinsics data" << std::endl;
//         }
//     }

// ===================================================================================

// void BaseRealSenseNode::publishStaticTransforms()
// {
//     rs2::stream_profile base_profile = getAProfile(_base_stream);
//
//     // Publish static transforms
//     if (_publish_tf)
//     {
//         for (std::pair<stream_index_pair, bool> ienable : _enable)
//         {
//             if (ienable.second)
//             {
//                 calcAndPublishStaticTransform(ienable.first, base_profile);
//             }
//         }
//         // Static transform for non-positive values
//         if (_tf_publish_rate > 0)
//             _tf_t = std::shared_ptr<std::thread>(new std::thread(boost::bind(&BaseRealSenseNode::publishDynamicTransforms, this)));
//         else
//             _static_tf_broadcaster.sendTransform(_static_tf_msgs);
//     }
//
//     // Publish Extrinsics Topics:
//     if (_enable[DEPTH] &&
//         _enable[FISHEYE])
//     {
//         static const char* frame_id = "depth_to_fisheye_extrinsics";
//         const auto& ex = base_profile.get_extrinsics_to(getAProfile(FISHEYE));
//
//         _depth_to_other_extrinsics[FISHEYE] = ex;
//         _depth_to_other_extrinsics_publishers[FISHEYE].publish(rsExtrinsicsToMsg(ex, frame_id));
//     }
//
//     if (_enable[DEPTH] &&
//         _enable[COLOR])
//     {
//         static const char* frame_id = "depth_to_color_extrinsics";
//         const auto& ex = base_profile.get_extrinsics_to(getAProfile(COLOR));
//         _depth_to_other_extrinsics[COLOR] = ex;
//         _depth_to_other_extrinsics_publishers[COLOR].publish(rsExtrinsicsToMsg(ex, frame_id));
//     }
//
//     if (_enable[DEPTH] &&
//         _enable[INFRA1])
//     {
//         static const char* frame_id = "depth_to_infra1_extrinsics";
//         const auto& ex = base_profile.get_extrinsics_to(getAProfile(INFRA1));
//         _depth_to_other_extrinsics[INFRA1] = ex;
//         _depth_to_other_extrinsics_publishers[INFRA1].publish(rsExtrinsicsToMsg(ex, frame_id));
//     }
//
//     if (_enable[DEPTH] &&
//         _enable[INFRA2])
//     {
//         static const char* frame_id = "depth_to_infra2_extrinsics";
//         const auto& ex = base_profile.get_extrinsics_to(getAProfile(INFRA2));
//         _depth_to_other_extrinsics[INFRA2] = ex;
//         _depth_to_other_extrinsics_publishers[INFRA2].publish(rsExtrinsicsToMsg(ex, frame_id));
//     }
//
// }

// ===================================================================================

// static void change_sensor_option(const rs2::sensor& sensor, rs2_option option_type)
//     {
//         // Sensors usually have several options to control their properties
//         //  such as Exposure, Brightness etc.
//
//         // To control an option, use the following api:
//
//         // First, verify that the sensor actually supports this option
//         if (!sensor.supports(option_type))
//         {
//             std::cerr << "This option is not supported by this sensor" << std::endl;
//             return;
//         }
//
//         // Each option provides its rs2::option_range to provide information on how it can be changed
//         // To get the supported range of an option we do the following:
//
//         std::cout << "Supported range for option " << option_type << ":" << std::endl;
//
//         rs2::option_range range = sensor.get_option_range(option_type);
//         float default_value = range.def;
//         float maximum_supported_value = range.max;
//         float minimum_supported_value = range.min;
//         float difference_to_next_value = range.step;
//         std::cout << "  Min Value     : " << minimum_supported_value << std::endl;
//         std::cout << "  Max Value     : " << maximum_supported_value << std::endl;
//         std::cout << "  Default Value : " << default_value << std::endl;
//         std::cout << "  Step          : " << difference_to_next_value << std::endl;
//
//         bool change_option = false;
//         change_option = prompt_yes_no("Change option's value?");
//
//         if (change_option)
//         {
//             std::cout << "Enter the new value for this option: ";
//             float requested_value;
//             std::cin >> requested_value;
//             std::cout << std::endl;
//
//             // To set an option to a different value, we can call set_option with a new value
//             try
//             {
//                 sensor.set_option(option_type, requested_value);
//             }
//             catch (const rs2::error& e)
//             {
//                 // Some options can only be set while the camera is streaming,
//                 // and generally the hardware might fail so it is good practice to catch exceptions from set_option
//                 std::cerr << "Failed to set option " << option_type << ". (" << e.what() << ")" << std::endl;
//             }
//         }
//     }
