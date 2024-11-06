#pragma once

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include "colors.h"

class Console {

    bool mIsReset;
    long mOldOutMode;
    long mOldInMode;
    int mWidth;
    int mHeight;

    std::atomic<int> mKeypress;
    std::vector<colors::lab> mPalette;
    std::thread mInputThread;

    void GetInputLoop();

public:
    /*
    * Different color modes to output to the terminal.
    */
    enum ColorMode {
        /* ASCII grayscale */
        MODE_ASCII,
        /* Default terminal color palette */
        MODE_16,
        /* Full RGB color space */
        MODE_256
    };

    /*
    * Initializes internal variables.
    */
    Console();
    ~Console();

    /*
    * Sets up the console for output.
    */
    void initialize();

    /*
    * Kind of "reinitializes" the console for the correct color mode or size.
    *
    * Call whenever the color mode or window size changes.
    */
    void reset_state();

    /*
    * Resets the console back to normal.
    */
    void reset_console();

    /*
    * Handles the latest keypress if any.
    *
    * @returns The latest keypress, or -1 if no keypress or if the latest
    * keypress was already handled.
    */
    int handle_keypress();

    /*
    * Draws an RGB buffer to the console.
    * @param buffer The RGB buffer. 
    */
    void draw(std::vector<colors::rgb>& buffer);

    /*
    * Sets the render mode for the console
    */
    void set_mode(Console::ColorMode mode);

    /*
    * Sets the title of the terminal window
    * @param title The title
    */
    void set_title(std::string title);

    /*
    * @returns The render width of the console.
    */
    int width() const { return mWidth; }

    /*
    * @returns The render height of the console.
    */
    int height() const { return mHeight; }

private:
    Console::ColorMode mMode;
};
