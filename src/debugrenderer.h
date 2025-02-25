#pragma once

#include <atomic>
#include <vector>
#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL2/SDL.h>
#include "colors.h"

/**
 * @brief A class to manipulate the console
 * 
 * Manipulates the console to render an RGB buffer, and handle key input.
 */
class DebugRenderer {
private:
    int mWidth; // Internal width
    int mHeight; // Internal height

    bool destroyed;

    SDL_Window* mWindow;
    SDL_Renderer* mRenderer;
    SDL_Texture* mPixelBuffer;

    SDL_Event mEvent;

    std::atomic<int> mKeypress; // Latest key that was pressed
public:

    /*
    * Initializes internal variables.
    */
    DebugRenderer();
    ~DebugRenderer();

    /*
    * Sets up the console for output.
    */
    void initialize();

    /*
    * Handles the latest keypress if any.
    *
    * @returns The latest keypress, or -1 if no keypress or if the latest
    * keypress was already handled.
    */
    int handle_keypress();

    void poll();

    /*
    * Draws an RGB buffer to the console.
    * @param buffer The RGB buffer. 
    * @param space Draw an empty space below the video for UI drawing.
    */
    void draw(std::vector<colors::rgb>& buffer);

    /*
    * @returns The render width of the console.
    */
    int width() const { return mWidth; }

    /*
    * @returns The render height of the console.
    */
    int height() const { return mHeight; }

    void destroy();
};
