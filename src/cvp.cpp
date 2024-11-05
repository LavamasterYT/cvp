#include "colors.h"
#include "console.h"

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
    fmt::println("\x1B[0;0H" "testing");
    fmt::print("\x1B[" "{};0H@", 18);
    fmt::print("\x1B[" "{};0H#", 19);
    fmt::print("\x1B[" "{};0H.", 20);
    fmt::println("\x1B[0;0H" "testing");

    getchar();


    std::string file = "/Users/josem/Movies/YouTube/chainsaw.mp4";

    Console r;
    v2 pos;

    r.set_mode(Console::ColorMode::MODE_ASCII);
    r.initialize();

    std::vector<colors::rgb> buffer(r.width() * r.height());

    for (auto& i : buffer) {
        i.r = 50;
        i.g = 50;
        i.b = 50;
    }

    pos.x = 0;
    pos.y = 0;

    bool done = false;

    while (!done) {

        r.draw(buffer);

        buffer[pos.x + r.width() * pos.y].r = 0;
        buffer[pos.x + r.width() * pos.y].g = 0;
        buffer[pos.x + r.width() * pos.y].b = 0;

        int key = r.handle_keypress();

        if (key == 'q') {
            done = true;
        }
        else if (key == 'w') {
            pos.y--;

            if (pos.y < 0) {
                pos.y = r.height() - 1;
            }
        }
        else if (key == 's') {
            pos.y++;

            if (pos.y >= r.height()) {
                pos.y = 0;
            }
        }
        else if (key == 'a') {
            pos.x--;

            if (pos.x < 0) {
                pos.x = r.width() - 1;
            }
        }
        else if (key == 'd') {
            pos.x++;

            if (pos.x >= r.width()) {
                pos.x = 0;
            }
        }

        buffer[pos.x + r.width() * pos.y].r = 0;
        buffer[pos.x + r.width() * pos.y].g = 255;
        buffer[pos.x + r.width() * pos.y].b = 0;
    }

    r.reset_console();

    return 0;
}