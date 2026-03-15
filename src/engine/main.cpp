#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <softcam/softcam.h>
#include <opencv2/opencv.hpp>
#include "MediaReader.h"
#include "LensModel.h"
#include "VirtualPTZ.h"

const int VIRTUAL_CAM_WIDTH = 1280;
const int VIRTUAL_CAM_HEIGHT = 720;
const float VIRTUAL_CAM_FPS = 60.0f;

std::atomic<bool> is_frozen(false);
std::atomic<bool> should_exit(false);

void CommandThread() {
    std::string command;
    while (!should_exit) {
        std::cin >> command;
        if (command == "freeze") {
            is_frozen = true;
            std::cout << "Feed frozen." << std::endl;
        } else if (command == "unfreeze") {
            is_frozen = false;
            std::cout << "Feed un-frozen." << std::endl;
        } else if (command == "exit" || command == "quit") {
            should_exit = true;
            break;
        }
    }
}

void PrintHelp(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " <media_file_path> [options]\n"
              << "Options:\n"
              << "  --zoom <factor>       Zoom factor (default: 1.0)\n"
              << "  --distortion <k1>     Radial distortion coefficient (default: 0.0)\n"
              << "  --noise <level>       Sensor noise intensity (default: 0.05)\n"
              << "  --help                Show this help message\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        PrintHelp(argv[0]);
        return 1;
    }

    std::string media_path;
    double zoom = 1.0;
    double radial_distortion_k1 = 0.0;
    double noise_level = 0.05;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            PrintHelp(argv[0]);
            return 0;
        } else if (arg == "--zoom" && i + 1 < argc) {
            zoom = std::stod(argv[++i]);
        } else if (arg == "--distortion" && i + 1 < argc) {
            radial_distortion_k1 = std::stod(argv[++i]);
        } else if (arg == "--noise" && i + 1 < argc) {
            noise_level = std::stod(argv[++i]);
        } else if (media_path.empty() && arg[0] != '-') {
            media_path = arg;
        }
    }

    if (media_path.empty()) {
        std::cerr << "Error: No media file specified." << std::endl;
        PrintHelp(argv[0]);
        return 1;
    }

    scCamera cam = scCreateCamera(VIRTUAL_CAM_WIDTH, VIRTUAL_CAM_HEIGHT, VIRTUAL_CAM_FPS);
    if (!cam) {
        std::cerr << "Failed to create virtual camera instance." << std::endl;
        return 1;
    }

    std::cout << "Starting playback (type 'freeze', 'unfreeze', or 'exit')..." << std::endl;

    MediaReader reader;
    if (!reader.Open(media_path, VIRTUAL_CAM_WIDTH, VIRTUAL_CAM_HEIGHT)) {
        std::cerr << "Failed to open media file." << std::endl;
        scDeleteCamera(cam);
        return 1;
    }

    // Pre-allocate buffers
    std::vector<uint8_t> frame_buffer(VIRTUAL_CAM_WIDTH * VIRTUAL_CAM_HEIGHT * 3, 0);
    cv::Mat frozen_frame;

    LensModel lens;
    VirtualPTZ ptz;

    std::thread cmd_thread(CommandThread);
    cmd_thread.detach();

    // Determine frame interval in microseconds based on source framerate
    double source_fps = reader.GetFramerate();
    if (source_fps <= 0.0) source_fps = 30.0;
    auto frame_interval = std::chrono::microseconds(static_cast<long long>(1000000.0 / source_fps));

    auto next_frame_time = std::chrono::steady_clock::now();

    while (!should_exit) {
        if (!is_frozen) {
            if (!reader.ReadFrame(frame_buffer)) {
                std::cerr << "Error reading frame or EOF reached." << std::endl;
                break;
            }

            cv::Mat frame_mat(VIRTUAL_CAM_HEIGHT, VIRTUAL_CAM_WIDTH, CV_8UC3, frame_buffer.data());

            if (zoom != 1.0) {
                ptz.ApplyTransform(frame_mat, 0.0, 0.0, zoom, 0.0, 0.0);
            }
            if (radial_distortion_k1 != 0.0) {
                lens.ApplyDistortion(frame_mat, radial_distortion_k1, 0.0);
            }

            // Apply CA after distortion for realism
            lens.ApplyChromaticAberration(frame_mat, 2, 0);

            // Auto exposure flicker
            lens.ApplyAutoExposureFlicker(frame_mat, 1.0, 0.01);

            // Keep frozen copy updated until frozen
            frame_mat.copyTo(frozen_frame);

            // Sensor noise on moving frame
            lens.AddSensorNoise(frame_mat, noise_level);

        } else {
            // Restore frozen frame into frame buffer so noise is applied to the static image
            if (!frozen_frame.empty()) {
                cv::Mat frame_mat(VIRTUAL_CAM_HEIGHT, VIRTUAL_CAM_WIDTH, CV_8UC3, frame_buffer.data());
                frozen_frame.copyTo(frame_mat);
                lens.AddSensorNoise(frame_mat, noise_level); // Noise on frozen frame
            }
        }

        scSendFrame(cam, frame_buffer.data());

        // Clock Sync - Slack timer
        next_frame_time += frame_interval;
        auto process_end_time = std::chrono::steady_clock::now();

        if (next_frame_time > process_end_time) {
            std::this_thread::sleep_for(next_frame_time - process_end_time);
        } else {
            next_frame_time = process_end_time; // We lagged
        }
    }

    scDeleteCamera(cam);
    return 0;
}
