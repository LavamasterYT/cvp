#include "audio.h"
#include "avdecoder.h"
#include "colors.h"
#include "console.h"
#include "config.h"
#include "timer.h"
#include "settings.h"
#include "ui.h"

#include <CLI/CLI.hpp>
#include <fmt/core.h>
#include <fmt/chrono.h>

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

    app.add_option("file", settings::file, "The input file to play")->required();
    app.add_option("-m, --mode", settings::colorMode, "Sets the render mode (ascii, palette, rgb)")
        ->transform(CLI::CheckedTransformer(modeMap, CLI::ignore_case))
        ->default_val(Console::ColorMode::MODE_ASCII);
    app.add_flag("-a,--audio", settings::playAudio, "Plays audio")
        ->default_val(false);
    app.add_flag("-d,--debug", settings::debug, "Shows debug information.");

    CLI11_PARSE(app, argc, argv);
    return 0;
}

int cvp_main(int argc, char** argv) {
    // Load command line switches and configuration file
    if (handle_args(argc, argv) != 0)
        return -1;

    config::conffile conf("cvp", "cvp");

    conf.add_option("audio", settings::playAudio);
    conf.add_option("debug", settings::debug);
    conf.add_option("ui", settings::showUI);
    conf.parse();

    // Open and setup player

    Console renderer;
    AVDecoder decoder;

    if (decoder.open(settings::file.c_str(), settings::playAudio)) {
        fmt::println("An error occurred opening file: {}", settings::file);
        return -1;
    }

    renderer.set_mode(settings::colorMode);
    renderer.initialize();
    decoder.rescale_decoder(renderer.width(), renderer.height());

    ui ui(decoder.duration());
    ui.resize(renderer.width(), renderer.height());

    Audio audio(decoder.get_audio_context());

    if (settings::playAudio)
        audio.init();

    std::vector<colors::rgb> buffer(renderer.width() * renderer.height());

    bool done = false;
    bool paused = false;
    double lastPts = 0.0;

    int syncCounter = 0;

    int64_t uiHideCounterStart = 0;
    bool drawUI = false;

    auto start = timer::now();
    auto pauseDelta = timer::now();

    // Main player loop
    while (!done) {
        int64_t ms_elapsed = timer::ms(start, timer::now());
        double s_elapsed = timer::ms(start, timer::now()) / 1000.0f;
        int key = renderer.handle_keypress();

        if (ms_elapsed - uiHideCounterStart >= 5000)
            drawUI = false;
        else
            drawUI = true;

        switch (key) {
        case 'q':
            done = true;
            break;
        case ' ':
            uiHideCounterStart = ms_elapsed;
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
            uiHideCounterStart = ms_elapsed;
            start = start + std::chrono::milliseconds(5000);
            if (timer::ms(start, timer::now()) < 0)
                start = timer::now();
            decoder.seek(-5000);
            audio.clear_queue();
            break;
        case 'd':
            uiHideCounterStart = ms_elapsed;
            start = start - std::chrono::milliseconds(5000);
            decoder.seek(5000);
            audio.clear_queue();
            break;
        case 's':
            if (!drawUI)
                uiHideCounterStart = ms_elapsed;
            else
                uiHideCounterStart -= 5000;
            break;
        case 'm':
            int cmode = (int)settings::colorMode;
            cmode++;
            if (cmode > 2)
                cmode = 0;
            settings::colorMode = (Console::ColorMode)cmode;
            renderer.set_mode(settings::colorMode);
            break;
        }

        if (drawUI)
            ui.controls(lastPts, paused);

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

        lastPts = frame.pts;

        if (settings::debug)
            renderer.set_title(fmt::format("pts: {:.2f} | {} | duration: {:%H:%M:%S}", frame.pts, frame.stream == AVDECODER_STREAM_VIDEO ? "V" : "A", decoder.duration()));

        if (frame.stream == AVDECODER_STREAM_AUDIO) {
            audio.play(frame.frame);
            continue; // The audio player automatically discards the frame.
        } 

        // Occasionally every couple of seconds, resync to account for any drift
        if (ms_elapsed / 2000 > syncCounter) {
            syncCounter = ms_elapsed / 2000;
            audio.clear_queue();
        }

        if (frame.pts > s_elapsed) { // We are ahead...
            double diff = frame.pts - s_elapsed;
            diff *= 1000.0;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(diff)));
        }
        else // We are behind...
            continue;

        // Decode and render video
        decoder.decode_video(buffer);
        renderer.draw(buffer, drawUI);

        decoder.discard_frame();
    }

    renderer.reset_console();

    return 0;
}