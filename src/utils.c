#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "renderer.h"

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
                if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--full-color") == 0)
                {
                    settings->mode = RENDERER_FULL_COLOR;
                }
                else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--multithreading") == 0)
                {
                    settings->multithreading = 1;
                }
                else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--audio") == 0)
                {
                    settings->audio = 1;
                }
                else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
                {
                    printf("Usage: cvp [options] <input>\n");
                    printf("Plays a video file on the terminal.\n");
                    printf("\n");
                    printf("  -f, --full-color      Play the video file in RGB mode\n");
                    printf("  -a, --audio           Play audio\n");
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
                    printf("cvp 1.1\n");
                    exit(0);
                }
                else
                {
                    printf("cvp: invalid option '%s'\n", argv[i]);
                    printf("Try 'cvp --help' for more information.\n");
                    exit(1);
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
    printf("\x1B]0;%ld FPS\x1B\x5C", 1000 / fps );
}

#if defined(__unix__) || defined(__APPLE__)

#include <unistd.h>
#include <termios.h>
#include <sys/select.h>

int kbhit(void)
{
    // TODO: implement this for posix
    return 0;
}

int _kbhit(void)
{
    return kbhit();
}

int getch(void)
{
    // TODO: implement this for posix
    return 0;
}

int _getch(void)
{
    return getch();
}

#endif