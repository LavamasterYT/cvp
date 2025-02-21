#pragma once

#include <chrono>
#include <cinttypes>
#include <string>

class ui {
private:
    int mWindowHeight;
    int mProgressLength;
    bool mDraw;
    std::chrono::seconds mDuration;

public:
    /**
     * @brief Initializes a new UI object.
     * 
     * @param duration The duration in seconds of the media.
     */
    ui(std::chrono::seconds duration);

    void resize(int windowWidth, int windowHeight);
    void controls(int64_t elapsed, bool paused);
};
