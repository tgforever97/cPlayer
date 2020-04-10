//
// Created by wang on 2020/4/9.
//

#include <Grabber.h>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include "SDL2/SDL.h"
}


#define MAX_AUDIO_FRAME_SIZE 192000

//Buffer:
//|-----------|-------------|
//chunk-------pos---len-----|
static  Uint8  *audio_chunk;
static  Uint32  audio_len;
static  Uint8  *audio_pos;

void  fill_audio(void *udata, Uint8 *stream, int len){
    SDL_memset(stream, 0, len);
    if(audio_len == 0)		/*  Only  play  if  we  have  data  left  */
        return;
    len = len>audio_len?audio_len:len;	/*  Mix  as  much  data  as  possible  */

    SDL_MixAudio(stream,audio_pos,len,SDL_MIX_MAXVOLUME);
    audio_pos += len;
    audio_len -= len;
}

int init_audio(Grabber *grabber, SwrContext **au_convert_ctx){

    AVCodecContext *audioCodecCtx = grabber->get_audio_ctx();

    uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
    //nb_samples: AAC-1024 MP3-1152
    int out_nb_samples = audioCodecCtx->frame_size;
    AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    int out_sample_rate = 44100;
    int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);

//    int out_buffer_size = av_samples_get_buffer_size(NULL,out_channels ,out_nb_samples,out_sample_fmt, 1);

    SDL_AudioSpec wanted_spec;
    wanted_spec.freq = out_sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = out_channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = out_nb_samples;
    wanted_spec.callback = fill_audio;
    wanted_spec.userdata = audioCodecCtx;

    if (SDL_OpenAudio(&wanted_spec, NULL)<0){
        printf("can't open audio.\n");
        return -1;
    }

    int64_t in_channel_layout = av_get_default_channel_layout(audioCodecCtx->channels);

    *au_convert_ctx = swr_alloc();

    *au_convert_ctx = swr_alloc_set_opts(*au_convert_ctx,out_channel_layout, out_sample_fmt, out_sample_rate,
                                        in_channel_layout,audioCodecCtx->sample_fmt, audioCodecCtx->sample_rate,0, NULL);
    swr_init(*au_convert_ctx);

    int out_buffer_size = av_samples_get_buffer_size(NULL,out_channels ,out_nb_samples,out_sample_fmt, 1);

    return out_buffer_size;
}

void audio_palyer(SwrContext *au_convert_ctx, AVFrame *frame, int out_buffer_size) {


    auto out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE*2);

    swr_convert(au_convert_ctx,&out_buffer, MAX_AUDIO_FRAME_SIZE,(const uint8_t **)frame->data , frame->nb_samples);

    while(audio_len > 0)
        SDL_Delay(1);

    audio_chunk = (Uint8 *) out_buffer;
    audio_len = out_buffer_size;
    audio_pos = audio_chunk;
    av_free(out_buffer);
}

void audio_close(SwrContext *au_convert_ctx){
    swr_free(&au_convert_ctx);
}

int audio_main(Grabber *grabber){
    SwrContext *au_convert_ctx = nullptr;
    AVFrame *frame = av_frame_alloc();
    auto out_size = init_audio(grabber, &au_convert_ctx);
    SDL_PauseAudio(0);
    while(!grabber->stream_end()){
        grabber->return_frame(false, frame);
        if(frame != nullptr){
            printf("got frame, ready to play %lld\n", frame->pkt_dts);
            audio_palyer(au_convert_ctx, frame, out_size);
        }
    }
    audio_close(au_convert_ctx);
    return 0;
}
