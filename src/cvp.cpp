#include "audio.h"
#include "avdecoder.h"
#include "colors.h"
#include "console.h"
#include "timer.h"

#include <CLI/CLI.hpp>
#include <fmt/core.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

int main(int argc, char** argv) {
    CLI::App app {"Plays multimedia files in the command line."};
    argv = app.ensure_utf8(argv);

    std::map<std::string, Console::ColorMode> modeMap = {
        {"ascii", Console::ColorMode::MODE_ASCII},
        {"palette", Console::ColorMode::MODE_16},
        {"rgb", Console::ColorMode::MODE_256}
    };

    std::string file = "";
    Console::ColorMode mode;
    bool playAudio = false;

    app.add_option("file", file, "The input file to play")->required();
    app.add_option("-m, --mode", mode, "Sets the render mode (ascii, palette, rgb)")
        ->transform(CLI::CheckedTransformer(modeMap, CLI::ignore_case))
        ->default_val(Console::ColorMode::MODE_ASCII);
    app.add_flag("-a,--audio", playAudio, "Plays audio");

    CLI11_PARSE(app, argc, argv);

    Console renderer;
    AVDecoder decoder;

    decoder.open(file.c_str(), playAudio);

    renderer.set_mode(mode);
    renderer.initialize();

    Audio audio(decoder.get_audio_context());

    if (playAudio) {
        playAudio = audio.init();
    }

    std::vector<colors::rgb> buffer(renderer.width() * renderer.height());

    AVDecoder::FrameData frame;

    int err;

    bool done = false;
    bool paused = false;
    double fpsMs = 1.0 / decoder.fps() * 1000;
    uint64_t frameCount = 0;

    auto start = timer::now();
    auto pauseDelta = timer::now();
    auto targetTime = start;

    while (!done) 
    {
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

        err = decoder.read_frame(frame);
        if (err != 0) {
            done = true;
            break;
        }

        if (frame.stream == AVDECODER_STREAM_AUDIO) {
            if (playAudio) {
                audio.play(frame.frame);
            }
            continue;
        }

        double framePTS = frame.pts;

        double masterClock = 0.0;
        if (playAudio) {
            masterClock = audio.get_clock();
        }
        else {
            auto now = timer::now();
            double elapsedMs = timer::ms(start, now);
            masterClock = elapsedMs / 1000.0;
        }

        double diff = framePTS - masterClock;

        if (diff > 0) {
            auto sleepDuration = std::chrono::duration<double>(diff);
            std::this_thread::sleep_for(sleepDuration);
        }
        else {
            continue;
        }

        decoder.decode_video(buffer, renderer.width(), renderer.height());
        renderer.draw(buffer);
    }

    renderer.reset_console();

    return 0;
}