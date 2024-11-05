#include "console.h"

#include <vector>

#include <fmt/core.h>
#include <ncurses.h>

#ifdef _WIN32

#include <Windows.h>
#define RENDERER_PIXEL_CHAR "\xDF"

#elif defined(__unix__) || defined(__APPLE__)

#include <sys/ioctl.h>
#include <unistd.h>
#define RENDERER_PIXEL_CHAR "▀"

#endif

#include "colors.h"

#define ESC "\x1B"
#define CSI "\x1B["

Console::Console() : mInputThread { } {
    mIsReset = true;
    mWidth = 0;
    mHeight = 0;
    mKeypress = ERR;
    mMode = Console::ColorMode::MODE_ASCII;
}

Console::~Console() {
    if (!mIsReset)
        reset_console();
}

void Console::GetInputLoop() {
    while (!mIsReset) {
        mKeypress = getch();
    }
}

void Console::initialize() {
    mIsReset = false;

    mPalette.resize(16);

	mPalette[0] = colors::rgb_to_lab((colors::rgb){12, 12, 12});
	mPalette[1] = colors::rgb_to_lab((colors::rgb){197, 15, 31});
	mPalette[2] = colors::rgb_to_lab((colors::rgb){19, 161, 14});
	mPalette[3] = colors::rgb_to_lab((colors::rgb){193, 161, 14});
	mPalette[4] = colors::rgb_to_lab((colors::rgb){0, 55, 218});
	mPalette[5] = colors::rgb_to_lab((colors::rgb){136, 23, 152});
	mPalette[6] = colors::rgb_to_lab((colors::rgb){58, 150, 221});
	mPalette[7] = colors::rgb_to_lab((colors::rgb){204, 204, 204});

	mPalette[8] = colors::rgb_to_lab((colors::rgb){118, 118, 118});
	mPalette[9] = colors::rgb_to_lab((colors::rgb){231, 72, 86});
	mPalette[10] = colors::rgb_to_lab((colors::rgb){22, 198, 12});
	mPalette[11] = colors::rgb_to_lab((colors::rgb){249, 241, 165});
	mPalette[12] = colors::rgb_to_lab((colors::rgb){59, 120, 255});
	mPalette[13] = colors::rgb_to_lab((colors::rgb){180, 0, 158});
	mPalette[14] = colors::rgb_to_lab((colors::rgb){97, 214, 214});
	mPalette[15] = colors::rgb_to_lab((colors::rgb){242, 242, 242});

    #ifdef _WIN32
	HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);

	SetConsoleMode(hout, mOldOutMode);
	SetConsoleMode(hin, OldInMode);
    #endif

    mInputThread = std::thread(&Console::GetInputLoop, this);

    initscr();
    cbreak();
    noecho();
    curs_set(0);
    timeout(1);

    reset_state();
}

void Console::draw(std::vector<colors::rgb>& buffer) {
	const char* ascii = " `.-':_,^=;><+!rc*/z?sLTv)J7(|Fi{C}fI31tlu[neoZ5Yxjya]2ESwqkP6h9d4VpOGbUAKXHm8RD#$Bg0MNWQ%&@";
    int dY = (mMode == MODE_ASCII) ? 1 : 2; // If ASCII, drawing takes up 1 full character instead of half
    colors::rgb oldTop = { 0, 0, 0 };
    colors::rgb oldBottom = { 0, 0, 0};
    
    fmt::print(CSI "0;0H");

    for (int y = 0; y < mHeight; y += dY) {
        fmt::print(CSI "{};0H", y / dY); // Set cursor to beginning of next line

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

            }break;
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

	og_in_mode = dwOgIn;
	og_out_mode = dwOgOut;

	// set console mode
	DWORD dwInMode = dwOgIn | ENABLE_VIRTUAL_TERMINAL_INPUT;
	DWORD dwOutMode = dwOgOut | (ENABLE_VIRTUAL_TERMINAL_PROCESSING);

	SetConsoleMode(hout, dwOutMode);
	SetConsoleMode(hin, dwInMode);

	// get console width and height
	width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	height = (csbi.srWindow.Bottom - csbi.srWindow.Top + 1) * 2;
    #elif defined(__unix__) || defined(__APPLE__)
	struct winsize size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);

	mWidth = size.ws_col;
	mHeight = size.ws_row;

	if (mMode != MODE_ASCII) {
		mHeight *= 2;
	}
    #endif
}

void Console::reset_console() {
    endwin();

    #ifdef _WIN32
    HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);

	SetConsoleMode(hout, window->og_out_mode);
	SetConsoleMode(hin, window->og_in_mode);
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
