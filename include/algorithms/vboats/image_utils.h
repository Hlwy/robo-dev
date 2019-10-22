#ifndef VBOATS_IMAGE_UTILS_H_
#define VBOATS_IMAGE_UTILS_H_

#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

using namespace std;

int strip_image(const cv::Mat& input, vector<cv::Mat>* strips, int nstrips = 5, bool cut_horizontally = true, bool visualize=false, bool verbose=false);
int merge_strips(const vector<cv::Mat>& strips, cv::Mat* merged, bool merge_horizontally = true, bool visualize=false, bool verbose=false);


/** TODO */
// def histogram_sliding_filter(hist, window_size=16, flag_plot=False):

#endif // VBOATS_IMAGE_UTILS_H_
