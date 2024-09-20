#ifndef AUDIO_H
#define AUDIO_H

#define SDL_MAIN_HANDLED

#include <stdint.h>
#include <SDL2/SDL.h>
S

typedef struct audio_context
{
	SDL_AudioDeviceID sdl_device;
	SDL_AudioSpec specs;
	SwrContext* resampler;
	AVCodecContext* codec_ctx;
	AVFrame* resampled_frame;
	uint8_t* audio_buffer;
} audio_context;

audio_context* audio_init(AVCodecContext* codec_ctx);
void audio_playdata(audio_context* ctx, AVFrame* data);
void audio_wait(audio_context* ctx);
void audio_destroy(audio_context* ctx);

#endif //AUDIO_H