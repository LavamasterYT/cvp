#include <chrono>
#include <csignal>
#include <cstdio>
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <filesystem>

#include <fmt/core.h>

#include "colors.h"
#include "decoder.h"
#include "renderer.h"
#include "input.h"
#include "ui.h"
#include "audio.h"

bool requested_int = false;
bool network_url = false;

std::unique_ptr<renderer> r;
std::unique_ptr<decoder> d;
std::unique_ptr<audio> a;

void set_signals()
{
    signal(SIGINT, [ ] (int) { requested_int = true; });
    signal(SIGWINCH, [ ] (int) { if (r != nullptr) r->set_dimensions(); });
}

bool handle_args(int argc, char** argv, std::string* f, int* mode, bool* ui, bool* play_audio)
{
    *mode = RENDERER_MODE_ASCII;
    *ui = true;
    *play_audio = false;
    
    auto help = []
    {
        fmt::println("Usage: cvp [options] <file>");
        fmt::println("Plays a video file in the terminal!");
        fmt::println("");
        fmt::println("-a, --play-audio      Play audio");
        fmt::println("-u, --no-controls     Don't render controls");
        fmt::println("-f, --full-color      Render in full color");
        fmt::println("-p, --palette         Render in 16 color palette");
    };
    
    auto ver = []
    {
        fmt::println("cvp 2.0a");
    };
    
    if (argc == 1)
    {
        fmt::println("No input file given.");
        fmt::println("Usage: cvp [options] <file>");
        return false;
    }
    else
    {
        for (int i = 1; i < argc; i++)
        {
            if (argv[i][0] == '-')
            {
                if (strlen(argv[i]) >= 2)
                {
                    if (argv[i][1] == '-')
                    {
                        if (strcmp(argv[i], "--version") == 0)
                        {
                            ver();
                            return false;
                        }
                        else if (strcmp(argv[i], "--help") == 0)
                        {
                            help();
                            return false;
                        }
                        else if (strcmp(argv[i], "--play-audio") == 0) *play_audio = true;
                        else if (strcmp(argv[i], "--no-controls") == 0) *ui = false;
                        else if (strcmp(argv[i], "--full-color") == 0) *mode = RENDERER_MODE_FULL_COLOR;
                        else if (strcmp(argv[i], "--palette") == 0) *mode = RENDERER_MODE_PALETTE;
                        else
                        {
                            fmt::println("cvp: invalid option -- '{}'", argv[i]);
                            fmt::println("Try 'cvp --help' for more information.\n");
                            return false;
                        }
                    }
                    else
                    {
                        for (size_t j = 1; j < strlen(argv[i]); j++)
                        {
                            if (argv[i][j] == 'f') *mode = RENDERER_MODE_FULL_COLOR;
                            else if (argv[i][j] == 'p') *mode = RENDERER_MODE_PALETTE;
                            else if (argv[i][j] == 'u') *ui = false;
                            else if (argv[i][j] == 'a') *play_audio = true;
                            else if (argv[i][j] == 'h' || argv[i][j] == '?')
                            {
                                help();
                                return false;
                            }
                            else if (argv[i][j] == 'v')
                            {
                                ver();
                                return false;
                            }
                            else
                            {
                                fmt::println("cvp: invalid option -- '{}'", argv[i]);
                                fmt::println("Try 'cvp --help' for more information.\n");
                                return false;
                            }
                        }
                    }
                }
            }
            else
            {
                if (std::filesystem::exists(argv[i]))
                {
                    f->clear();
                    f->append(argv[i]);
                }
                else
                {
                   network_url = true;
                }
            }
        }
    }

    return true;
}

int old_main(int argc, char** argv)
{
    std::vector<colors_rgb> buffer;
    std::string file;
    double elapsed = 0;
    double pts = 0;
    double audio_pts = 0;
    double fd = 0;
    bool render_controls = true;
    bool play_audio = false;
    bool paused = false;
    int mode = RENDERER_MODE_ASCII;
    int s_w = 0;
    int s_h = 0;

    set_signals();

    if (!handle_args(argc, argv, &file, &mode, &render_controls, &play_audio))
        return 1;

    r = std::make_unique<renderer>();
    d = std::make_unique<decoder>();

    if (d->open(file, play_audio))
    {
        r.~unique_ptr();
        fmt::println("Unable to open media file \"{}\"!", file);
        return 1;
    }
    
    if (render_controls)
        r->height -= 2;
    r->mode = mode;

    if (play_audio)
        a = std::make_unique<audio>(d->get_audio_ctx());

    fd = 1.0 / d->fps;

    auto start = std::chrono::steady_clock::now();
    auto delta = std::chrono::steady_clock::now();

    while (1)
    {
        if (requested_int)
            break;

        if (key_pending())
        {
            int key = get_key();

            switch (key)
            {
                case 'q':
                    requested_int = 1;
                    break;
                case '1':
                    r->mode = RENDERER_MODE_ASCII;
                    break;
                case '2':
                    r->mode = RENDERER_MODE_FULL_COLOR;
                    break;
                case '3':
                    r->mode = RENDERER_MODE_PALETTE;
                    break;
                case 'a': {
                        auto new_ms = elapsed - 5000;
                        if (new_ms < 0)
                            new_ms = 0;
                        d->seek(new_ms);
                        start += std::chrono::milliseconds(static_cast<int>(elapsed - new_ms));
                    } break;
                case 'd': {
                        auto new_ms = elapsed + 5000;
                        d->seek(new_ms);
                        start -= std::chrono::milliseconds(static_cast<int>(new_ms - elapsed));
                    } break;
                case ' ':
                    paused = !paused;

                    if (paused == false)
                    {
                        auto dif = std::chrono::steady_clock::now() - delta;
                        start += dif;
                    }
                    else
                        delta = std::chrono::steady_clock::now();
                    break;
            }
        }

        if (paused)
            continue;

        int fi = d->read_frame(&pts);

        if (fi < 0) // eof
            break;

        if (fi == d->audio_index)
        {
            a->play(d->get_raw_frame());
            audio_pts = pts;
            continue;
        }

        auto current = std::chrono::steady_clock::now();
        elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current - start).count();
        
        double diff = pts - audio_pts;
        
        if (diff > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(diff)));
        }

        // If video is behind, drop the frame
        else if (diff < -0.1) {
            continue;  // Skip this frame
        }
        
        d->decode(buffer, r->width, r->height, &s_w, &s_h, r->mode == RENDERER_MODE_ASCII);
        r->draw(buffer, s_w, s_h);
        
        if (render_controls)
            ui_draw(paused, static_cast<int>(elapsed), d->duration, r->width, r->height);
    }

    r.~unique_ptr();
    d.~unique_ptr();
    a.~unique_ptr();

    return 0;
}
