# Virtual Camera Engine

A command-line engine that uses FFmpeg to read media files, applies OpenCV effects, and injects them into the DirectShow softcam filter.

## Building on Windows

### Prerequisites
1. **Visual Studio 2022** (Desktop development with C++).
2. **CMake**.
3. **OpenCV 4.x** (Extract to `C:\opencv`).
4. **FFmpeg Dev Libraries** (Easiest way is via MSYS2 `ucrt64` environment: `pacman -S mingw-w64-ucrt-x86_64-ffmpeg mingw-w64-ucrt-x86_64-pkgconf`).
5. Build the core softcam DLL first by opening `softcam.sln` and building the `Release` `x64` target.

### Build Steps

Ensure MSYS2 `pkg-config` is in your path so CMake can find FFmpeg.

```powershell
cd src/engine
mkdir build
cd build
cmake .. -DOpenCV_DIR="C:\opencv\build"
cmake --build . --config Release
```

The executable will be generated at `src/engine/build/Release/virtual_camera_engine.exe`.
Make sure `softcam.dll` and the OpenCV/FFmpeg DLLs are in the same directory or in your system PATH when running.

## Building on Linux (Stub Mode)
```bash
cd src/engine
mkdir build
cd build
cmake ..
make
```

## CLI Usage

```bash
virtual_camera_engine <media_file_path> [options]

Options:
  --zoom <factor>       Zoom factor (e.g., 1.2)
  --distortion <k1>     Radial distortion coefficient (e.g., 0.05)
  --noise <level>       Sensor noise intensity (e.g., 0.05)
  --help                Show this help message
```

## Interactive Commands
During playback, you can type the following commands and press Enter:
- `freeze`: Freezes the video feed on the current frame while maintaining the active stream and noise filter.
- `unfreeze`: Resumes the video feed.
- `exit` or `quit`: Stops the stream and exits the engine.

## Obfuscation
To avoid detection as a generic "softcam", run the obfuscation script before compiling the DirectShow filter:
```powershell
.\scripts\obfuscate_softcam.ps1 -FriendlyName "Integrated Webcam"
```
