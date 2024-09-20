#include "audio.h"

extern "C" {
    #include <SDL2/SDL.h>
    #include <libswresample/swresample.h>
    #include <libavcodec/avcodec.h>
}

audio::audio(AVCodecContext* audio_ctx)
{
    ctx = audio_ctx;
    buffer = nullptr;
    sampled_frame = av_frame_alloc();
    smplr = swr_alloc();
    
	swr_alloc_set_opts2(&smplr, &ctx->ch_layout, AV_SAMPLE_FMT_S16, 44100, &ctx->ch_layout, ctx->sample_fmt, ctx->sample_rate, 0, NULL);
    swr_init(smplr);

    if (!init_sdl())
    {
        ctx = nullptr;
        if (sampled_frame) av_frame_free(&sampled_frame);
        if (smplr) swr_free(&smplr);
    }
}

audio::~audio()
{
    buffer = nullptr;
    ctx = nullptr;
    if (sampled_frame) av_frame_free(&sampled_frame);
    if (smplr) swr_free(&smplr);

    if (dev > 0)
    {
        SDL_CloseAudioDevice(dev);
        SDL_Quit();
    }
}

bool audio::init_sdl()
{
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_AUDIO))
        return false;
    
    SDL_AudioSpec want;
	SDL_zero(want);
	SDL_zero(spec);
	want.freq = 44100;
	want.channels = ctx->ch_layout.nb_channels;
	want.format = AUDIO_S16SYS;
	dev = SDL_OpenAudioDevice(NULL, 0, &want, &spec, 0);

	if (dev < 1)
	{
		SDL_Quit();
		return false;
	}

	SDL_PauseAudioDevice(dev, 0);

    return true;
}

void audio::wait()
{
    unsigned int remaining = 0;

	while (1)
	{
		remaining = SDL_GetQueuedAudioSize(dev);

		if (remaining < 5)
			return;

		SDL_Delay(100);
	}
}

void audio::play(AVFrame* frame)
{
    int dst_samples = frame->ch_layout.nb_channels * av_rescale_rnd(swr_get_delay(smplr, frame->sample_rate) + frame->nb_samples, 44100, frame->sample_rate, AV_ROUND_UP);
	int ret = av_samples_alloc(&buffer, NULL, 1, dst_samples, AV_SAMPLE_FMT_S16, 1);

	if (ret < 0)
	{
		av_frame_unref(frame);
		av_frame_unref(sampled_frame);

		av_freep(&buffer);
		return;
	}

	dst_samples = frame->ch_layout.nb_channels * swr_convert(smplr, &buffer, dst_samples, (const uint8_t**)frame->data, frame->nb_samples);
	ret = av_samples_fill_arrays(sampled_frame->data, sampled_frame->linesize, buffer, 1, dst_samples, AV_SAMPLE_FMT_S16, 1);

	if (ret < 0)
	{
		av_frame_unref(frame);
		av_frame_unref(sampled_frame);

		av_freep(&buffer);
		return;
	}

	SDL_QueueAudio(dev, sampled_frame->data[0], sampled_frame->linesize[0]);

	av_frame_unref(frame);
	av_frame_unref(sampled_frame);

	av_freep(&buffer);
}