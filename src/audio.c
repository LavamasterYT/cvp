#include "audio.h"

#include <stdlib.h>
#include <SDL2/SDL.h>
#include <libswresample/swresample.h>
#include <libavcodec/avcodec.h>

int audio_init_sdl(audio_context* ctx, int channels)
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_AUDIO))
		return 1;

	SDL_AudioSpec want;
	SDL_zero(want);
	SDL_zero(ctx->specs);
	want.freq = 44100;
	want.channels = channels;
	want.format = AUDIO_S16SYS;
	ctx->sdl_device = SDL_OpenAudioDevice(NULL, 0, &want, &ctx->specs, 0);

	if (ctx->sdl_device < 1)
	{
		SDL_Quit();
		return 2;
	}

	SDL_PauseAudioDevice(ctx->sdl_device, 0);
	return 0;
}

audio_context* audio_init(AVCodecContext* codec_ctx)
{
	audio_context* ctx = (audio_context*)malloc(sizeof(audio_context));
	if (ctx == NULL)
		return NULL;
	
	ctx->codec_ctx = codec_ctx;
	ctx->audio_buffer = NULL;
	ctx->resampled_frame = av_frame_alloc();
	ctx->resampler = swr_alloc();
	
	int ret  = 0;
#if defined(_WIN32) || defined(__APPLE__)
	ret = swr_alloc_set_opts2(&ctx->resampler, &codec_ctx->ch_layout, AV_SAMPLE_FMT_S16, 44100, &codec_ctx->ch_layout, codec_ctx->sample_fmt, codec_ctx->sample_rate, 0, NULL);
#else
	ctx->resampler = swr_alloc_set_opts(ctx->resampler, codec_ctx->channel_layout, AV_SAMPLE_FMT_S16, 44100, codec_ctx->channel_layout, codec_ctx->sample_fmt, codec_ctx->sample_rate, 0, NULL);
	if (ctx->resampler == NULL)
		ret = -1;
	else
		ret = 0;
#endif

	if (ret < 0)
	{
		av_frame_free(&ctx->resampled_frame);
		free(ctx);
		return NULL;
	}

	swr_init(ctx->resampler);

	if (audio_init_sdl(ctx, codec_ctx->ch_layout.nb_channels))
	{
		av_frame_free(&ctx->resampled_frame);
		swr_free(&ctx->resampler);
		free(ctx);
		return NULL;
	}

	return ctx;
}

void audio_playdata(audio_context* ctx, AVFrame* data)
{
	if (ctx == NULL)
		return;

	int dst_samples = data->ch_layout.nb_channels * av_rescale_rnd(swr_get_delay(ctx->resampler, data->sample_rate) + data->nb_samples, 44100, data->sample_rate, AV_ROUND_UP);
	int ret = av_samples_alloc(&ctx->audio_buffer, NULL, 1, dst_samples, AV_SAMPLE_FMT_S16, 1);

	if (ret < 0)
	{
		av_frame_unref(data);
		av_frame_unref(ctx->resampled_frame);

		av_freep(&ctx->audio_buffer);
		return;
	}

	dst_samples = data->ch_layout.nb_channels * swr_convert(ctx->resampler, &ctx->audio_buffer, dst_samples, (const uint8_t**)data->data, data->nb_samples);
	ret = av_samples_fill_arrays(ctx->resampled_frame->data, ctx->resampled_frame->linesize, ctx->audio_buffer, 1, dst_samples, AV_SAMPLE_FMT_S16, 1);

	if (ret < 0)
	{
		av_frame_unref(data);
		av_frame_unref(ctx->resampled_frame);

		av_freep(&ctx->audio_buffer);
		return;
	}

	SDL_QueueAudio(ctx->sdl_device, ctx->resampled_frame->data[0], ctx->resampled_frame->linesize[0]);

	av_frame_unref(data);
	av_frame_unref(ctx->resampled_frame);

	av_freep(&ctx->audio_buffer);
}

void audio_wait(audio_context* ctx)
{
	if (ctx == NULL)
		return;

	uint32_t remaining = 0;

	while (1)
	{
		remaining = SDL_GetQueuedAudioSize(ctx->sdl_device);

		if (remaining < 5)
			return;

		SDL_Delay(100);
	}
}

void audio_destroy(audio_context* ctx)
{
	if (ctx == NULL)
		return;

	av_frame_free(&ctx->resampled_frame);
	swr_free(&ctx->resampler);

	SDL_CloseAudioDevice(ctx->sdl_device);
	SDL_Quit();

	free(ctx);
}