#pragma once

#define SDL_MAIN_HANDLED

extern "C" {
    #include <SDL2/SDL.h>
    #include <libswresample/swresample.h>
    #include <libavcodec/avcodec.h>
}

class audio {
public:
    audio(AVCodecContext* ctx);
    ~audio();

    void play(AVFrame* frame);
    void wait();

private:
    SDL_AudioDeviceID dev;
    SDL_AudioSpec spec;
    AVCodecContext* ctx;
    AVFrame* sampled_frame;
    SwrContext* smplr;
    uint8_t* buffer;

    bool init_sdl();
};