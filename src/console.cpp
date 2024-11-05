#include "console.h"

#include <vector>

#include <fmt/core.h>

#ifdef _WIN32

#include <conio.h>
#include <Windows.h>

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

    while (!mIsReset) {
        if (_kbhit()) {
            mKeypress = _getch();
        }
    }
}

void Console::initialize() {
    mIsReset = false;

    mPalette.resize(16);

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
	HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);

	SetConsoleMode(hout, mOldOutMode);
	SetConsoleMode(hin, mOldInMode);
    #else
    struct termios term;
    tcgetattr(0, &term);
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(0, TCSANOW, &term);
    #endif

    mInputThread = std::thread(&Console::GetInputLoop, this);

    fmt::print(CSI "?1049h" CSI "?25l");

    reset_state();
}

void Console::draw(std::vector<colors::rgb>& buffer) {
	const char* ascii = " `.-':_,^=;><+!rc*/z?sLTv)J7(|Fi{C}fI31tlu[neoZ5Yxjya]2ESwqkP6h9d4VpOGbUAKXHm8RD#$Bg0MNWQ%&@";
    int dY = (mMode == MODE_ASCII) ? 1 : 2; // If ASCII, drawing takes up 1 full character instead of half
    colors::rgb oldTop = { 0, 0, 0 };
    colors::rgb oldBottom = { 0, 0, 0};
    
    fmt::print(CSI "0;0H");

    if (mMode == MODE_256) {
        fmt::print(CSI "38;2;0;0;0m" CSI "48;2;0;0;0m▀");
    }
    else if (mMode == MODE_16) {
        // TODO: reset colors
    }

    for (int y = 0; y < mHeight; y += dY) {
        fmt::print(CSI "{};0H", (y / dY) + 1); // Set cursor to beginning of next line

        for (int x = 0; x < mWidth; x++) {
            switch (mMode) {
            case MODE_ASCII: {
                float grayscaleValue = static_cast<float>(rgb_to_grayscale(buffer[x + mWidth * y]));
                int asciiIndex = static_cast<int>(grayscaleValue / 255.0f * strlen(ascii));

                fmt::print("{}", ascii[asciiIndex]);
            } break;
            case MODE_16: {

            }break;
            case MODE_256: {
                colors::rgb top = buffer[x + mWidth * y];
                colors::rgb bottom = buffer[x + mWidth * (y + 1)];

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
    #ifdef _WIN32
	// Get handles
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD dwOgOut = 0;
	DWORD dwOgIn = 0;

	// get current console information
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

    if (mMode != MODE_ASCII) {
        mHeight *= 2;
    }
}

void Console::reset_console() {
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

int Console::handle_keypress() {
    int key = mKeypress;
    mKeypress = -1;
    return key;
}

void Console::set_mode(Console::ColorMode mode) {
    mMode = mode;

    if (!mIsReset)
        reset_state();
}
