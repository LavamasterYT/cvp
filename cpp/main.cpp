#include <chrono>
#include <csignal>
#include <cstdio>
#include <iostream>
#include <memory>
#include <vector>
#include <thread>

#include <fmt/core.h>

#include "colors.h"
#include "decoder.h"
#include "renderer.h"
#include "input.h"
#include "ui.h"
#include "audio.h"

bool requested_int = false;

std::unique_ptr<renderer> r = std::make_unique<renderer>();
std::unique_ptr<decoder> d = std::make_unique<decoder>();
std::unique_ptr<audio> a(nullptr);

void set_signals()
{
    signal(SIGINT, [ ] (int) { requested_int = true; });
    signal(SIGWINCH, [ ] (int) { if (r != nullptr) r->set_dimensions(); });
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        fmt::println("Usage: cvp [options] <input>");
        return -1;
    }

    set_signals();

    d->open(argv[1], true);
    r->height -= 2;
    r->mode = RENDERER_MODE_ASCII;

    a = std::make_unique<audio>(d->get_audio_ctx());

    std::vector<colors_rgb> buffer;

    bool paused = false;

    int s_w, s_h;

    double pts;
    double fd = 1.0 / d->fps;
    double elapsed = 0;

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
            continue;
        }

        auto current = std::chrono::steady_clock::now();
        elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current - start).count();

        double delay = pts - (elapsed / 1000);

        if (delay > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(delay * 1000)));
        else if (delay < -fd)
            continue;

        d->decode(buffer, r->width, r->height, &s_w, &s_h, r->mode == RENDERER_MODE_ASCII);
        r->draw(buffer, s_w, s_h);
        ui_draw(paused, static_cast<int>(elapsed), d->duration, r->width, r->height);
    }

    r.~unique_ptr();
    d.~unique_ptr();
    a.~unique_ptr();

    return 0;
}