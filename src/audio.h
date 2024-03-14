#ifndef AUDIO_H
#define AUDIO_H

#include <mpv/client.h>

typedef struct audio_context
{
	mpv_handle* mpv;
	int paused;
} audio_context;

audio_context* audio_init(char* input);
void audio_playpause(audio_context* ctx);
void audio_destroy(audio_context* ctx);

#endif //AUDIO_H