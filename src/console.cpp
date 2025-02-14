#include "console.h"

#include <chrono>
#include <thread>
#include <vector>

#include <fmt/core.h>

#ifdef _WIN32

#include <conio.h>
#include <Windows.h>

#undef max

#elif defined(__unix__) || defined(__APPLE__)

#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#endif

#include "colors.h"

#define ESC "\x1B"
#define CSI "\x1B["

Console::Console() : mInputThread { } {
    mIsReset = true;
    mWidth = 0;
    mHeight = 0;
    mKeypress = -1;
    mMode = Console::ColorMode::MODE_ASCII;
}

Console::~Console() {
    if (!mIsReset)
        reset_console();
}

void Console::GetInputLoop() {
    #if defined(__unix__) || defined(__APPLE__)

    // Custom implementations of kbhit and getch for posix systems

    auto _kbhit = [&] () {
        struct timeval tv = { 0L, 0L };
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        return select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv);
    };

    auto _getch = [&] () {
        char ch;
        if (read(STDIN_FILENO, &ch, 1) < 0) {
            return -1;
        }
        return (int)ch;
    };

    #endif

    // Main input loop
    while (!mIsReset) {
        if (_kbhit()) {
            mKeypress = _getch();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void Console::initialize() {
    mIsReset = false;

    mPalette.resize(16);

    // Converts the 16 console colors to CIELAB
	mPalette[0] = colors::rgb_to_lab({12, 12, 12});
	mPalette[1] = colors::rgb_to_lab({197, 15, 31});
	mPalette[2] = colors::rgb_to_lab({19, 161, 14});
	mPalette[3] = colors::rgb_to_lab({193, 161, 14});
	mPalette[4] = colors::rgb_to_lab({0, 55, 218});
	mPalette[5] = colors::rgb_to_lab({136, 23, 152});
	mPalette[6] = colors::rgb_to_lab({58, 150, 221});
	mPalette[7] = colors::rgb_to_lab({204, 204, 204});

	mPalette[8] = colors::rgb_to_lab({118, 118, 118});
	mPalette[9] = colors::rgb_to_lab({231, 72, 86});
	mPalette[10] = colors::rgb_to_lab({22, 198, 12});
	mPalette[11] = colors::rgb_to_lab({249, 241, 165});
	mPalette[12] = colors::rgb_to_lab({59, 120, 255});
	mPalette[13] = colors::rgb_to_lab({180, 0, 158});
	mPalette[14] = colors::rgb_to_lab({97, 214, 214});
	mPalette[15] = colors::rgb_to_lab({242, 242, 242});

    #ifdef _WIN32

    // TODO: i realized this isnt correct, fix when im on Windows
	HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);

	SetConsoleMode(hout, mOldOutMode);
	SetConsoleMode(hin, mOldInMode);

    #else

    // Disables echo and canical mode
    struct termios term;
    tcgetattr(0, &term);
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(0, TCSANOW, &term);

    #endif

    mInputThread = std::thread(&Console::GetInputLoop, this);

    // Alternative buffer and cursor shape
    fmt::print(CSI "?1049h" CSI "?25l");

    reset_state();
}

void Console::draw(std::vector<colors::rgb>& buffer) {
    // ASCII grayscale values
	const char* ascii = " `.-':_,^=;><+!rc*/z?sLTv)J7(|Fi{C}fI31tlu[neoZ5Yxjya]2ESwqkP6h9d4VpOGbUAKXHm8RD#$Bg0MNWQ%&@";
    colors::rgb oldTop = { 0, 0, 0 };
    colors::rgb oldBottom = { 0, 0, 0};
    
    // Reset the cursor
    fmt::print(CSI "0;0H");

    // Reset the color
    if (mMode == MODE_256)
        fmt::print(CSI "38;2;0;0;0m" CSI "48;2;0;0;0m▀");
    else if (mMode == MODE_ASCII)
        fmt::print(ESC "[0m");

    for (int y = 0; y < mHeight; y += 2) {
        fmt::print(CSI "{};0H", (y / 2) + 1); // Set cursor to beginning of next line

        for (int x = 0; x < mWidth; x++) {
            switch (mMode) {
            case MODE_ASCII: {
                // Convert value to grayscale 0-255, then find appropriate index
                float grayscaleValue = static_cast<float>(rgb_to_grayscale(buffer[x + mWidth * y]));
                int asciiIndex = static_cast<int>(grayscaleValue / 255.0f * strlen(ascii));

                fmt::print("{}", ascii[asciiIndex]);
            } break;
            case MODE_16: {
                // Converts the pixels of the current x,y and x,y+1 to CIELAB,
                // and then finds the closest match from the console palette.
                //
                // I know I can run the euclidean algorithm with the RGB values
                // and it would be faster, however I plan to emphasize with hue
                // and it is easier to do that with the CIELAB colorspace, just
                // haven't gotten around to do it yet.
                //
                // TODO: better color matching
                int top, bottom;
                int distanceTop = std::numeric_limits<int>::max();
                int distanceBottom = distanceTop;
                colors::lab currentTop = colors::rgb_to_lab(buffer[x + mWidth * y]);
                colors::lab currentBottom = colors::rgb_to_lab(buffer[x + mWidth * (y + 1)]);

                for (int i = 0; i < mPalette.size(); i++) {
                    int t = colors::euclidean_lab(currentTop, mPalette[i]);
                    int b = colors::euclidean_lab(currentBottom, mPalette[i]);

                    if (distanceTop > t) {
                        distanceTop = t;
                        top = i <= 7 ? 30 + i : 90 + i % 8;
                    }
                    if (distanceBottom > b) {
                        distanceBottom = b;
                        bottom = i <= 7 ? 40 + i : 100 + i % 8;
                    }
                }

                // Avoid any unnecessary writes to stdout. Whether this saves performance idk
                if (top == oldTop.r && bottom == oldBottom.r)
                    fmt::print("▀");
                else if (top == oldTop.r)
                    fmt::print(CSI "{}m▀", bottom);
                else if (bottom == oldBottom.r)
                    fmt::print(CSI "{}m▀", top);
                else
                    fmt::print(CSI "{}m" CSI "{}m▀", top, bottom);

                oldTop.r = top;
                oldBottom.r = bottom;

            } break;
            case MODE_256: {
                colors::rgb top = buffer[x + mWidth * y];
                colors::rgb bottom = buffer[x + mWidth * (y + 1)];

                // Avoid any unnecessary writes to stdout. Whether this saves performance idk
                if (colors::compare_rgb(top, oldTop) && colors::compare_rgb(bottom, oldBottom))
                    fmt::print("▀");
                else if (colors::compare_rgb(top, oldTop))
                    fmt::print(CSI "48;2;{};{};{}m▀", bottom.r, bottom.g, bottom.b);
                else if (colors::compare_rgb(bottom, oldBottom))
                    fmt::print(CSI "38;2;{};{};{}m▀", top.r, top.g, top.b);
                else
				    fmt::print(CSI "38;2;{};{};{}m" CSI "48;2;{};{};{}m▀", top.r, top.g, top.b, bottom.r, bottom.g, bottom.b);
                    
                oldTop = top;
                oldBottom = bottom;

            } break;
            }
        }
    }
}

void Console::reset_state() {
    // Just resets the width and height of the console
    
    #ifdef _WIN32
    // TODO: i realized this isnt correct, fix when im on Windows
	
    // Get handles
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD dwOgOut = 0;
	DWORD dwOgIn = 0;

	// Fet current console information
	GetConsoleMode(hout, &dwOgOut);
	GetConsoleMode(hin, &dwOgIn);
	GetConsoleScreenBufferInfo(hout, &csbi);

	mOldInMode = dwOgIn;
	mOldOutMode = dwOgOut;

	// set console mode
	DWORD dwInMode = dwOgIn | ENABLE_VIRTUAL_TERMINAL_INPUT;
	DWORD dwOutMode = dwOgOut | (ENABLE_VIRTUAL_TERMINAL_PROCESSING);

	SetConsoleMode(hout, dwOutMode);
	SetConsoleMode(hin, dwInMode);

	// get console width and height
	mWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	mHeight = (csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
    #elif defined(__unix__) || defined(__APPLE__)

	struct winsize size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);

	mWidth = size.ws_col;
	mHeight = size.ws_row;

    #endif

    mHeight *= 2;
}

void Console::reset_console() {
    // Goes back to main buffer, resets cursor, and set console mode back to normal.
    fmt::print(CSI "?1049l" CSI "?25h");

    #ifdef _WIN32
    HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);

	SetConsoleMode(hout, mOldOutMode);
	SetConsoleMode(hin, mOldInMode);
    #else

    struct termios term;
    tcgetattr(0, &term);
    term.c_lflag |= ICANON | ECHO;
    tcsetattr(0, TCSANOW, &term);
    tcflush(0, TCIFLUSH);

    #endif

    mIsReset = true;

    mInputThread.join();
}

void Console::set_title(std::string title) {
    // Self explanitory
    fmt::print(ESC "]0;{}\x07", title);
}

int Console::handle_keypress() {
    int key = mKeypress;
    mKeypress = -1;
    return key;
}

void Console::set_mode(Console::ColorMode mode) {
    // This whole thing could probably be made public, but idk ill change it later
    mMode = mode;

    // This MIGHT be unnecessary lol
    if (!mIsReset)
        reset_state();
}
