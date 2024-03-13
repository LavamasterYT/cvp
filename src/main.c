#include <stdio.h>
#include <libavutil/time.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>

#include "renderer.h"
#include "decoder.h"

// signal stuff

int requested_int = 0;

void handle_interrupt(int sig)
{
    requested_int = 1;
}

int handle_args(int argc, char** argv, int* mode, char** input, int* multithreading)
{
    if (argc == 1)
    {
        printf("cvp: usage error: Input file required\n");
        return 1;
    }
    else
    {
        for (int i = 1; i < argc; i++)
        {
            if (argv[i][0] == '-')
            {
                if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--full-color") == 0)
                {
                    *mode = RENDERER_FULL_COLOR;
                }
                else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--multithreading") == 0)
                {
                    *multithreading = 1;
                }
                else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
                {
                    printf("Usage: cvp [options] <input>\n");
                    printf("Plays a video file on the terminal.\n");
                    printf("\n");
                    printf("  -f, --full-color      Play the video file in RGB mode\n");
                    printf("  -t, --multithreading  Uses multithreading to decode videos.\n");
                    printf("                        Some codecs have trouble with this on, others\n");
                    printf("                        only work with this on. Use if video is playing\n");
                    printf("                        slowly.\n");
                    printf("  -h, --help            Display this help and exit\n");
                    printf("  -v, --version         Output version information and exit\n");
                    exit(0);
                }
                else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0)
                {
                    printf("cvp 1.01\n");
                    exit(0);
                }
                else
                {
                    printf("cvp: invalid option '%s'\n", argv[i]);
                    printf("Try 'cvp --help' for more information.\n");
                    return 1;
                }
            }
            else
            {
                if (*input == NULL)
                {
                    *input = argv[i];
                }
                else
                {
                    printf("cvp: too many arguments given -- %s\n", argv[i]);
                    printf("Try 'cvp --help' for more information.\n");
                    return 1;
                }
            }
        }

        if (*input == NULL)
        {
            printf("cvp: no input file given\n");
            return 1;
        }
    }

    return 0;
}

int main(int argc, char** argv)
{
    int mode = RENDERER_PALETTE; // Drawing mode
    int multithreading = 0; // Use multithreading
    char* input = NULL; // File input

    if (handle_args(argc, argv, &mode, &input, &multithreading) != 0)
        return 1;

    signal(SIGINT, handle_interrupt);

    decoder_context* ctx = decoder_init();

    if (ctx == NULL)
    {
        printf("cvp: unable to intialize decoder!\n");
        return -1;
    }

    renderer_term_window* window = renderer_init(mode);

    printf("\x1B]0;%dx%d\x1B\x5C", window->width, window->height);

    if (decoder_open_input(ctx, input, window->width, window->height, multithreading) < 0)
    {
        renderer_destroy(window);
        printf("cvp: cannot access %s\n", input);
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

    int frame = 0;
    int frame_to_reach = 0;

    int64_t ms = 0;
    int64_t start =  av_gettime();

    while (decoder_read_frame(ctx, video_buffer) == 0 || !requested_int)
    {
        if (requested_int)
            break;

        frame++;
        if (frame < frame_to_reach)
            continue;

        renderer_draw(window, (renderer_rgb*)video_buffer);

        do
        {
            if (requested_int)
                break;

            ms = (av_gettime() - start) / 1000;
            frame_to_reach = (int)(floor(ms * ctx->fps / 1000.0)) + 1;
        } while (frame_to_reach == frame);
    }

    free((void*)video_buffer);
    decoder_ctx_destroy(ctx);
    renderer_destroy(window);

    return 0;
}