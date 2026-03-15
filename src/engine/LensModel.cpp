#include "LensModel.h"
#include <random>

LensModel::LensModel() {}

void LensModel::ApplyDistortion(cv::Mat& frame, double k1, double k2) {
    // Basic implementation for radial distortion
    // Need camera matrix and dist coeffs
    cv::Mat cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
    cameraMatrix.at<double>(0, 0) = frame.cols; // fx
    cameraMatrix.at<double>(1, 1) = frame.cols; // fy
    cameraMatrix.at<double>(0, 2) = frame.cols / 2.0; // cx
    cameraMatrix.at<double>(1, 2) = frame.rows / 2.0; // cy

    cv::Mat distCoeffs = (cv::Mat_<double>(4, 1) << k1, k2, 0, 0);
    cv::Mat newCameraMatrix = cv::getOptimalNewCameraMatrix(cameraMatrix, distCoeffs, frame.size(), 1, frame.size(), 0);

    cv::Mat map1, map2;
    cv::initUndistortRectifyMap(cameraMatrix, distCoeffs, cv::Mat(), newCameraMatrix, frame.size(), CV_16SC2, map1, map2);

    cv::Mat output;
    cv::remap(frame, output, map1, map2, cv::INTER_LINEAR);
    frame = output;
}

void LensModel::ApplyChromaticAberration(cv::Mat& frame, int shift_x, int shift_y) {
    if (frame.empty()) return;
    std::vector<cv::Mat> channels;
    cv::split(frame, channels);

    // Assuming BGR format
    cv::Mat shifted_b, shifted_r;

    cv::Mat translation_matrix_b = (cv::Mat_<double>(2, 3) << 1, 0, shift_x, 0, 1, shift_y);
    cv::warpAffine(channels[0], shifted_b, translation_matrix_b, frame.size(), cv::INTER_LINEAR, cv::BORDER_REPLICATE);

    cv::Mat translation_matrix_r = (cv::Mat_<double>(2, 3) << 1, 0, -shift_x, 0, 1, -shift_y);
    cv::warpAffine(channels[2], shifted_r, translation_matrix_r, frame.size(), cv::INTER_LINEAR, cv::BORDER_REPLICATE);

    channels[0] = shifted_b;
    channels[2] = shifted_r;

    cv::merge(channels, frame);
}

void LensModel::AddSensorNoise(cv::Mat& frame, double intensity) {
    if (m_noise_buffer.size() != frame.size() || m_noise_buffer.type() != frame.type()) {
        m_noise_buffer = cv::Mat(frame.size(), frame.type());
    }
    cv::randn(m_noise_buffer, cv::Scalar::all(0), cv::Scalar::all(intensity * 255.0));
    cv::add(frame, m_noise_buffer, frame, cv::noArray(), frame.type());
}

void LensModel::ApplyAutoExposureFlicker(cv::Mat& frame, double base_brightness, double fluctuation) {
    static std::mt19937 gen(1337);
    std::uniform_real_distribution<> dis(-fluctuation, fluctuation);
    double flicker = dis(gen);

    cv::Mat hsv;
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
    std::vector<cv::Mat> channels;
    cv::split(hsv, channels);

    double factor = base_brightness + flicker;
    channels[2].convertTo(channels[2], -1, factor, 0);

    cv::merge(channels, hsv);
    cv::cvtColor(hsv, frame, cv::COLOR_HSV2BGR);
}
