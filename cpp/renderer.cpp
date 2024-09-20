#include "renderer.h"

#include <iostream>
#include <string>
#include <vector>

#include <fmt/core.h>

#ifdef _WIN32
#include <Windows.h>
#define RENDERER_PIXEL_CHAR "\xDF"
#elif defined(__unix__) || defined(__APPLE__)
#include <sys/ioctl.h>
#include <unistd.h>
#define RENDERER_PIXEL_CHAR "▀"
#endif

#include "colors.h"

renderer::renderer()
{
	width = 0;
	height = 0;
	mode = RENDERER_MODE_ASCII;
	ascii = " `.-':_,^=;><+!rc*/z?sLTv)J7(|Fi{C}fI31tlu[neoZ5Yxjya]2ESwqkP6h9d4VpOGbUAKXHm8RD#$Bg0MNWQ%&@";

	set_dimensions();
	fmt::print(CSI "?1049h" CSI "?25l");
	clear();
	intialize_palette();
}

renderer::~renderer()
{
	fmt::print(CSI "?1049l" CSI "?25h");

#ifdef _WIN32
	HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);

	SetConsoleMode(hout, window->og_out_mode);
	SetConsoleMode(hin, window->og_in_mode);
#endif
}

void renderer::intialize_palette()
{
	palette.resize(16);

	colors_rgb_to_lab((colors_rgb){12, 12, 12},    &palette[0]);
	colors_rgb_to_lab((colors_rgb){197, 15, 31},   &palette[1]);
	colors_rgb_to_lab((colors_rgb){19, 161, 14},   &palette[2]);
	colors_rgb_to_lab((colors_rgb){193, 161, 14},  &palette[3]);
	colors_rgb_to_lab((colors_rgb){0, 55, 218},    &palette[4]);
	colors_rgb_to_lab((colors_rgb){136, 23, 152},  &palette[5]);
	colors_rgb_to_lab((colors_rgb){58, 150, 221},  &palette[6]);
	colors_rgb_to_lab((colors_rgb){204, 204, 204}, &palette[7]);

	colors_rgb_to_lab((colors_rgb){118, 118, 118}, &palette[8]);
	colors_rgb_to_lab((colors_rgb){231, 72, 86},   &palette[9]);
	colors_rgb_to_lab((colors_rgb){22, 198, 12},   &palette[10]);
	colors_rgb_to_lab((colors_rgb){249, 241, 165}, &palette[11]);
	colors_rgb_to_lab((colors_rgb){59, 120, 255},  &palette[12]);
	colors_rgb_to_lab((colors_rgb){180, 0, 158},   &palette[13]);
	colors_rgb_to_lab((colors_rgb){97, 214, 214},  &palette[14]);
	colors_rgb_to_lab((colors_rgb){242, 242, 242}, &palette[15]);
}

void renderer::set_dimensions()
{
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

	width = size.ws_col;
	height = size.ws_row * 2;
#endif
}

void renderer::clear()
{
	// clear terminal with terminal colors

	fmt::print(CSI "30m" CSI "40m");
	for (int y = 0; y < height; y += 2)
		for (int x = 0; x < width; x++)
			fmt::print("▀");
}

void renderer::scale_buffer(std::vector<colors_rgb>& src, std::vector<colors_rgb>& dst, int slc_width, int slc_height)
{
	dst.resize(width * height);

	if (slc_width == width)
	{
		int offset = (height - slc_height) / 2;

		for (int y = offset; y < slc_height + offset; y++)
			for (int x = 0; x < width; x++)
				dst[x + width * y] = src[x + slc_width * (y - offset)];
	}
	else if (slc_height == height)
	{
		int offset = (width - slc_width) / 2;

		for (int y = 0; y < height; y++)
			for (int x = offset; x < slc_width + offset; x++)
				dst[x + width * y] = src[(x - offset) + slc_width * y];
	}
}

void renderer::draw(std::vector<colors_rgb>& buffer, int crop_width, int crop_height)
{
	std::vector<colors_rgb> full_buffer;
	scale_buffer(buffer, full_buffer, crop_width, crop_height);

	fmt::print(CSI "0;0H");

	switch (mode)
	{
		case RENDERER_MODE_FULL_COLOR:
			fmt::print(CSI "48;2;0;0;0m" CSI "38;2;0;0;0m");
			break;
		case RENDERER_MODE_PALETTE:
			fmt::print(CSI "30m" CSI "40m");
			break;
		case RENDERER_MODE_ASCII:
			fmt::print(CSI "37m" CSI "40m");
			break;
	}

	colors_rgb old_top;
	colors_rgb old_bottom;

	old_top.r = 0;
	old_top.g = 0;
	old_top.b = 0;
	old_bottom.r = 0;
	old_bottom.g = 0;
	old_bottom.b = 0;

	for (int y = 0; y < height; y += 2)
	{
		fmt::print(CSI "{};0H", y / 2);

		for (int x = 0; x < width; x++)
		{
			switch (mode)
			{
				case RENDERER_MODE_ASCII: {
						float gs = (float)full_buffer[x + width * y].r;
						int index = (int)(gs / 255.0f * ascii.length());
						char c = ascii[index];
						fmt::print("{}", c);
					} break;
				case RENDERER_MODE_FULL_COLOR: {
						colors_rgb top = full_buffer[x + width * y];
						colors_rgb bottom = full_buffer[x + width * (y + 1)];

						if (colors_compare_rgb(top, old_top) && colors_compare_rgb(bottom, old_bottom))
							fmt::print("▀");
						else if (colors_compare_rgb(top, old_top))
							fmt::print(CSI "48;2;{};{};{}m▀", bottom.r, bottom.g, bottom.b);
						else if (colors_compare_rgb(bottom, old_bottom))
							fmt::print(CSI "38;2;{};{};{}m▀", top.r, top.g, top.b);
						else
							fmt::print(CSI "38;2;{};{};{}m" CSI "48;2;{};{};{}m▀", top.r, top.g, top.b, bottom.r, bottom.g, bottom.b);

						old_top = top;
						old_bottom = bottom;
					} break;
				case RENDERER_MODE_PALETTE: {
						colors_lab current;
						int top, bottom;

						int distance = 1000000;
						colors_rgb_to_lab(full_buffer[x + width * y], &current);

						for (int i = 0; i < palette.size(); i++)
						{
							int c = colors_euclidean_lab(current, palette[i]);

							if (distance > c)
							{
								distance = c;
								top = i <= 7 ? 30 + i : 90 + i % 8;
							}
						}

						distance = 10000000;
						colors_rgb_to_lab(full_buffer[x + width * (y + 1)], &current);

						for (int i = 0; i < palette.size(); i++)
						{
							int c = colors_euclidean_lab(current, palette[i]);

							if (distance > c)
							{
								distance = c;
								bottom = i <= 7 ? 40 + i : 100 + i % 8;
							}
						}

						if (top == old_top.r && bottom == old_bottom.r)
							fmt::print("▀");
						else if (top == old_top.r)
							fmt::print(CSI "{}m▀", bottom);
						else if (bottom == old_bottom.r)
							fmt::print(CSI "{}m▀", top);
						else
							fmt::print(CSI "{}m" CSI "{}m▀", top, bottom);

						old_top.r = top;
						old_bottom.r = bottom;
					} break;
			}
		}
	}
}
