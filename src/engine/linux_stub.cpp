#include <softcam/softcam.h>

extern "C" {
    scCamera scCreateCamera(int width, int height, float framerate) { return (void*)1; }
    void scDeleteCamera(scCamera camera) {}
    void scSendFrame(scCamera camera, const void* image_bits) {}
    bool scWaitForConnection(scCamera camera, float timeout) { return true; }
    bool scIsConnected(scCamera camera) { return true; }
}
