#pragma once

#include <cstdint>

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL2/SDL.h>

extern "C" {
    #include <libswresample/swresample.h>
    #include <libavcodec/avcodec.h>
}

/**
 * @brief A simple audio player.
 * 
 */
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

    /**
     * @brief Initializes 
     * 
     * @return true if the device successfully initalizes.
     * @return false on error.
     */
    bool init();

    /**
     * @brief Plays audio from the AVFrame
     * 
     * Converts and samples the audio given from AVFrame to the one matching the
     * audio device, and adds it to the audio queue using SDL_QueueAudio.
     * 
     * @param frame The frame to read audio from.
     */
    void play(AVFrame* frame);
    
    /**
     * @brief Gets the amount of audio that is queued in the SDL audio queue
     * in seconds, ie. there could be 0.9 seconds worth of audio in the queue
     * 
     * @return double 
     */
    double get_queued_time();

    /**
     * @brief Clears the audio queue.
     * 
     */
    void clear_queue();
};
