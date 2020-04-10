//
// Created by wang on 2020/4/1.
//

#include "Grabber.h"
#include <thread>
extern "C"{
#include <SDL2/SDL_video.h>
}


extern int init_sdl();
extern int video_player(Grabber *grabber);
extern int audio_main(Grabber *grabber);


void test(Grabber *grabber) {
    AVFrame *frame = av_frame_alloc();
    while(!grabber->stream_end()){
        grabber->return_frame(true, frame);
        printf("video frame %lld\n", frame->width);
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }
    av_frame_free(&frame);
}

void test_1(Grabber *grabber) {
    AVFrame *frame = av_frame_alloc();
    while(!grabber->stream_end()){
        grabber->return_frame(false, frame);
        printf("audio frame %lld\n", frame->channels);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    av_frame_free(&frame);
}

int main(){
    // FFmpeg
    char filepath[] = "/Users/wang/Desktop/Videos/test1.mp4";

    auto grabber = new Grabber(filepath);
    grabber->init();

    SDL_Window *screen = nullptr;
    init_sdl();

    std::thread t1(&Grabber::grab_frame, grabber);
//    std::thread t2(video_player, grabber, screen);
    std::thread t3(audio_main, grabber);
    video_player(grabber);
    t1.join();
//    t2.join();
    t3.join();
    grabber->free_all();
    return 0;
}

