#include "MediaReader.h"
#include <iostream>
#include <algorithm>

MediaReader::MediaReader() {
}

MediaReader::~MediaReader() {
    Close();
}

bool MediaReader::Open(const std::string& filepath, int target_width, int target_height) {
    Close(); // Clean up if reopening

    m_filepath = filepath;
    m_target_width = target_width;
    m_target_height = target_height;

    // Open video file
    if (avformat_open_input(&m_format_ctx, filepath.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "Could not open input file: " << filepath << std::endl;
        return false;
    }

    // Retrieve stream information
    if (avformat_find_stream_info(m_format_ctx, nullptr) < 0) {
        std::cerr << "Could not find stream information" << std::endl;
        return false;
    }

    // Find the first video stream
    m_video_stream_index = -1;
    const AVCodec* codec = nullptr;
    for (unsigned int i = 0; i < m_format_ctx->nb_streams; i++) {
        const AVCodec* local_codec = avcodec_find_decoder(m_format_ctx->streams[i]->codecpar->codec_id);
        if (m_format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (!codec) {
                m_video_stream_index = i;
                codec = local_codec;
            }
        }
    }

    if (m_video_stream_index == -1) {
        std::cerr << "Could not find a video stream" << std::endl;
        return false;
    }

    // Get a pointer to the codec context for the video stream
    AVCodecParameters* codec_params = m_format_ctx->streams[m_video_stream_index]->codecpar;
    m_codec_ctx = avcodec_alloc_context3(codec);
    if (!m_codec_ctx) {
        std::cerr << "Could not allocate video codec context" << std::endl;
        return false;
    }

    if (avcodec_parameters_to_context(m_codec_ctx, codec_params) < 0) {
        std::cerr << "Could not copy codec context" << std::endl;
        return false;
    }

    // Open codec
    if (avcodec_open2(m_codec_ctx, codec, nullptr) < 0) {
        std::cerr << "Could not open codec" << std::endl;
        return false;
    }

    m_width = m_codec_ctx->width;
    m_height = m_codec_ctx->height;

    // Get Framerate
    AVRational framerate = m_format_ctx->streams[m_video_stream_index]->avg_frame_rate;
    if (framerate.den != 0) {
        m_framerate = (double)framerate.num / (double)framerate.den;
    } else {
        m_framerate = 30.0; // fallback
    }

    m_frame = av_frame_alloc();
    m_frame_rgb = av_frame_alloc();
    m_packet = av_packet_alloc();

    if (!m_frame || !m_frame_rgb || !m_packet) {
        std::cerr << "Could not allocate frame or packet" << std::endl;
        return false;
    }

    // Determine scaling to maintain aspect ratio with letterboxing
    double aspect_ratio = (double)m_width / m_height;
    double target_aspect_ratio = (double)m_target_width / m_target_height;

    int scaled_width, scaled_height;
    if (aspect_ratio > target_aspect_ratio) {
        scaled_width = m_target_width;
        scaled_height = (int)(m_target_width / aspect_ratio);
    } else {
        scaled_height = m_target_height;
        scaled_width = (int)(m_target_height * aspect_ratio);
    }

    // Ensure scaled dimensions are even (swscale prefers this)
    scaled_width -= scaled_width % 2;
    scaled_height -= scaled_height % 2;

    m_sws_ctx = sws_getContext(
        m_width, m_height, m_codec_ctx->pix_fmt,
        scaled_width, scaled_height, AV_PIX_FMT_BGR24, // DirectShow softcam expects BGR24
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    if (!m_sws_ctx) {
        std::cerr << "Could not initialize the conversion context" << std::endl;
        return false;
    }

    // Allocate buffer for RGB frame (using scaled dimensions first)
    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_BGR24, scaled_width, scaled_height, 1);
    m_rgb_buffer = (uint8_t*)av_malloc(num_bytes * sizeof(uint8_t));

    av_image_fill_arrays(m_frame_rgb->data, m_frame_rgb->linesize, m_rgb_buffer, AV_PIX_FMT_BGR24, scaled_width, scaled_height, 1);

    return true;
}

bool MediaReader::ReadFrame(std::vector<uint8_t>& output_rgb24) {
    if (!m_format_ctx || !m_codec_ctx) return false;

    int read_ret;
    while (true) {
        read_ret = av_read_frame(m_format_ctx, m_packet);
        if (read_ret == AVERROR_EOF) {
            // End of file reached, loop back to the beginning
            avformat_seek_file(m_format_ctx, m_video_stream_index, 0, 0, 0, AVSEEK_FLAG_FRAME);
            avcodec_flush_buffers(m_codec_ctx);
            continue; // Read again
        } else if (read_ret < 0) {
            return false;
        }

        // Is this a packet from the video stream?
        if (m_packet->stream_index == m_video_stream_index) {
            // Decode video frame
            int response = avcodec_send_packet(m_codec_ctx, m_packet);
            if (response < 0) {
                std::cerr << "Error while sending a packet to the decoder" << std::endl;
                av_packet_unref(m_packet);
                return false;
            }

            response = avcodec_receive_frame(m_codec_ctx, m_frame);
            if (response == AVERROR(EAGAIN)) {
                av_packet_unref(m_packet);
                continue;
            } else if (response == AVERROR_EOF) {
                av_packet_unref(m_packet);
                // Handle EOF from decoder (though handled by av_read_frame above usually)
                avformat_seek_file(m_format_ctx, m_video_stream_index, 0, 0, 0, AVSEEK_FLAG_FRAME);
                avcodec_flush_buffers(m_codec_ctx);
                continue;
            } else if (response < 0) {
                std::cerr << "Error while receiving a frame from the decoder" << std::endl;
                av_packet_unref(m_packet);
                return false;
            }

            // Frame is decoded. Convert to BGR24 and scale
            double aspect_ratio = (double)m_width / m_height;
            double target_aspect_ratio = (double)m_target_width / m_target_height;

            int scaled_width, scaled_height;
            if (aspect_ratio > target_aspect_ratio) {
                scaled_width = m_target_width;
                scaled_height = (int)(m_target_width / aspect_ratio);
            } else {
                scaled_height = m_target_height;
                scaled_width = (int)(m_target_height * aspect_ratio);
            }
            scaled_width -= scaled_width % 2;
            scaled_height -= scaled_height % 2;

            sws_scale(m_sws_ctx, (uint8_t const* const*)m_frame->data, m_frame->linesize, 0, m_height,
                      m_frame_rgb->data, m_frame_rgb->linesize);

            // Letterbox to target resolution
            output_rgb24.assign(m_target_width * m_target_height * 3, 0); // Initialize with black

            int x_offset = (m_target_width - scaled_width) / 2;
            int y_offset = (m_target_height - scaled_height) / 2;

            for (int y = 0; y < scaled_height; ++y) {
                int src_index = y * m_frame_rgb->linesize[0];
                int dst_index = ((y + y_offset) * m_target_width + x_offset) * 3;
                std::copy(m_rgb_buffer + src_index, m_rgb_buffer + src_index + scaled_width * 3, output_rgb24.begin() + dst_index);
            }

            av_packet_unref(m_packet);
            return true;
        }

        // Free the packet that was allocated by av_read_frame
        av_packet_unref(m_packet);
    }
    return false;
}

void MediaReader::Close() {
    if (m_rgb_buffer) {
        av_freep(&m_rgb_buffer);
        m_rgb_buffer = nullptr;
    }
    if (m_frame_rgb) {
        av_frame_free(&m_frame_rgb);
        m_frame_rgb = nullptr;
    }
    if (m_frame) {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }
    if (m_packet) {
        av_packet_free(&m_packet);
        m_packet = nullptr;
    }
    if (m_codec_ctx) {
        avcodec_free_context(&m_codec_ctx);
        m_codec_ctx = nullptr;
    }
    if (m_format_ctx) {
        avformat_close_input(&m_format_ctx);
        m_format_ctx = nullptr;
    }
    if (m_sws_ctx) {
        sws_freeContext(m_sws_ctx);
        m_sws_ctx = nullptr;
    }
}
