#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "renderer.h"
#include "audio.h"

void help()
{
    printf("Usage: cvp [options] <input>\n");
    printf("Plays a video file on the terminal.\n");
    printf("\n");
    printf("  -f, --full-color      Play the video file in RGB mode\n");
    printf("  -a, --audio           Play audio\n");
    printf("  -l, --libao           Uses libao instead of SDL for audio playback\n");
    printf("  -t, --multithreading  Uses multithreading to decode videos.\n");
    printf("                        Some codecs have trouble with this on, others\n");
    printf("                        only work with this on. Use if video is playing\n");
    printf("                        slowly.\n");
    printf("  -h, --help            Display this help and exit\n");
    printf("  -v, --version         Output version information and exit\n");
}

void version()
{
    printf("cvp 1.2\n");
}

// very rough argument handling function
void handle_args(int argc, char** argv, cvp_settings* settings)
{
    if (argc == 1)
    {
        printf("cvp: usage error: Input file required\n");
        exit(1);
    }
    else
    {
        for (int i = 1; i < argc; i++)
        {
            if (argv[i][0] == '-')
            {
                if (strlen(argv[i]) >= 2)
                {
                    if (argv[i][1] == '-')
                    {
                        if (strcmp(argv[i], "--full-color") == 0) settings->mode = RENDERER_FULL_COLOR;
                        else if (strcmp(argv[i], "--multithreading") == 0) settings->multithreading = 1;
                        else if (strcmp(argv[i], "--audio") == 0) settings->audio = 1;
                        else if (strcmp(argv[i], "--libao") == 0) settings->audio_driver = AUDIO_DRIVER_LIBAO;
                        else if (strcmp(argv[i], "--help") == 0)
                        {
                            help();
                            exit(0);
                        }
                        else if (strcmp(argv[i], "--version") == 0)
                        {
                            version();
                            exit(0);
                        }
                        else
                        {
                            printf("cvp: invalid option -- '%s'\n", argv[i]);
                            printf("Try 'cvp --help' for more information.\n");
                            exit(1);
                        }
                    }
                    else
                    {
                        for (size_t j = 1; j < strlen(argv[i]); j++)
                        {
                            if (argv[i][j] == 'f') settings->mode = RENDERER_FULL_COLOR;
                            else if (argv[i][j] == 't') settings->multithreading = 1;
                            else if (argv[i][j] == 'a') settings->audio = 1;
                            else if (argv[i][j] == 'l') settings->audio_driver = AUDIO_DRIVER_LIBAO;
                            else if (argv[i][j] == 'h' || argv[i][j] == '?')
                            {
                                help();
                                exit(0);
                            }
                            else if (argv[i][j] == 'v')
                            {
                                version();
                                exit(0);
                            }
                            else
                            {
                                printf("cvp: invalid option -- '%s'\n", argv[i]);
                                printf("Try 'cvp --help' for more information.\n");
                                exit(1);
                            }

                        }
                    }
                }
            }
            else
            {
                if (settings->input == NULL)
                {
                    settings->input = argv[i];
                }
                else
                {
                    printf("cvp: too many arguments given -- %s\n", argv[i]);
                    printf("Try 'cvp --help' for more information.\n");
                    exit(1);
                }
            }
        }

        if (settings->input == NULL)
        {
            printf("cvp: no input file given\n");
            exit(1);
        }
    }
}

void show_fps(int64_t fps)
{
    if (fps == 0)
        fps = 1;
    printf("\x1B]0;%" PRId64 " FPS\x1B\x5C", 1000 / fps );
}

#if defined(__unix__) || defined(__APPLE__)

#include <unistd.h>
#include <termios.h>
#include <sys/select.h>

int has_init = 0;
struct termios og_term;

void reset_term()
{
    tcsetattr(0, TCSANOW, &og_term);
}

int _kbhit(void)
{
    if (!has_init)
    {
        struct termios new_term;
        tcgetattr(0, &og_term);
        memcpy(&new_term, &og_term, sizeof(new_term));
        atexit(reset_term);
        cfmakeraw(&new_term);
        tcsetattr(0, TCSANOW, &new_term);
        has_init = 1;
    }

    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv) > 0;
}

int _getch(void)
{
    int r;
    unsigned char c;
    if ((r = read(0, &c, sizeof(c))) < 0)
        return r;
    else
        return c;
}

#endif