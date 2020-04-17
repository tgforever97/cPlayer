//
// Created by wang on 2020/4/9.
//
//
// Created by wang on 2020/4/1.
//

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <VideoProcessor.h>
#include <AudioProcessor.h>
#include <fstream>
#include <time.h>

#define SFM_REFRESH_EVENT (SDL_USEREVENT + 1)

#define SFM_BREAK_EVENT (SDL_USEREVENT + 2)

extern "C"{
#include "SDL2/SDL.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}

int thread_exit = 0;

int time_interval = 0;

long long currentTimeMillis() {
    using namespace std::chrono;
    auto time_now = system_clock::now();
    auto duration_in_ms = duration_cast<milliseconds>(time_now.time_since_epoch());
    return duration_in_ms.count();
}

int init_sdl() {
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf( "Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }

    return 0;
}

void push_break_event(){
    SDL_Event event;
    event.type = SFM_BREAK_EVENT;
    SDL_PushEvent(&event);
}

void push_refresh_event(){
    SDL_Event event;
    event.type = SFM_REFRESH_EVENT;
    SDL_PushEvent(&event);
}

int sfp_refresh_thread(void* opaque){
    while(!thread_exit){
        push_refresh_event();
        auto temp_interval = time_interval > 20 ? time_interval : 20;
        temp_interval = temp_interval < 60 ? temp_interval : 60;
//        cout << "picRefresher timeInterval[" << temp_interval << "]" << endl;
        SDL_Delay(temp_interval);
//        std::this_thread::sleep_for(std::chrono::milliseconds(time_interval));
    }
    thread_exit=0;
    push_break_event();
    return 0;
}

int video_player(VideoProcessor *pro, AudioProcessor *audio_p){

    AVCodecContext *videoCodecCtx = pro->get_codec_ctx();
    int screen_w = videoCodecCtx->width;
    int screen_h = videoCodecCtx->height;

    SDL_Window *screen = SDL_CreateWindow("Wang's Player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              screen_w, screen_h,SDL_WINDOW_SHOWN);

    if(!screen) {
        printf("SDL: could not create window - exiting:%s\n",SDL_GetError());
        return -1;
    }

    SDL_Renderer *sdlRenderer = SDL_CreateRenderer(screen, -1, 0);

    SDL_Texture *sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, screen_w, screen_h);

    SDL_Rect sdlRect;
    sdlRect.x = 0;
    sdlRect.y = 0;
    sdlRect.w = screen_w;
    sdlRect.h = screen_h;

    time_interval = 1000/pro->get_frame_rate();

    SDL_Thread *video_tid = SDL_CreateThread(sfp_refresh_thread, "video_refresh", NULL);

    SDL_Event event;

//    AVFrame *frameYUV = av_frame_alloc();

    for(;;){
        SDL_WaitEvent(&event);
        if(event.type == SFM_REFRESH_EVENT){
            if(pro->get_closed()){
                push_break_event();
            }
            cout << "refresh .." << endl;
            pro->decode_video_frame();
            auto diff = pro->get_pts() - audio_p->get_pts();
            if(diff > 30){
                cout << "video faster diff is " << diff << endl;
                time_interval = time_interval + 20;
            }else if(diff < 0 && abs(diff) > 30){
                time_interval = 1000/pro->get_frame_rate();
                cout << "video slow diff is" << diff << endl;
                push_refresh_event();
            }
            AVFrame *frame = pro->frame;
            if(frame->width != 0){
                SDL_UpdateYUVTexture(sdlTexture, &sdlRect,
                                     frame->data[0], frame->linesize[0],
                                     frame->data[1], frame->linesize[1],
                                     frame->data[2], frame->linesize[2]);
                SDL_RenderClear( sdlRenderer );
                SDL_RenderCopy( sdlRenderer, sdlTexture, NULL, &sdlRect);
                SDL_RenderPresent( sdlRenderer );
            }
        }else if(event.type == SDL_QUIT){
            thread_exit = 1;
        }else if(event.type == SFM_BREAK_EVENT){
            break;
        }else{
            continue;
        }
    }
    cout << "stop video" << endl;
    SDL_CloseAudio();
    SDL_Quit();
    return 0;
}




