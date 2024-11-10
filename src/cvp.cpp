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

int main() {
    std::string file = "/Users/josem/Movies/YouTube/new.mp4";
    //std::string file = "D:\\docs\\videos\\oshinoko.mp4";

    Console renderer;
    AVDecoder decoder;

    renderer.set_mode(Console::ColorMode::MODE_256);
    renderer.initialize();
    decoder.open(file.c_str(), true);

    Audio audio(decoder.get_audio_context());

    std::vector<colors::rgb> buffer(renderer.width() * renderer.height());

    AVDecoder::FrameData frame;

    bool done = false;
    bool paused = false;
    double fpsMs = 1.0 / decoder.fps() * 1000;
    uint64_t frameCount = 0;

    auto start = timer::now();
    auto pauseDelta = timer::now();

    while (!done) {
        int key = renderer.handle_keypress();
        if (key == 'q') {
            done = true;
        }
        else if (key == ' ') {
            paused = !paused;

            if (paused) {
                pauseDelta = timer::now();
            }
            else {
                auto diff = timer::now() - pauseDelta;
                start += diff;
            }
        }

        if (paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if (decoder.read_frame(frame) != 0) {
            done = true;
            break;
        }

        if (frame.stream == AVDECODER_STREAM_AUDIO) {
            audio.play(frame.frame);
        } else {
            frameCount++;
            auto elapsed = timer::ms(start, timer::now());
            auto frameMs = fpsMs * frameCount;

            int diff = elapsed - frameMs;

            if (diff < 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(-diff));
            }
            else if (diff > 0.2) {
                continue;
            }

            decoder.decode_video(buffer, renderer.width(), renderer.height());
            renderer.draw(buffer);
        }
    }

    renderer.reset_console();

    return 0;
}