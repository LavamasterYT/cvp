/*
This is the audio implementation for now until I can figure out how to use
SDL, libao, or OpenAL with the FFmpeg libraries to play raw audio packets.

This is a rough audio implementation that uses libmpv.
*/

#include "audio.h"

#include <stdlib.h>
#include <mpv/client.h>

audio_context* audio_init(char* input)
{
	audio_context* ctx = (audio_context*)malloc(sizeof(audio_context));
	if (ctx == NULL)
		return NULL;

	// init mpv
	ctx->mpv = mpv_create();

	if (ctx->mpv == NULL)
	{
		free(ctx);
		return NULL;
	}

	// no video, auto audio device
	ctx->paused = 0;
	mpv_set_option_string(ctx->mpv, "video", "no");
	mpv_set_option_string(ctx->mpv, "audio-deivce", "auto");

	if (mpv_initialize(ctx->mpv) != 0)
	{
		mpv_terminate_destroy(ctx->mpv);
		ctx->mpv = NULL;
		free(ctx);
		return NULL;
	}

	// load file and wait until fully loaded (mainly for network videos)
	mpv_command(ctx->mpv, (const char* []) { "loadfile", input, NULL });

	while (1) {
		mpv_event* event = mpv_wait_event(ctx->mpv, -1);
		if (event->event_id == MPV_EVENT_FILE_LOADED) {
			break;
		}
	}

	// pause audio
	audio_playpause(ctx);

	return ctx;
}

void audio_playpause(audio_context* ctx)
{
	if (ctx == NULL)
		return;

	mpv_command_string(ctx->mpv, "cycle pause");
	if (ctx->paused)
		ctx->paused = 0;
	else
		ctx->paused = 1;
}

void audio_destroy(audio_context* ctx)
{
	mpv_destroy(ctx->mpv);
	free(ctx);
}