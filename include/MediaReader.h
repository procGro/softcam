#pragma once

#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

class MediaReader {
public:
    MediaReader();
    ~MediaReader();

    bool Open(const std::string& filepath, int target_width, int target_height);
    bool ReadFrame(std::vector<uint8_t>& output_rgb24);
    void Close();

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    double GetFramerate() const { return m_framerate; }

private:
    std::string m_filepath;
    int m_target_width = 0;
    int m_target_height = 0;

    int m_width = 0;
    int m_height = 0;
    double m_framerate = 0.0;

    AVFormatContext* m_format_ctx = nullptr;
    AVCodecContext* m_codec_ctx = nullptr;
    int m_video_stream_index = -1;
    AVFrame* m_frame = nullptr;
    AVFrame* m_frame_rgb = nullptr;
    AVPacket* m_packet = nullptr;
    SwsContext* m_sws_ctx = nullptr;

    uint8_t* m_rgb_buffer = nullptr;
};
