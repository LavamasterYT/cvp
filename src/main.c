#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <libavutil/time.h>

#ifdef _WIN32
#include <conio.h>
#endif

#include "renderer.h"
#include "audio.h"
#include "decoder.h"
#include "ui.h"
#include "utils.h"

int requested_int = 0;

void handle_interrupt(int sig)
{
    (void)sig;
    requested_int = 1;
}

int main(int argc, char** argv)
{
    // init settings and handle cmd line arguments
    cvp_settings settings;
    settings.audio = 0;
    settings.input = NULL;
    settings.mode = RENDERER_ASCII;
    handle_args(argc, argv, &settings);

    // stop playing if we receive interrupt
    signal(SIGINT, handle_interrupt);

    decoder_context* ctx = decoder_init();

    if (ctx == NULL)
    {
        printf("cvp: unable to intialize decoder!\n");
        return -1;
    }

    if (settings.mode == RENDERER_ASCII)
    {
        ctx->grayscale = 1;
    }

    renderer_term_window* window = renderer_init(settings.mode);
    window->height -= 2; // make space for ui and "fix" a bug in the renderer

    if (decoder_open_input(ctx, settings.input, window->width, window->height) < 0)
    {
        renderer_destroy(window);
        printf("cvp: cannot access %s\n", settings.input);
        decoder_ctx_destroy(ctx);
        return -1;
    }

    decoder_rgb* video_buffer = decoder_alloc_rgb(ctx);
    decoder_rgb* last_frame = decoder_alloc_rgb(ctx);

    if (video_buffer == NULL || last_frame == NULL)
    {
        renderer_destroy(window);
        printf("cvp: unable to allocate memory\n");
        decoder_ctx_destroy(ctx);
        return -1;
    }

    audio_context* audio_ctx = NULL;
    if (settings.audio)
        audio_ctx = audio_init(ctx->audio_ctx);

    ui_context* ui_ctx = ui_init(ctx->duration, window->width, window->height + 1);

    int frame = 0;
    int frame_to_reach = 0;

    int64_t new_ms = 0;
    int64_t ms = 0; // elapsed ms since start of video
    int64_t start = 0; // time when video started
    int64_t fps_timer = 0; // time when frame started

    int frame_index = 0;

    start = av_gettime();

    while (1)
    {
        if (requested_int)
            break;

        frame_index = decoder_read_frame(ctx);

        if (frame_index == ctx->audio_index)
        {
            audio_playdata(audio_ctx, ctx->frame);
            continue;
        }
        else if (frame_index < 0)
            break;

        frame++;
        if (frame < frame_to_reach) // skip frames if necessary
        {
            decoder_discard_frame(ctx);
            continue;
        }

        decoder_decode_video(ctx, video_buffer);
        renderer_draw(window, (renderer_rgb*)video_buffer, (renderer_rgb*)last_frame, ctx->width, ctx->height);

        if (ui_ctx != NULL)
            ui_draw(ui_ctx, ms);

        if (settings.audio)
        {
            int64_t audio_pts = (44100 - SDL_GetQueuedAudioSize(audio_ctx->sdl_device) / 
                                (2 * ctx->audio_ctx->ch_layout.nb_channels)) * 1000 / 44100; 
            int64_t video_pts = ms;

            if (audio_pts > video_pts + 100) {
                SDL_ClearQueuedAudio(audio_ctx->sdl_device);
            }
        }

        show_fps((av_gettime() - fps_timer) / 1000, ctx->width, ctx->height);
        fps_timer = av_gettime(); // reset frame timer

        do
        {
            if (requested_int)
                break;

            if (_kbhit()) // check for keyboard input
            {
                int key = _getch();

                switch (key)
                {
                case ' ': // space
                    ;   
                    if (settings.audio)
                        SDL_ClearQueuedAudio(audio_ctx->sdl_device);
                    
                    // time when pause period started
                    int64_t p_start = av_gettime();
                    while (1) // wait for interrupt or when space is pressed
                    {
                        if (requested_int)
                            break;

                        if (_kbhit())
                        {
                            key = _getch();
                            if (key == ' ')
                            {
                                break;
                            }
                            else if (key == 'q')
                            {
                                requested_int = 1;
                                break;
                            }
                        }
                        av_usleep(10000);
                    }

                    // add pause time to start time to offset
                    start += av_gettime() - p_start;
                    break;
                case 'a':;
                    new_ms = ms - 5000;
                    if (new_ms < 0)
                        new_ms = 0;
                    frame = decoder_seek(ctx, new_ms);
                    start += (ms - new_ms) * 1000;
                    break;
                case 'd':;
                    new_ms = ms + 5000;
                    frame = decoder_seek(ctx, new_ms);
                    start -= (new_ms - ms) * 1000;
                    break;
                case 'q':
                    requested_int = 1;
                    break;
                }
            }
            
            ms = (av_gettime() - start) / 1000; // calculate elapsed ms
            frame_to_reach = (int)(floor(ms * ctx->fps / 1000.0)) + 1; // calculate any frames needed to skip
        } while (frame_to_reach == frame);
    }

    audio_wait(audio_ctx);
    audio_destroy(audio_ctx);
    ui_destroy(ui_ctx);
    free((void*)video_buffer);
    decoder_ctx_destroy(ctx);
    renderer_destroy(window);

    return 0;
}