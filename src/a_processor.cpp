//
// Created by wang on 2020/4/16.
//
//
// Created by wang on 2020/4/8.
//
#include "AudioProcessor.h"
#define MAX_AUDIO_FRAME_SIZE 192000

AudioProcessor::AudioProcessor(AVFormatContext *format){
    this->avFormatCtx = format;
    out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE*2);
}

int AudioProcessor::init(){

    if(int ret = open_codec() < 0){
        printf("open codec fail.");
        return -1;
    }


    audio_info.out_nb_samples = codecCtx->frame_size;
    audio_info.out_channels = av_get_channel_layout_nb_channels(audio_info.out_channel_layout);
    int64_t in_channel_layout = av_get_default_channel_layout(codecCtx->channels);

    au_convert_ctx = swr_alloc();

    au_convert_ctx = swr_alloc_set_opts(au_convert_ctx,
                                        audio_info.out_channel_layout,
                                        audio_info.out_sample_fmt,
                                        audio_info.out_sample_rate,
                                        in_channel_layout,
                                        codecCtx->sample_fmt,
                                        codecCtx->sample_rate,0, NULL);

    swr_init(au_convert_ctx);

    stream_time_base = avFormatCtx->streams[stream_idx]->time_base;

    return 0;
}

int AudioProcessor::open_codec(){

    int stream_temp = -1;

    for(int i = 0; i < avFormatCtx->nb_streams; i++){
        if(avFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
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
    codecCtx = avcodec_alloc_context3(avCodec);
    avcodec_parameters_to_context(codecCtx, avCodeParameters);
    if(avcodec_open2(codecCtx, avCodec,NULL)<0){
        printf("Could not open codec.\n");
        return -1;
    }
    stream_idx = stream_temp;

    return 0;
}


void AudioProcessor::decode_audio_frame() {
    AVPacket *packet = av_packet_alloc();
    get_packet(packet);
    //更新时间戳
    if (packet->pts != AV_NOPTS_VALUE) {
//        cout << av_q2d(stream_time_base) << endl;
        set_pts(packet->pts * av_q2d(stream_time_base) * 1000 );
    }

    set_pts(clock + 1000 * packet->size/((double) 44100 * 2 * 2));

    int ret = -1;
    ret = avcodec_send_packet(codecCtx, packet);
    if (ret == 0) {
        // cout << "avcodec_send_packet success." << endl;
    } else if (ret == AVERROR(EAGAIN)) {
        // buff full, can not decode any more, nothing need to do.
        // keep the packet for next time decode.
        av_packet_unref(packet);
    } else if (ret == AVERROR_EOF) {
        // no new packets can be sent to it, it is safe.
        cout << "[WARN]  no new packets can be sent to it. index=" << packet->stream_index << endl;
    } else {
        string errorMsg = "+++++++++ ERROR avcodec_send_packet error audio: ";
        errorMsg += ret;
        cout << errorMsg << endl;
        throw std::runtime_error(errorMsg);
    }

    auto target_frame = av_frame_alloc();
    ret = avcodec_receive_frame(codecCtx, target_frame);

    if (ret == 0) {
        swr_convert(au_convert_ctx, &out_buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)target_frame->data , target_frame->nb_samples);
    }else if (ret == AVERROR_EOF) {
        cout << "+++++++++++++no more output frames. index="
             << packet->stream_index<< endl;
    } else if (ret == AVERROR(EAGAIN)) {
        // need more packet.
    } else {
        string errorMsg = "avcodec_receive_frame error: ";
        errorMsg += ret;
        cout << errorMsg << endl;
        throw std::runtime_error(errorMsg);
    }
    av_frame_free(&target_frame);
    av_packet_free(&packet);
}

void AudioProcessor::free_all() {

    swr_free(&au_convert_ctx);
    av_free(out_buffer);
    avcodec_free_context(&codecCtx);

    while(!packet_que.empty()){
        std::lock_guard<std::mutex> lock(packet_que_mutex);
        packet_que.pop_front();
    }
}

