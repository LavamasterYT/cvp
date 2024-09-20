#include "input.h"

#include <cstdlib>
#include <memory>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>

int has_init = 0;
struct termios og_term;

void reset_term()
{
    tcsetattr(0, TCSANOW, &og_term);
}

int key_pending()
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

int get_key()
{
    int r;
    unsigned char c;
    if ((r = read(0, &c, sizeof(c))) < 0)
        return r;
    else
        return c;
}
