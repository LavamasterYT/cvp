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

    int syncCounter = 0;

    auto start = timer::now();
    auto pauseDelta = timer::now();

    while (!done) {
        int key = renderer.handle_keypress();
        if (key == 'q') {
            done = true;
        } else if (key == ' ') {
            paused = !paused;
            if (paused) {
                pauseDelta = timer::now();
            } else {
                auto diff = timer::now() - pauseDelta;
                start += diff;
            }
        }
        else if (key == 'a') {
            decoder.seek(-5000);
            start = start + std::chrono::milliseconds(5000);
            if (playAudio)
                audio.clear_queue();
        }
        else if (key == 'd') {
            decoder.seek(5000);
            start = start - std::chrono::milliseconds(5000);
            if (playAudio)
                audio.clear_queue();
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
            if (playAudio) {
                audio.play(frame.frame);

                if (audio.get_queued_time() > 0.1) {
                    audio.clear_queue();
                }
            }
            
            continue; // The audio player automatically discards the frame.
        }

        int ms_elapsed = timer::ms(start, timer::now());
        double s_elapsed = timer::ms(start, timer::now()) / 1000.0f;

        // Occasionally every second, resync to account for any drift
        if (ms_elapsed / 1000 > syncCounter) {
            syncCounter = ms_elapsed / 1000;
            audio.clear_queue();
        }

        renderer.set_title(fmt::format("pts: {:.2f} | elapsed: {:.2f}", frame.pts, s_elapsed));

        if (frame.pts > s_elapsed) { // ahead
            double diff = frame.pts - s_elapsed;
            diff *= 1000.0;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(diff)));
        }
        else { // behind
            continue;
        }

        // Decode and render video
        decoder.decode_video(buffer, renderer.width(), renderer.height());
        renderer.draw(buffer);

        decoder.discard_frame();
    }

    renderer.reset_console();

    return 0;
}