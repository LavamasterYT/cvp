#include "audio.h"
#include "colors.h"
#include "decoder.h"
#include "renderer.h"
#include "ui.h"

#include <fmt/core.h>
#include <ncurses.h>

#include <cstdint>
#include <string>
#include <vector>

struct v2 {
    int x;
    int y;
};

typedef struct v2 v2;

int main() {
    initscr();
    noecho();
    cbreak();

    std::string file = "/Users/josem/Movies/YouTube/chainsaw.mp4";

    renderer r;
    v2 pos;

    r.mode = RENDERER_MODE_PALETTE;
    r.set_dimensions();

    std::vector<colors_rgb> buffer(r.width * r.height);

    for (auto& i : buffer) {
        i.r = 0;
        i.g = 0;
        i.b = 0;
    }

    pos.x = 0;
    pos.y = 0;

    bool done = false;

    while (!done) {

        r.draw(buffer, 80, 25);

        buffer[pos.x + r.width * pos.y].r = 0;
        buffer[pos.x + r.width * pos.y].g = 0;
        buffer[pos.x + r.width * pos.y].b = 0;

        int key = getch();

        if (key == 'q') {
            done = true;
        }
        else if (key == 'w') {
            pos.y--;

            if (pos.y < 0) {
                pos.y = r.height - 1;
            }
        }
        else if (key == 's') {
            pos.y++;

            if (pos.y >= r.height) {
                pos.y = 0;
            }
        }
        else if (key == 'a') {
            pos.x--;

            if (pos.x < 0) {
                pos.x = r.width - 1;
            }
        }
        else if (key == 'd') {
            pos.x++;

            if (pos.x >= r.width) {
                pos.x = 0;
            }
        }

        buffer[pos.x + r.width * pos.y].r = 0;
        buffer[pos.x + r.width * pos.y].g = 255;
        buffer[pos.x + r.width * pos.y].b = 0;
    }

    endwin();

    return 0;
}