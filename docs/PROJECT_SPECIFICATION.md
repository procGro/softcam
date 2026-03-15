# Virtual Media Injection & Realistic Camera Engine
**Project Specification & Detailed Explanation**

---

## 1. Executive Summary

The **Virtual Camera Engine** transforms the baseline `softcam` DirectShow filter from a simple raw pixel-pusher into a robust, feature-rich virtual camera capable of high-fidelity video injection and realistic physical camera simulation.

This engine is designed to operate as a command-line utility that decodes media files (MP4, MKV, AVI, PNG, JPG) using **FFmpeg**, applies sensor-accurate visual filters and geometric transformations using **OpenCV**, and seamlessly broadcasts the processed frames to the virtual DirectShow device. The core objective is to create a stream that is indistinguishable from a physical hardware sensor when viewed by standard conferencing software, while providing powerful deployment and obfuscation tools.

---

## 2. Technical Architecture & Data Pipeline

The system is built on a C++ core, bridging FFmpeg for media decoding and OpenCV for image processing, interfacing directly with the existing `softcam` API.

**The Pipeline:**
`Source File -> FFmpeg (Decode/Scale) -> OpenCV (Filters/PTZ) -> softcam (scSendFrame) -> DirectShow Filter -> Target App`

1.  **MediaReader (FFmpeg):** Opens the file, extracts the video stream, handles decoding (including `AVERROR(EAGAIN)` buffer states), and uses `libswscale` to resize and letterbox the source to match the target virtual camera resolution (e.g., 720p/1080p). It automatically seeks to frame `0` upon reaching EOF for seamless, infinite looping.
2.  **Processing Loop (main.cpp):** Manages a high-resolution `std::chrono` "slack" timer. If processing a frame takes 4ms, the thread sleeps for the exact remainder of the 16.6ms interval (for 60fps) to ensure the OS recognizes a stable, hardware-like clock rhythm.
3.  **LensModel & VirtualPTZ (OpenCV):** The raw RGB buffer is converted to a `cv::Mat` where spatial, chromatic, and geometric distortions are applied.
4.  **Softcam API:** The final processed buffer is dispatched to the DirectShow filter via `scSendFrame`.

---

## 3. Core Components

### A. Media Injection Engine (`MediaReader`)
*   **Format Support:** Decodes any format supported by `libavcodec` (mp4, mkv, avi, jpg, png).
*   **Seamless Looping:** Uses `avformat_seek_file` to reset the stream instantly upon `AVERROR_EOF`, preventing dropped frames or black screen flickering.
*   **Scaling & Letterboxing:** Automatically computes the aspect ratio of the source file and scales it to fit the virtual camera canvas (`VIRTUAL_CAM_WIDTH` x `VIRTUAL_CAM_HEIGHT`), padding the empty space with black bars.
*   **Buffer Stability:** Fixes standard FFmpeg decoding hangups by properly handling B-frame buffering (`AVERROR(EAGAIN)`).

### B. Realistic Camera Manipulation (`LensModel`)
The "Realism Layer" mimics the physical imperfections of cheap CMOS sensors and cheap glass lenses.
*   **Radial Distortion:** Simulates lens curvature (barrel/pincushion distortion). The transformation maps (`cv::initUndistortRectifyMap`) are heavily cached to ensure `< 16ms` processing times.
*   **Chromatic Aberration:** Displaces the Red and Blue color channels horizontally/vertically toward the edges of the frame to mimic light refraction imperfections.
*   **Sensor Noise:** Injects a randomly generated, low-opacity Gaussian noise matrix (`cv::randn`) that regenerates on *every single frame*, providing the "static" movement characteristic of live digital sensors.
*   **Auto-Exposure Flicker:** Subtly oscillates the overall image brightness (±1%) via HSV channel manipulation to simulate a cheap webcam constantly adjusting to light.

### C. Geometric Transformation (`VirtualPTZ`)
*   **Virtual PTZ (Pan, Tilt, Zoom):** Uses affine and perspective transformations (`cv::warpPerspective`) to manipulate the image.
*   It utilizes `cv::INTER_CUBIC` interpolation to maintain realistic image sharpness during deep digital zooms, as opposed to pixelated linear or nearest-neighbor approaches.

---

## 4. Stealth & Deployment Features

To evade generic "Virtual Camera" detection algorithms in corporate or automated proctoring environments, the engine includes specific deployment tools.

### A. Device Spoofing (`obfuscate_softcam.ps1`)
A PowerShell script that modifies the core C++ `softcam` source files *prior* to compilation.
*   It generates a fresh, random `CLSID` (GUID) for the DirectShow filter.
*   It replaces the generic "DirectShow Softcam" device name with a stealthy alias like "Integrated Webcam" or "USB 2.0 HD UVC WebCam".

### B. The "Live Freeze" Command
A distinct thread listens to `std::cin` for a `freeze` command.
*   Unlike pausing a video (which looks like a frozen, crashed application), this feature freezes the *source frame* but **continues to apply active Sensor Noise**.
*   This results in an image that looks completely authentic—a live physical camera pointed at a completely still scene.

---

## 5. Comprehensive Usage Guide

### Step 1: Obfuscation & Core Compilation (Windows)
Before building the engine, obfuscate and build the underlying DirectShow filter.

```powershell
# Navigate to the repository root
cd scripts
# Run the obfuscation script
.\obfuscate_softcam.ps1 -FriendlyName "Integrated Camera"
```
Once obfuscated, open `softcam.sln` in Visual Studio 2022 and build the **Release | x64** target.

### Step 2: Engine Compilation
Ensure CMake, OpenCV (extracted to `C:\opencv`), and FFmpeg (via MSYS2 `ucrt64`) are installed.

```powershell
cd src/engine
mkdir build
cd build
cmake .. -DOpenCV_DIR="C:\opencv\build"
cmake --build . --config Release
```

*(Note: On Linux, the CMake script compiles in "Stub Mode", bypassing the DirectShow APIs for core logic testing.)*

### Step 3: Running the Engine
The engine requires a path to a media file and accepts several optional parameters to tweak the realism.

**Basic Usage:**
```cmd
.\virtual_camera_engine.exe C:\Media\interview_loop.mp4
```

**Advanced Usage (Applying PTZ and Lens Distortion):**
```cmd
.\virtual_camera_engine.exe C:\Media\interview_loop.mp4 --zoom 1.25 --distortion 0.08 --noise 0.07
```
*   `--zoom 1.25`: Crops in on the video by 25%.
*   `--distortion 0.08`: Applies a noticeable "fisheye" barrel distortion to the image.
*   `--noise 0.07`: Increases the intensity of the live CMOS sensor grain.

### Step 4: Interactive Commands
While the engine is running and broadcasting to the virtual camera, you can type commands directly into the console window:

*   Type `freeze` and press Enter: The video playback will pause, but the sensor grain will continue to animate over the frozen frame, simulating a live camera staring at a static subject.
*   Type `unfreeze` and press Enter: Video playback resumes seamlessly.
*   Type `exit` and press Enter: Gracefully shuts down the engine and disconnects the virtual camera.

---

## 6. Success Metrics & Validation
*   **Latency:** The cached `LensModel` operations ensure that the end-to-end processing (Decode -> Scale -> PTZ -> Distort -> CA -> Noise -> Send) executes comfortably within the 16.6ms window required for 60fps streams.
*   **Memory Stability:** The pre-allocated `std::vector` RGB buffers and proper FFmpeg `av_packet_unref` memory management ensure the engine can run infinitely looping streams for >24 hours with a completely flat memory profile.
