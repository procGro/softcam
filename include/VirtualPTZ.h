#pragma once

#include <opencv2/opencv.hpp>

class VirtualPTZ {
public:
    VirtualPTZ();

    void ApplyTransform(cv::Mat& frame, double pan_x, double pan_y, double zoom, double tilt_x, double tilt_y);
};
