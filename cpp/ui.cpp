#include "ui.h"

#include <string>
#include <fmt/core.h>

#define ESC "\x1B"
#define CSI "\x1B["

void ui_draw(bool paused, int elapsed, int duration, int width, int height)
{
    int d_hours = duration / 3600000;
    int d_minutes = (duration % 3600000) / 60000;
    int d_seconds = ((duration % 3600000) % 60000) / 1000;

    int c_hours = elapsed / 3600000;
    int c_minutes = (elapsed % 3600000) / 60000;
    int c_seconds = ((elapsed % 3600000) % 60000) / 1000;

    int time_width = d_hours == 0 ? 7 : 10;
    int linesize = width - (time_width * 2);

    if (width <= (time_width * 2) + 2)
        return;

    std::string elapsed_line = "";
    std::string remaining_line = "";

    double progress = static_cast<double>(elapsed) / static_cast<double>(duration);
    int progress_width = static_cast<int>(static_cast<double>(linesize) * progress);
    int remaining_width = linesize - progress_width;

    for (int i = 0; i < progress_width; i++)
        elapsed_line += "━";
    for (int i = 0; i < remaining_width; i++)
        remaining_line += "·";


    // 37

    fmt::print(CSI "0m" CSI "107m" CSI "40m" CSI "{};0H", height);

    if (d_hours == 0)
    {
        fmt::print("<{:02}:{:02}>{}{}<{:02}:{:02}>", c_minutes, c_seconds, elapsed_line, remaining_line, d_minutes, d_seconds);
    }
    else
    {
        //fmt::print("<{:02}:{:02}>{}\x1B[37m{}\x1B[107m<{:02}:{:02}>", c_minutes, c_seconds, elapsed_line, remaining_line, d_minutes, d_seconds);
    }
}