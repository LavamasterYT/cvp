#include "audio.h"

#include <cstdint>

#include <SDL2/SDL.h>

extern "C" {
    #include <libswresample/swresample.h>
    #include <libavcodec/avcodec.h>
}

Audio::Audio(AVCodecContext* ctx) {
    mCtx = ctx;
    mBuffer = nullptr;
    mSampledFrame = av_frame_alloc();
    mSampler = swr_alloc();
}

Audio::~Audio() {
    mBuffer = nullptr;
    mCtx = nullptr;
    if (mSampledFrame) av_frame_free(&mSampledFrame);
    if (mSampler) swr_free(&mSampler);

    if (mDev > 0) {
        SDL_CloseAudioDevice(mDev);
        SDL_Quit();
    }
}

bool Audio::init() {
    if (mCtx == nullptr)
        return false;

    swr_alloc_set_opts2(&mSampler, &mCtx->ch_layout, AV_SAMPLE_FMT_S16, 44100, &mCtx->ch_layout, mCtx->sample_fmt, mCtx->sample_rate, 0, NULL);
    swr_init(mSampler);

    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_AUDIO))
        return false;

    SDL_AudioSpec want;
    SDL_zero(want);
    want.freq = 44100;
    want.channels = mCtx->ch_layout.nb_channels;
    want.format = AUDIO_S16SYS;
    mDev = SDL_OpenAudioDevice(NULL, 0, &want, &mSpec, 0);
    SDL_PauseAudioDevice(mDev, 0);

    return mDev > 0;
}

void Audio::play(AVFrame* frame) {
    int dstSamples = frame->ch_layout.nb_channels * av_rescale_rnd(swr_get_delay(mSampler, frame->sample_rate) + frame->nb_samples, 44100, frame->sample_rate, AV_ROUND_UP);
    int ret = av_samples_alloc(&mBuffer, nullptr, 1, dstSamples, AV_SAMPLE_FMT_S16, 1);

    if (ret < 0) {
        av_frame_unref(frame);
        av_frame_unref(mSampledFrame);
        av_freep(&mBuffer);
        return;
    }

    dstSamples = frame->ch_layout.nb_channels * swr_convert(mSampler, &mBuffer, dstSamples, (const uint8_t**)frame->data, frame->nb_samples);
	ret = av_samples_fill_arrays(mSampledFrame->data, mSampledFrame->linesize, mBuffer, 1, dstSamples, AV_SAMPLE_FMT_S16, 1);
    
    if (ret < 0) {
        av_frame_unref(frame);
        av_frame_unref(mSampledFrame);
        av_freep(&mBuffer);
        return;
    }

    SDL_QueueAudio(mDev, mSampledFrame->data[0], mSampledFrame->linesize[0]);
    
    av_frame_unref(frame);
    av_frame_unref(mSampledFrame);
    av_freep(&mBuffer);
}

double Audio::get_queued_time() {
    int playedBytes = SDL_GetQueuedAudioSize(mDev);
    double playedTime = static_cast<double>(playedBytes) / (mSpec.freq * mSpec.channels * sizeof(int16_t));
    return playedTime;
}

void Audio::clear_queue() {
    SDL_ClearQueuedAudio(mDev);
}
