//
// Created by wang on 2020/4/8.
//
#include "Grabber.h"
#define MAX_AUDIO_FRAME_SIZE 192000

Grabber::Grabber(char *file_name){
    input_file = file_name;
    avFormatCtx = avformat_alloc_context();
}

int Grabber::init(){

    if(avformat_open_input(&avFormatCtx, input_file, NULL, NULL) != 0){
        printf("Couldn't open input stream.\n");
        return -1;
    }
    if(avformat_find_stream_info(avFormatCtx,NULL) < 0){
        printf("Couldn't find stream information.\n");
        return -1;
    }

    return 0;
}

int Grabber::grab_packet(AVPacket *packet){
    if (av_read_frame(avFormatCtx, packet) >= 0) {
//        cout << "read a packet" << endl;
    }else{
        cout << "stream ends." << endl;
        stream_eof = true;
        return -1;
    }
    return 0;
}

void Grabber::free_all() {
    avformat_close_input(&avFormatCtx);
}
