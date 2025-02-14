#pragma once

#include "console.h"

#include <string>

namespace settings {
    extern std::string file;
    extern bool playAudio;
    extern bool showUI;
    extern bool debug;
    extern Console::ColorMode colorMode;
}
