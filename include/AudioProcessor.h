//
// Created by wang on 2020/4/17.
//

#ifndef CPLAYER_AUDIOPROCESSOR_H
#define CPLAYER_AUDIOPROCESSOR_H

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

typedef struct AudioInfo{
    uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
    int out_nb_samples;
    AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    int out_sample_rate = 44100;
    int out_channels;
} AudioInfo;

class AudioProcessor{
public:
    AudioProcessor(AVFormatContext* format);
    int init();
    void decode_audio_frame();
    int open_codec();
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

    struct SwrContext *get_swr_ctx(){
        return au_convert_ctx;
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

    void update_pts(int buf_size){
        int bytes_per_sec = codecCtx->sample_rate * codecCtx->channels * 2;
        auto pts = static_cast<double>(buf_size) / bytes_per_sec;
        clock -= pts;
    }

    AudioInfo audio_info{};
    uint8_t *out_buffer = nullptr;
    mutex audio_play;
    condition_variable audio_cond;

private:
    AVFormatContext *avFormatCtx;
    int stream_idx = -1;
    int max_deque_size = 10;
    deque<AVPacket> packet_que = {};
    mutex packet_que_mutex{};
    AVCodecContext *codecCtx = nullptr;
    struct SwrContext *au_convert_ctx = nullptr;
    bool closed = false;
    int64_t clock = 0;
    mutex clock_mutex;
    AVRational stream_time_base;
};

#endif //CPLAYER_AUDIOPROCESSOR_H
