//
// Created by wang on 2020/4/8.
//
#include "Grabber.h"

Grabber::Grabber(char *file_name){
    input_file = file_name;
    avFormatCtx = avformat_alloc_context();
    packet = av_packet_alloc();
    av_init_packet(packet);
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

    video_idx = open_codec(&videoCodecCtx, AVMEDIA_TYPE_VIDEO);

    audio_idx = open_codec(&audioCodecCtx, AVMEDIA_TYPE_AUDIO);

    return 0;
}

int Grabber::open_codec(AVCodecContext **dec_ctx, enum AVMediaType type){
    int stream_temp = -1;

    for(int i = 0; i < avFormatCtx->nb_streams; i++){
        if(avFormatCtx->streams[i]->codecpar->codec_type == type) {
            stream_temp = i;
            break;
        }
    }

    if(stream_temp == -1){
        printf("Didn't find a video or audio stream.\n");
        return -1;
    }

    AVCodecParameters *avCodeParameters = avFormatCtx->streams[stream_temp]->codecpar;
    AVCodec	*avCodec = avcodec_find_decoder(avCodeParameters->codec_id);
    if(avCodec == NULL){
        printf("Codec not found.\n");
        return -1;
    }
    *dec_ctx = avcodec_alloc_context3(avCodec);
    avcodec_parameters_to_context(*dec_ctx, avCodeParameters);
    if(avcodec_open2(*dec_ctx, avCodec,NULL)<0){
        printf("Could not open codec.\n");
        return -1;
    }
    return stream_temp;
}


int Grabber::grab_frame(){

    AVFrame *frame = av_frame_alloc();
    while(true){
        if (av_read_frame(avFormatCtx, packet) >= 0) {
            if (packet->stream_index == video_idx) {
                int got_picture = 0;
                int ret = avcodec_decode_video2(videoCodecCtx, frame, &got_picture, packet);
                if (ret < 0) {
                    printf("Decode Error.\n");
                    return -1;
                }
                if (got_picture) {
                    printf("video frame_count=%d\n", videoCodecCtx->frame_number);
                    std::lock_guard<std::mutex> lock(video_que_mutex);
                    video_que.push_back(*frame);
                    std::this_thread::sleep_for(std::chrono::milliseconds(40));
                }
            } else if (packet->stream_index == audio_idx) {
                int got_sound = 0;
                int ret = avcodec_decode_audio4(audioCodecCtx, frame, &got_sound, packet);
                if (ret < 0) {
                    printf("Decode Error.\n");
                    return -1;
                }
                if (got_sound) {
                    printf("audio frame_count=%d\n", audioCodecCtx->frame_number);
                    std::lock_guard<std::mutex> lock(audio_que_mutex);
                    audio_que.push_back(*frame);
                }
            }
        }else{
            stream_eof = true;
            break;
        }
//        }else{
//            cout << "queque is full." << endl;
//            std::this_thread::sleep_for(std::chrono::milliseconds(40));
//        }
    }
    cout << "stream ends." << endl;
    av_frame_free(&frame);
    return 0;
}

void Grabber::free_all() {
    avformat_close_input(&avFormatCtx);
    avcodec_free_context(&videoCodecCtx);
    avcodec_free_context(&audioCodecCtx);
    av_packet_free(&packet);
}

void Grabber::return_frame(bool isImage, AVFrame *dec) {
    if(isImage && !video_que.empty()){
        std::lock_guard<std::mutex> lock(video_que_mutex);
        *dec = video_que.front();
        video_que.pop_front();
    }else if(!isImage && !audio_que.empty()){
        std::lock_guard<std::mutex> lock(audio_que_mutex);
        *dec = audio_que.front();
        audio_que.pop_front();
    }else{
        cout << "queque is empty." << endl;
    }
}

bool Grabber::stream_end() {
    return stream_eof;
}
