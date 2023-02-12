#include "renderer.hpp"

Renderer::Renderer()
{
	// Set up console if on Windows
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);

	HANDLE hWnd = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD mode;
	GetConsoleMode(hWnd, &mode);
	mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
	SetConsoleMode(hWnd, mode);

#endif // _WIN32

	std::cout << CSI "?1049h"; // Set alternate screen buffer;
	std::cout << CSI "?25l"; // Hide cursor

	Width = 0;
	Height = 0;

	redraw = false;

	SetDimensions();
}

bool Renderer::SetDimensions()
{
	int col = Width;
	int row = Height / 2;

#ifndef _WIN32
	struct winsize sz;
	ioctl(fileno(stdout), TIOCGWINSZ, &sz);

	if (sz.ws_row != row || sz.ws_col != col) {
		if (framebuffer != NULL) {
			delete[] framebuffer;
			delete[] prev_frame;
		}

		col = sz.ws_col;
		row = sz.ws_row;

		Width = col;
		Height = row * 2;

		framebuffer = new conchar[Width * (Height / 2)];
		prev_frame = new conchar[Width * (Height / 2)];

		for (int i = 0; i < Width * (Height / 2); i++)
		{
			framebuffer[i].bg = { 0, 0, 0 };
			prev_frame[i].fg = { 255, 255, 255 };
			framebuffer[i].bg = { 0, 0, 0 };
			prev_frame[i].fg = { 255, 255, 255 };
		}

		redraw = true;

		return true;
	}
#else
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	col = (int)(csbi.srWindow.Right - csbi.srWindow.Left + 1);
	row = (int)(csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
#endif

	return false;
}

void Renderer::SetPixel(int x, int y, color col)
{
	int i = x + Width * (y / 2);

	if (y % 2 == 1)
	{
		framebuffer[i].bg = col;
	}
	else
	{
		framebuffer[i].fg = col;
	}
}

void Renderer::Clear(color col)
{
	for (int i = 0; i < Width * (Height / 2); i++)
	{
		framebuffer[i].bg = col;
		framebuffer[i].fg = col;
	}
}

void Renderer::Render()
{
	int x;
	int y;

	std::string str = "";

	if (redraw) {
		for (int i = 0; i < Width * (Height / 2); i++) {
			x = i % Width;
			y = i / Width;
			str += CSI + std::to_string(y) + ";" + std::to_string(x) + "H";
			str += CSI "38;2;" + std::to_string(framebuffer[i].fg.r) + ";" + std::to_string(framebuffer[i].fg.g) + ";" + std::to_string(framebuffer[i].fg.b) + "m" + CSI "48;2;" + std::to_string(framebuffer[i].bg.r) + ";" + std::to_string(framebuffer[i].bg.g) + ";" + std::to_string(framebuffer[i].bg.b) + "m" + "\u2580";
		}
	}
	else
		for (int i = 0; i < Width * (Height / 2); i++) {
			if (std::memcmp(&prev_frame[i], &framebuffer[i], sizeof(conchar)) == 0)
				continue;
			else {
				x = i % Width;
				y = i / Width;
				str += CSI + std::to_string(y) + ";" + std::to_string(x) + "H";
			}

			str += CSI "38;2;" + std::to_string(framebuffer[i].fg.r) + ";" + std::to_string(framebuffer[i].fg.g) + ";" + std::to_string(framebuffer[i].fg.b) + "m" + CSI "48;2;" + std::to_string(framebuffer[i].bg.r) + ";" + std::to_string(framebuffer[i].bg.g) + ";" + std::to_string(framebuffer[i].bg.b) + "m" + "\u2580";
		}

	std::cout << str;

	std::memcpy(prev_frame, framebuffer, (Width * (Height / 2) * sizeof(conchar)));
}

void Renderer::Clean()
{
	delete[] framebuffer;
	delete[] prev_frame;

	std::cout << "\x1b[?1049l"; // Set main screen buffer;
	std::cout << "\x1b[?25h"; // Show cursor
	std::cout << ESC "!p"; // Reset to default in case
}
