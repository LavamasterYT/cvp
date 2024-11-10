#pragma once

#include <cstdint>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

extern "C" {
    #include <libswresample/swresample.h>
    #include <libavcodec/avcodec.h>
}

class Audio {

private:
    SDL_AudioDeviceID mDev;
    SDL_AudioSpec mSpec;
    AVCodecContext* mCtx;
    AVFrame* mSampledFrame;
    SwrContext* mSampler;
    uint8_t* mBuffer;

public:
    Audio(AVCodecContext* ctx);
    ~Audio();

    bool init();
    void play(AVFrame* frame);
    double get_clock();
};
