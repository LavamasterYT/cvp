#ifndef AUDIO_H
#define AUDIO_H

#define SDL_MAIN_HANDLED

#include <stdint.h>
#include <SDL2/SDL.h>
#include <ao/ao.h>
#include <libswresample/swresample.h>
#include <libavcodec/avcodec.h>

#define AUDIO_DRIVER_SDL 0
#define AUDIO_DRIVER_LIBAO 1

typedef struct audio_context
{
	SDL_AudioDeviceID sdl_device;
	SDL_AudioSpec specs;
	ao_device* device;
	SwrContext* resampler;
	AVCodecContext* codec_ctx;
	AVFrame* resampled_frame;
	uint8_t* audio_buffer;
	int driver;

} audio_context;

audio_context* audio_init(AVCodecContext* codec_ctx, int audio_driver);
void audio_playdata(audio_context* ctx, AVFrame* data);
void audio_wait(audio_context* ctx);
void audio_destroy(audio_context* ctx);

#endif //AUDIO_H