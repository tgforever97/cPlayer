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
#include "libavformat/avformat.h"
}

using namespace std;

typedef struct Frame{
    AVFrame frame;
    int frame_count;
} Frame;

class Grabber{
public:
    Grabber(char *file_name);
    int init();
    int grab_packet(AVPacket *packet);
    void free_all();
    bool stream_end(){
        return stream_eof;
    }
    AVFormatContext	*get_ctx(){
        return avFormatCtx;
    }

private:
    char *input_file = nullptr;
    int video_idx = 0;
    int audio_idx = 0;
    bool stream_eof = false;
    AVFormatContext	*avFormatCtx = nullptr;
};

#endif //CPLAYER_GRABBER_H
