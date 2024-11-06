#include "colors.h"
#include "console.h"
#include "avdecoder.h"

#include <fmt/core.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

struct v2 {
    int x;
    int y;
};

typedef struct v2 v2;

int main() {
    std::string file = "/Users/josem/Movies/YouTube/chainsaw.mp4";

    Console renderer;
    AVDecoder decoder;
    AVDecoder::FrameData frame;

    renderer.set_mode(Console::ColorMode::MODE_ASCII);
    renderer.initialize();

    decoder.open(file.c_str(), true);

    std::vector<colors::rgb> buffer(renderer.width() * renderer.height());

    for (auto& i : buffer) {
        i.r = 0;
        i.g = 0;
        i.b = 0;
    }

    bool done = false;

    while (!done) {

        renderer.draw(buffer);

        int key = renderer.handle_keypress();

        if (key == 'q') {
            done = true;
        }
        
        if (decoder.read_frame(frame) != 0) {
            done = true;
            break;
        }

        if (frame.stream == AVDECODER_STREAM_VIDEO) {
            decoder.decode_video(buffer, renderer.width(), renderer.height());
        }
        else {
            decoder.discard_frame(frame);
        }

       // std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    renderer.reset_console();

    return 0;
}