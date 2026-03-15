# Virtual Camera Engine

A command-line engine that uses FFmpeg to read media files, applies OpenCV effects, and injects them into the DirectShow softcam filter.

## Building

Requires FFmpeg and OpenCV. Build with CMake:
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
