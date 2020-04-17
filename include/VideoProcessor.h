//
// Created by wang on 2020/4/17.
//

#ifndef CPLAYER_VIDEOPROCESSOR_H
#define CPLAYER_VIDEOPROCESSOR_H

#include <iostream>
#include <deque>
#include <mutex>
#include <thread>
#include <condition_variable>

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}

using namespace std;

class VideoProcessor{
public:
    VideoProcessor(AVFormatContext* format);
    int init();
    void decode_video_frame();
    int open_codec(AVCodecContext **desc);
    void free_all();

    bool need_new_packet(){
        return packet_que.size() < 3;
    }

    void push_packet(AVPacket *packet){
        std::lock_guard<std::mutex> lock(packet_que_mutex);
        packet_que.push_back(*packet);
    }

    void get_packet(AVPacket *packet){
        std::lock_guard<std::mutex> lock(packet_que_mutex);
        *packet = packet_que.front();
        packet_que.pop_front();
    }

    AVCodecContext *get_codec_ctx(){
        return codecCtx;
    };

    double get_frame_rate(){
        auto frame_rate = codecCtx->framerate;
        double fr = frame_rate.num && frame_rate.den ? av_q2d(frame_rate) : 25.0;
        return fr;
    }

    struct SwsContext *get_sws_ctx(){
        return img_convert_ctx;
    }

    int packet_que_size(){
        return packet_que.size();
    };

    int get_stream_idx(){
        return stream_idx;
    }

    void set_closed(){
        closed = true;
    }

    bool get_closed(){
        return closed;
    }

    int64_t get_pts(){
        std::lock_guard<std::mutex> lock(clock_mutex);
        return clock;
    }

    void set_pts(int64_t pts){
        std::lock_guard<std::mutex> lock(clock_mutex);
        clock = pts;
    }

    AVFrame *frame = nullptr;

private:
    AVFormatContext *avFormatCtx;
    int stream_idx = -1;
    int max_deque_size = 10;
    deque<AVPacket> packet_que = {};
    mutex packet_que_mutex{};
    AVCodecContext *codecCtx = nullptr;
    struct SwsContext *img_convert_ctx = nullptr;
    bool closed = false;
    int64_t clock = 0;
    mutex clock_mutex;
    AVRational stream_time_base;
};

#endif //CPLAYER_VIDEOPROCESSOR_H
