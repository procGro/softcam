#pragma once

#include <opencv2/opencv.hpp>

class LensModel {
public:
    LensModel();

    void ApplyDistortion(cv::Mat& frame, double k1, double k2);
    void ApplyChromaticAberration(cv::Mat& frame, int shift_x, int shift_y);
    void AddSensorNoise(cv::Mat& frame, double intensity);
    void ApplyAutoExposureFlicker(cv::Mat& frame, double base_brightness, double fluctuation);

private:
    cv::Mat m_noise_buffer;
};
