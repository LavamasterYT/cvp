#pragma once

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include "colors.h"

/**
 * @brief A class to manipulate the console
 * 
 * Manipulates the console to render an RGB buffer, and handle key input.
 */
class Console {

    bool mIsReset; // Internal state to check if console is modified or not.
    long mOldOutMode; // Windows variable, old console out state.
    long mOldInMode; // Windows variable, old console in state.
    int mWidth; // Internal width
    int mHeight; // Internal height

    std::atomic<int> mKeypress; // Latest key that was pressed
    std::vector<colors::lab> mPalette; // 16 color palette already converted.
    std::thread mInputThread;

    void GetInputLoop(); // Main loop for keypresses

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
    * @param space Draw an empty space below the video for UI drawing.
    */
    void draw(std::vector<colors::rgb>& buffer, bool space = false);

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
