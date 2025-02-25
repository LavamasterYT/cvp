#include "debugrenderer.h"

#include <SDL2/SDL.h>
#include <fmt/core.h>

DebugRenderer::DebugRenderer() {
    mWidth = 1280;
    mHeight = 720;
    mKeypress = -1;
    destroyed = false;
}

void DebugRenderer::initialize() {
    SDL_Init(SDL_INIT_VIDEO);

    mWindow = SDL_CreateWindow("cvp", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, mWidth, mHeight, SDL_WINDOW_SHOWN);
    mRenderer = SDL_CreateRenderer(mWindow, -1, SDL_RENDERER_ACCELERATED);
    mPixelBuffer = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 1280, 720);
}

void DebugRenderer::poll() {
    SDL_PollEvent(&mEvent);

    if (mEvent.type == SDL_QUIT) {
        mKeypress = 'q';
    }
    else if (mEvent.type == SDL_KEYDOWN) {
        switch (mEvent.key.keysym.scancode) {
            case SDL_SCANCODE_Q:
                mKeypress = 'q';    
                break;
        }
    }
}

void DebugRenderer::draw(std::vector<colors::rgb>& buffer) {
    int pitch;
    colors::rgb* buf;

    if (buffer.empty()) {
        return;
    }

    SDL_LockTexture(mPixelBuffer, NULL, (void**)&buf, &pitch);
    for (int i = 0; i < buffer.size(); i++) {
        buf[i].r = buffer[i].r;
        buf[i].g = buffer[i].g;
        buf[i].b = buffer[i].b;
    }
    SDL_UnlockTexture(mPixelBuffer);

    SDL_RenderCopy(mRenderer, mPixelBuffer, NULL, NULL);
    SDL_RenderPresent(mRenderer);
}

int DebugRenderer::handle_keypress() {
    int key = mKeypress;
    mKeypress = -1;
    return key;
}

void DebugRenderer::destroy() {
    if (destroyed)
        return;

    SDL_Quit();
    SDL_DestroyTexture(mPixelBuffer);
    SDL_DestroyRenderer(mRenderer);
    SDL_DestroyWindow(mWindow);
    destroyed = true;
}

DebugRenderer::~DebugRenderer() {
    destroy();
}
