//
// Created by wang on 2020/4/1.
//

#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "SDL2/SDL.h"
}

#define SFM_REFRESH_EVENT (SDL_USEREVENT + 1)

#define SFM_BREAK_EVENT (SDL_USEREVENT + 2)

int thread_exit = 0;
int thread_pause = 0;

int sfp_refresh_thread(void *opaque){
    thread_exit = 0;
    thread_pause = 0;

    while(!thread_exit){
        if(!thread_pause){
            SDL_Event event;
            event.type = SFM_REFRESH_EVENT;
            SDL_PushEvent(&event);
        }
        SDL_Delay(30);
    }
    thread_exit=0;
    thread_pause=0;

    SDL_Event event;
    event.type = SFM_BREAK_EVENT;
    SDL_PushEvent(&event);

    return 0;
}

AVCodecContext *open_codec(int *stream_idx, AVFormatContext *avFormatContext, enum AVMediaType type){


}

int main(){
    // FFmpeg
    char filepath[] = "/Users/wang/Desktop/Videos/test1.mp4";

    avformat_network_init();
    AVFormatContext	*pFormatCtx = avformat_alloc_context();

    if(avformat_open_input(&pFormatCtx,filepath,NULL,NULL) != 0){
        printf("Couldn't open input stream.\n");
        return -1;
    }
    if(avformat_find_stream_info(pFormatCtx,NULL) < 0){
        printf("Couldn't find stream information.\n");
        return -1;
    }
    int video_index = -1;
    int audio_index = -1;
    for(int i = 0; i < pFormatCtx->nb_streams; i++){
        if(pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_index = i;
        }
        else if(pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            audio_index = i;
        }
        if(video_index != -1 && audio_index != -1)
            break;
    }

    if(video_index == -1){
        printf("Didn't find a video stream.\n");
        return -1;
    }
    AVCodecParameters *avCodeParameters = pFormatCtx->streams[video_index]->codecpar;
    AVCodec	*avCodec = avcodec_find_decoder(avCodeParameters->codec_id);
    if(avCodec == NULL){
        printf("Codec not found.\n");
        return -1;
    }
    AVCodecContext* avCodecContext = avcodec_alloc_context3(avCodec);
    avcodec_parameters_to_context(avCodecContext, avCodeParameters);
    if(avcodec_open2(avCodecContext, avCodec,NULL)<0){
        printf("Could not open codec.\n");
        return -1;
    }

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf( "Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }
    //SDL 2.0 Support for multiple windows
    int screen_w = avCodecContext->width;
    int screen_h = avCodecContext->height;
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

    AVFrame *frame = av_frame_alloc();
    AVFrame *frameYUV = av_frame_alloc();

    auto *out_buffer=(unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P,  avCodecContext->width, avCodecContext->height, 1));
    av_image_fill_arrays(frameYUV->data, frameYUV->linesize, out_buffer,
                         AV_PIX_FMT_YUV420P, screen_w, screen_h,1);

    struct SwsContext *img_convert_ctx = sws_getContext(avCodecContext->width, avCodecContext->height, avCodecContext->pix_fmt,
                                                        avCodecContext->width, avCodecContext->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    AVPacket *avPacket = av_packet_alloc();

    SDL_Thread *video_tid = SDL_CreateThread(sfp_refresh_thread, NULL, NULL);

    SDL_Event event;

    for(;;){
        SDL_WaitEvent(&event);
        if(event.type == SFM_REFRESH_EVENT){
            while(av_read_frame(pFormatCtx, avPacket) >= 0){
                if(avPacket->stream_index == video_index){
                    //Decode
                    int got_picture = 0;
                    int ret = avcodec_decode_video2(avCodecContext, frame, &got_picture, avPacket);
                    if(ret < 0){
                        printf("Decode Error.\n");
                        return -1;
                    }
                    printf("frame_count=%d\n", avCodecContext->frame_number);
                    if(got_picture){
                        sws_scale(img_convert_ctx, (const unsigned char* const*)frame->data, frame->linesize, 0, avCodecContext->height, frameYUV->data, frameYUV->linesize);
//                SDL_UpdateTexture( sdlTexture, NULL, frameYUV->data[0], frameYUV->linesize[0] );
                        SDL_UpdateYUVTexture(sdlTexture, &sdlRect,
                                             frameYUV->data[0], frameYUV->linesize[0],
                                             frameYUV->data[1], frameYUV->linesize[1],
                                             frameYUV->data[2], frameYUV->linesize[2]);
                        SDL_RenderClear( sdlRenderer );
                        SDL_RenderCopy( sdlRenderer, sdlTexture, NULL, &sdlRect);
                        SDL_RenderPresent( sdlRenderer );
                    }
                }
            }
        }else if(event.type == SDL_KEYDOWN){
            if(event.key.keysym.sym == SDLK_SPACE){
                thread_pause  = !thread_pause;
            }
        }else if(event.type == SDL_QUIT){
            thread_exit = 1;
        }else if(event.type == SFM_BREAK_EVENT){
            break;
        }
    }


    sws_freeContext(img_convert_ctx);
    SDL_Quit();

    avformat_close_input(&pFormatCtx);
    avcodec_free_context(&avCodecContext);
    av_frame_free(&frame);
    av_frame_free(&frameYUV);
    av_packet_free(&avPacket);

    return 0;
}

