#include "audio.h"
#include "avdecoder.h"
#include "colors.h"
#include "console.h"
#include "timer.h"

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
    std::string file = "/Users/josem/Movies/YouTube/new.mp4";
    //std::string file = "D:\\docs\\videos\\oshinoko.mp4";

    Console renderer;
    AVDecoder decoder;
    AVDecoder::FrameData frame;

    renderer.set_mode(Console::ColorMode::MODE_256);
    renderer.initialize();

    decoder.open(file.c_str(), true);

    Audio audio(decoder.get_audio_context());

    std::vector<colors::rgb> buffer(renderer.width() * renderer.height());

    for (auto& i : buffer) {
        i.r = 0;
        i.g = 0;
        i.b = 0;
    }

    bool done = false;
    auto start = timer::now();

    while (!done) {
        int key = renderer.handle_keypress();

        if (key == 'q') {
            done = true;
        }
        
        if (decoder.read_frame(frame) != 0) {
            done = true; // End of file
            break;
        }

        if (frame.stream == AVDECODER_STREAM_AUDIO) {
            audio.play(frame.frame);
        }
        else {
            double clock = audio.get_clock();
            double delay = frame.pts - clock;

            renderer.set_title(fmt::format("{:.2f} {:.2f} {:.2f}", frame.pts, clock, delay));

            if (delay > 0.1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(delay)));
            }
            else if (delay < -0.05) {
                continue;
            }

            decoder.decode_video(buffer, renderer.width(), renderer.height());
            renderer.draw(buffer); // This has a chance to take a while (up to 60ms!) or little (1ms)
        }

        auto elapsed = timer::ms(start, timer::now());
    }

    renderer.reset_console();

    return 0;
}