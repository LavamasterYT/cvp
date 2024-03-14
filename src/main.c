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
    settings.multithreading = 0;
    settings.mode = RENDERER_PALETTE;
    handle_args(argc, argv, &settings);

    // stop playing if we receive interrupt
    signal(SIGINT, handle_interrupt);

    decoder_context* ctx = decoder_init();

    if (ctx == NULL)
    {
        printf("cvp: unable to intialize decoder!\n");
        return -1;
    }

    renderer_term_window* window = renderer_init(settings.mode);
    window->height -= 2; // make space for ui and "fix" a bug in the renderer

    if (decoder_open_input(ctx, settings.input, window->width, window->height, settings.multithreading) < 0)
    {
        renderer_destroy(window);
        printf("cvp: cannot access %s\n", settings.input);
        decoder_ctx_destroy(ctx);
        return -1;
    }

    decoder_rgb* video_buffer = decoder_alloc_rgb(ctx);
    if (video_buffer == NULL)
    {
        renderer_destroy(window);
        printf("cvp: unable to allocate memory\n");
        decoder_ctx_destroy(ctx);
        return -1;
    }

    audio_context* audio_ctx = NULL;
    if (settings.audio)
        audio_ctx = audio_init(settings.input);

    ui_context* ui_ctx = ui_init(ctx->duration, ctx->width, ctx->height + 1);

    int frame = 0;
    int frame_to_reach = 0;

    int64_t ms = 0; // elapsed ms since start of video
    int64_t start = 0; // time when video started
    int64_t fps_timer = 0; // time when frame started

    if (audio_ctx != NULL)
        audio_playpause(audio_ctx);

    start = av_gettime();

    while (decoder_read_frame(ctx, video_buffer) == 0)
    {
        if (requested_int)
            break;

        frame++;
        if (frame < frame_to_reach) // skip frames if necessary
            continue;

        renderer_draw(window, (renderer_rgb*)video_buffer);

        if (ui_ctx != NULL)
            ui_draw(ui_ctx, ms);

        show_fps((av_gettime() - fps_timer) / 1000);
        fps_timer = av_gettime(); // reset frame timer

        do
        {
            if (requested_int)
                break;

            if (_kbhit()) // check for keyboard input
            {
                int key = getch();

                switch (key)
                {
                case ' ': // space
                    // pause audio
                    if (audio_ctx != NULL)
                        audio_playpause(audio_ctx);

                    // time when pause period started
                    int64_t p_start = av_gettime();
                    while (1) // wait for interrupt or when space is pressed
                    {
                        if (requested_int)
                            break;

                        if (_kbhit())
                        {
                            if (_getch() == ' ')
                            {
                                break;
                            }
                        }
                        av_usleep(10000);
                    }

                    // add pause time to start time to offset
                    start += av_gettime() - p_start;

                    // play audio again
                    if (audio_ctx != NULL)
                        audio_playpause(audio_ctx);
                    break;
                }
            }

            ms = (av_gettime() - start) / 1000; // calculate elapsed ms
            frame_to_reach = (int)(floor(ms * ctx->fps / 1000.0)) + 1; // calculate any frames needed to skip
        } while (frame_to_reach <= frame);
    }

    // free resources
    if (audio_ctx != NULL)
        audio_destroy(audio_ctx);

    ui_destroy(ui_ctx);
    free((void*)video_buffer);
    decoder_ctx_destroy(ctx);
    renderer_destroy(window);

    return 0;
}