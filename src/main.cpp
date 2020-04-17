//
// Created by wang on 2020/4/1.
//

#include "Grabber.h"
#include "VideoProcessor.h"
#include "AudioProcessor.h"
#include <thread>

extern int init_sdl();
extern int video_player(VideoProcessor *pro, AudioProcessor *audio_p);
extern int audio_main(AudioProcessor *pro);
extern void playYuvFile(const string& inputPath);



int reader(Grabber *grabber, VideoProcessor *video_p, AudioProcessor *audio_p) {
   int video_idx = video_p->get_stream_idx();
   int audio_idx = audio_p->get_stream_idx();
   auto packet = (AVPacket*)av_malloc(sizeof(AVPacket));
   while(!grabber->stream_end()){
//       cout << "video:" << video_p->packet_que_size()
//       << "audio:" << audio_p->packet_que_size()
//       << endl;
       while(video_p->need_new_packet() || audio_p->need_new_packet()){
//           memset(packet, 0, sizeof(AVPacket));
           int ret = grabber->grab_packet(packet);
           if(ret < 0){
               cout << "grab packet fail." << endl;
               break;
           }
//           cout << "packet" << packet->stream_index << endl;
           if(packet->stream_index == video_idx){
               video_p->push_packet(packet);
           }else if(packet->stream_index == audio_idx){
               audio_p->push_packet(packet);
           }else{
//           cout << "wrong packet" << endl;
           }
//           av_packet_unref(packet);
       }
       std::this_thread::sleep_for(std::chrono::milliseconds(10));
   }

   video_p->set_closed();
   audio_p->set_closed();

   av_free(packet);
   return 0;
}



int main(){
    // FFmpeg
    char filepath[] = "/Users/wang/Desktop/Videos/test1.mp4";
//    playYuvFile(filepath);

    auto grabber = new Grabber(filepath);
    grabber->init();

    auto video_p = new VideoProcessor(grabber->get_ctx());
    video_p->init();

    auto audio_p = new AudioProcessor(grabber->get_ctx());
    audio_p->init();

    init_sdl();

    std::thread t1(reader, grabber, video_p, audio_p);
    std::thread t3(audio_main, audio_p);
    video_player(video_p, audio_p);
    t1.join();
    t3.join();
    cout << "finish" << endl;
    grabber->free_all();
    video_p->free_all();
    audio_p->free_all();
    return 0;
}

