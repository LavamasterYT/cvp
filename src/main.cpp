#include "cvp.h"
#include "debugrenderer.h"

int main(int argc, char** argv) {
    //return cvp_main(argc, argv);

    // Load command line switches and configuration file
    if (handle_args(argc, argv) != 0)
        return -1;

    // Open and setup player

    DebugRenderer renderer;
    AVDecoder decoder;

    if (decoder.open(settings::file.c_str(), settings::playAudio)) {
        fmt::println("An error occurred opening file: {}", settings::file);
        return -1;
    }

    renderer.initialize();
    decoder.rescale_decoder(renderer.width(), renderer.height());

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

        renderer.poll();
        int key = renderer.handle_keypress();

        if (ms_elapsed - uiHideCounterStart >= 5000)
            drawUI = false;
        else
            drawUI = true;

        switch (key) {
        case 'q':
            done = true;
            break;
        }

        AVDecoder::FrameData frame;
        int err = decoder.read_frame(frame);
        if (err != 0) {
            done = true;
            break;
        }

        lastPts = frame.pts;

        fmt::println("elapsed: {:.02f} | pts: {:.02f}", s_elapsed, lastPts);

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

            SDL_Delay(static_cast<int>(diff));
        }
        else
            continue;

        // Decode and render video
        decoder.decode_video(buffer);
        renderer.draw(buffer);

        decoder.discard_frame();
    }

    renderer.destroy();

    return 0;
}