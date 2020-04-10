//
// Created by wang on 2020/4/9.
//
//
// Created by wang on 2020/4/1.
//

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <Grabber.h>

#define SFM_REFRESH_EVENT (SDL_USEREVENT + 1)

#define SFM_BREAK_EVENT (SDL_USEREVENT + 2)

extern "C"{
#include "SDL2/SDL.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}

int thread_exit = 0;

int init_sdl() {
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf( "Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }

    return 0;
}

int sfp_refresh_thread(void *opaque){
    thread_exit = 0;

    while(!thread_exit){
        SDL_Event event;
        event.type = SFM_REFRESH_EVENT;
        SDL_PushEvent(&event);

        SDL_Delay(40);
    }
    thread_exit=0;
    SDL_Event event;
    event.type = SFM_BREAK_EVENT;
    SDL_PushEvent(&event);

    return 0;
}

int video_player(Grabber *grabber){

    AVCodecContext *videoCodecCtx = grabber->get_video_ctx();
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

    AVFrame *frameYUV = av_frame_alloc();

    auto *out_buffer=(unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P,  videoCodecCtx->width, videoCodecCtx->height, 1));
    av_image_fill_arrays(frameYUV->data, frameYUV->linesize, out_buffer,
                         AV_PIX_FMT_YUV420P, screen_w, screen_h,1);

    struct SwsContext *img_convert_ctx = sws_getContext(videoCodecCtx->width, videoCodecCtx->height, videoCodecCtx->pix_fmt,
                                                        videoCodecCtx->width, videoCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    SDL_Thread *video_tid = SDL_CreateThread(sfp_refresh_thread, NULL, NULL);

    SDL_Event event;

    AVFrame *frame = av_frame_alloc();

    for(;;){
        SDL_WaitEvent(&event);
        if(event.type == SFM_REFRESH_EVENT){
            if(!grabber->stream_end()){
                grabber->return_frame(true, frame);
                if(frame != nullptr){
                    printf("got frame, ready to draw, %lld\n", frame->pkt_dts);
                    sws_scale(img_convert_ctx, (const unsigned char* const*)frame->data, frame->linesize, 0, videoCodecCtx->height, frameYUV->data, frameYUV->linesize);
                    SDL_UpdateYUVTexture(sdlTexture, &sdlRect,
                                         frameYUV->data[0], frameYUV->linesize[0],
                                         frameYUV->data[1], frameYUV->linesize[1],
                                         frameYUV->data[2], frameYUV->linesize[2]);
                    SDL_RenderClear( sdlRenderer );
                    SDL_RenderCopy( sdlRenderer, sdlTexture, NULL, &sdlRect);
                    SDL_RenderPresent( sdlRenderer );
                }
            }else{
                SDL_Event event;
                event.type = SFM_BREAK_EVENT;
                SDL_PushEvent(&event);
            }
        }else if(event.type == SDL_QUIT){
            thread_exit = 1;
        }else if(event.type == SFM_BREAK_EVENT){
            break;
        }
    }


    sws_freeContext(img_convert_ctx);
    SDL_CloseAudio();
    SDL_Quit();
    av_frame_free(&frameYUV);
    return 0;
}



