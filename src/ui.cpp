#include "ui.h"

#include <fmt/core.h>
#include <fmt/chrono.h>

#include <chrono>
#include <string>

#define ESC "\x1B"
#define CSI "\x1B["

ui::ui(std::chrono::seconds duration) {
    mDraw = false;
    mDuration = duration;
    mWindowHeight = 0;
    mProgressLength = 0;
}

void ui::resize(int windowWidth, int windowHeight) {
    // UI will look like this: |[⏵][00:00]━━--------------[59:59]|
    //                     or: |[⏸][00:00:00]━━--------[59:59:59]|

    int uiElementCount = 17; // How much UI space the time, symbols, and other stuff takes up.
    int minimumProgressWidth = 2; // The minimum size for the progress bar.

    // Find out if we are doing hours
    if (mDuration.count() >= 3660)
        uiElementCount += 6; // Account for hours

    // If the width is too small, don't bother drawing the UI.
    if (windowWidth < uiElementCount + minimumProgressWidth) {
        mDraw = false;
        return;
    }

    mProgressLength = windowWidth - uiElementCount;
    mWindowHeight = windowHeight;

    mDraw = true;
}

void ui::controls(int64_t elapsed, bool paused) {
    if (mDraw == false)
        return;

    std::string progressBuilder = "";
    
    double percentageElapsed = static_cast<double>(elapsed) / mDuration.count(); // Percentage of elapsed time
    int filled = percentageElapsed * static_cast<double>(mProgressLength); // How much progress to fill
    int notFilled = mProgressLength - filled;

    if (filled > mProgressLength)
        filled = mProgressLength;

    for (int i = 0; i < filled; i++)
        progressBuilder += "━";
    
    for (int i = 0; i < notFilled; i++)
        progressBuilder += "·";

    fmt::print(
        CSI "{};0H" // Set cursor position
        CSI "0m" // Set colors
        "[{}]", // Pause/play button
        mWindowHeight - 1,
        paused ? "⏵" : "⏸");

    // Elapsed duration
    if (mDuration.count() >= 3660)
        fmt::print("[{:%H:%M:%S}]", std::chrono::seconds(elapsed));
    else
        fmt::print("[{:%M:%S}]", std::chrono::seconds(elapsed));
    
    // Progress bar
    fmt::print("{}", progressBuilder);

    // Total duration
    if (mDuration.count() >= 3660)
        fmt::print("[{:%H:%M:%S}]", mDuration);
    else
        fmt::print("[{:%M:%S}]", mDuration);
}
