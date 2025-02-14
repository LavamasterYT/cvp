#include "audio.h"
#include "avdecoder.h"
#include "colors.h"
#include "console.h"
#include "timer.h"
#include "config.h"

#include <CLI/CLI.hpp>
#include <fmt/core.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

int handle_args(int argc, char** argv) {
    CLI::App app {"Plays multimedia files in the command line."};
    argv = app.ensure_utf8(argv);

    std::map<std::string, Console::ColorMode> modeMap = {
        {"ascii", Console::ColorMode::MODE_ASCII},
        {"palette", Console::ColorMode::MODE_16},
        {"rgb", Console::ColorMode::MODE_256}
    };

    app.add_option("file", config::file, "The input file to play")->required();
    app.add_option("-m, --mode", config::colorMode, "Sets the render mode (ascii, palette, rgb)")
        ->transform(CLI::CheckedTransformer(modeMap, CLI::ignore_case))
        ->default_val(Console::ColorMode::MODE_ASCII);
    app.add_flag("-a,--audio", config::playAudio, "Plays audio");

    CLI11_PARSE(app, argc, argv);
    return 0;
}

int main(int argc, char** argv) {
    if (handle_args(argc, argv) != 0)
        return -1;

    Console renderer;
    AVDecoder decoder;

    if (decoder.open(config::file.c_str(), config::playAudio)) {
        fmt::println("An error occurred opening file: {}", config::file);
        return -1;
    }

    renderer.set_mode(config::colorMode);
    renderer.initialize();

    Audio audio(decoder.get_audio_context());

    if (config::playAudio)
        audio.init();

    std::vector<colors::rgb> buffer(renderer.width() * renderer.height());

    bool done = false;
    bool paused = false;

    int syncCounter = 0;

    auto start = timer::now();
    auto pauseDelta = timer::now();

    while (!done) {
        int key = renderer.handle_keypress();

        switch (key) {
        case 'q':
            done = true;
            break;
        case ' ':
            paused = !paused;
            if (paused) {
                audio.clear_queue();
                pauseDelta = timer::now();
            } else {
                auto diff = timer::now() - pauseDelta;
                start += diff;
            }
            break;
        case 'a':
            start = start + std::chrono::milliseconds(5000);
            if (timer::ms(start, timer::now()) < 0)
                start = timer::now();
            decoder.seek(-5000);
            audio.clear_queue();
            break;
        case 'd':
            start = start - std::chrono::milliseconds(5000);
            decoder.seek(5000);
            audio.clear_queue();
            break;
        case 'm':
            int cmode = (int)config::colorMode;
            cmode++;
            if (cmode > 2)
                cmode = 0;
            config::colorMode = (Console::ColorMode)cmode;
            renderer.set_mode(config::colorMode);
            break;
        }

        if (paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        AVDecoder::FrameData frame;
        int err = decoder.read_frame(frame);
        if (err != 0) {
            done = true;
            break;
        }

        if (frame.stream == AVDECODER_STREAM_AUDIO) {
            audio.play(frame.frame);
            continue; // The audio player automatically discards the frame.
        }

        int64_t ms_elapsed = timer::ms(start, timer::now());
        double s_elapsed = timer::ms(start, timer::now()) / 1000.0f;

        // Occasionally every second, resync to account for any drift
        if (ms_elapsed / 2000 > syncCounter) {
            syncCounter = ms_elapsed / 2000;
            audio.clear_queue();
        }

        if (frame.pts > s_elapsed) { // ahead
            double diff = frame.pts - s_elapsed;
            diff *= 1000.0;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(diff)));
        }
        else // behind
            continue;

        // Decode and render video
        decoder.decode_video(buffer, renderer.width(), renderer.height());
        renderer.draw(buffer);

        decoder.discard_frame();
    }

    renderer.reset_console();

    return 0;
}