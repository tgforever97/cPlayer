//
// Created by wang on 2020/4/9.
//

#include <AudioProcessor.h>

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

long long current_time_millis() {
    using namespace std::chrono;
    auto time_now = system_clock::now();
    auto duration_in_ms = duration_cast<milliseconds>(time_now.time_since_epoch());
    return duration_in_ms.count();
}

void  fill_audio(void *udata, Uint8 *stream, int len){
    SDL_memset(stream, 0, len);
    if(audio_len == 0)		/*  Only  play  if  we  have  data  left  */
        return;
    len = len>audio_len?audio_len:len;	/*  Mix  as  much  data  as  possible  */

    SDL_MixAudio(stream,audio_pos,len,SDL_MIX_MAXVOLUME);
    audio_pos += len;
    audio_len -= len;
}

int init_audio(AudioProcessor *pro){

    AVCodecContext *audioCodecCtx = pro->get_codec_ctx();
    auto audio_info = pro->audio_info;

    SDL_AudioSpec wanted_spec;
    wanted_spec.freq = audio_info.out_sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = audio_info.out_channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = audio_info.out_nb_samples;
    wanted_spec.callback = fill_audio;
    wanted_spec.userdata = audioCodecCtx;

    if (SDL_OpenAudio(&wanted_spec, NULL)<0){
        printf("can't open audio.\n");
        return -1;
    }

    int out_buffer_size = av_samples_get_buffer_size(NULL, audio_info.out_channels,
                                                     audio_info.out_nb_samples,
                                                     audio_info.out_sample_fmt, 1);

    return out_buffer_size;
}

void audio_player(uint8_t *out_buffer, int out_buffer_size) {

    while(audio_len > 0)
        SDL_Delay(1);

    audio_chunk = out_buffer;
    audio_len = out_buffer_size;
    audio_pos = audio_chunk;
}

int audio_main(AudioProcessor *pro){
//    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    auto out_size = init_audio(pro);
    if(out_size < 0){
        cout << "init audio error" << out_size
        << endl;
        return -1;
    }
    SDL_PauseAudio(0);
    while(!pro->get_closed()){
        pro->decode_audio_frame();
//        pro->update_pts(audio_len);
//        printf("got frame, ready to play\n");
        uint8_t *buffer = pro->out_buffer;
        if(buffer != nullptr){
            audio_player(buffer, out_size);
        }
    }
    cout << "stop play audio" << endl;
    return 0;
}


