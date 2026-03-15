#include "VirtualPTZ.h"

VirtualPTZ::VirtualPTZ() {}

void VirtualPTZ::ApplyTransform(cv::Mat& frame, double pan_x, double pan_y, double zoom, double tilt_x, double tilt_y) {
    if (frame.empty()) return;

    int width = frame.cols;
    int height = frame.rows;

    cv::Point2f src_pts[4] = {
        cv::Point2f(0, 0),
        cv::Point2f(width, 0),
        cv::Point2f(width, height),
        cv::Point2f(0, height)
    };

    // Apply zoom
    double zoomed_w = width / zoom;
    double zoomed_h = height / zoom;

    // Apply pan
    double center_x = width / 2.0 + pan_x;
    double center_y = height / 2.0 + pan_y;

    double left = center_x - zoomed_w / 2.0;
    double right = center_x + zoomed_w / 2.0;
    double top = center_y - zoomed_h / 2.0;
    double bottom = center_y + zoomed_h / 2.0;

    cv::Point2f dst_pts[4] = {
        cv::Point2f(left, top),
        cv::Point2f(right, top),
        cv::Point2f(right, bottom),
        cv::Point2f(left, bottom)
    };

    // Apply tilt (Perspective warp simulation)
    // Positive tilt_y makes top wider, bottom narrower
    dst_pts[0].x -= tilt_x;
    dst_pts[1].x += tilt_x;
    dst_pts[2].x -= tilt_x;
    dst_pts[3].x += tilt_x;

    dst_pts[0].y -= tilt_y;
    dst_pts[1].y -= tilt_y;
    dst_pts[2].y += tilt_y;
    dst_pts[3].y += tilt_y;

    cv::Mat trans_matrix = cv::getPerspectiveTransform(dst_pts, src_pts);

    cv::Mat output;
    cv::warpPerspective(frame, output, trans_matrix, frame.size(), cv::INTER_CUBIC, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
    frame = output;
}
