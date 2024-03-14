#ifndef UTILS_H
#define UTILS_H

typedef struct cvp_settings
{
    int mode; // Drawing mode
    int audio; // Play audio
    int multithreading; // Use multithreading
    char* input; // File input
} cvp_settings;

void handle_args(int argc, char** argv, cvp_settings* settings);
void show_fps(long fps);

#if defined(__unix__) || defined(__APPLE__)
int kbhit(void);
int _kbhit(void);
int getch(void);
int _getch(void);
#endif

#endif // UTILS_H