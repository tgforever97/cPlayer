//
// Created by wang on 2020/4/9.
//

#ifndef CPLAYER_GRABBER_H
#define CPLAYER_GRABBER_H

#include <iostream>
#include <deque>
#include <mutex>
#include <thread>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

using namespace std;

class Grabber{
public:
    Grabber(char *file_name);
    int open_codec(AVCodecContext **dec_ctx, enum AVMediaType type);
    int init();
    int grab_frame();
    void return_frame(bool isImage, AVFrame *dec);
    void free_all();
    bool stream_end();
    AVCodecContext *get_video_ctx(){
        return videoCodecCtx;
    };
    AVCodecContext *get_audio_ctx(){
        return audioCodecCtx;
    };
    int video_que_size(){
        return video_que.size();
    };
    int audio_que_size(){
        return audio_que.size();
    };


private:
    char *input_file = nullptr;
    int video_idx = 0;
    int audio_idx = 0;
    bool stream_eof = false;
    AVFormatContext	*avFormatCtx = nullptr;
    AVCodecContext *videoCodecCtx = nullptr;
    AVCodecContext *audioCodecCtx = nullptr;
    AVPacket *packet = nullptr;
    int max_deque_size = 10;
    deque<AVFrame> video_que = {};
    mutex video_que_mutex{};
    deque<AVFrame> audio_que = {};
    mutex audio_que_mutex{};
};

#endif //CPLAYER_GRABBER_H
