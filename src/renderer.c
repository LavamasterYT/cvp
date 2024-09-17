#include "renderer.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
#include <Windows.h>
#define RENDERER_PIXEL_CHAR "\xDF"
#elif defined(__unix__) || defined(__APPLE__)
#include <sys/ioctl.h>
#include <unistd.h>
#define RENDERER_PIXEL_CHAR "▀"
#endif


void renderer_palette_init(renderer_term_window* window)
{
	// this palette is based off of the windows terminal palette
	// order of colors: black, red, green, yellow, blue, magenta, cyan, and white
	// the rest of the 8 colors are the same but the bright versions of colors
	// TODO: custom palette?

	window->palette[0].r = 12; window->palette[0].g = 12; window->palette[0].b = 12;
	window->palette[1].r = 197; window->palette[1].g = 15; window->palette[1].b = 31;
	window->palette[2].r = 19; window->palette[2].g = 161; window->palette[2].b = 14;
	window->palette[3].r = 193; window->palette[3].g = 156; window->palette[3].b = 0;
	window->palette[4].r = 0; window->palette[4].g = 55; window->palette[4].b = 218;
	window->palette[5].r = 136; window->palette[5].g = 23; window->palette[5].b = 152;
	window->palette[6].r = 58; window->palette[6].g = 150; window->palette[6].b = 221;
	window->palette[7].r = 204; window->palette[7].g = 204; window->palette[7].b = 204;

	window->palette[8].r = 118; window->palette[8].g = 118; window->palette[8].b = 118;
	window->palette[9].r = 231; window->palette[9].g = 72; window->palette[9].b = 86;
	window->palette[10].r = 22; window->palette[10].g = 198; window->palette[10].b = 12;
	window->palette[11].r = 249; window->palette[11].g = 241; window->palette[11].b = 165;
	window->palette[12].r = 59; window->palette[12].g = 120; window->palette[12].b = 255;
	window->palette[13].r = 180; window->palette[13].g = 0; window->palette[13].b = 158;
	window->palette[14].r = 97; window->palette[14].g = 214; window->palette[14].b = 214;
	window->palette[15].r = 242; window->palette[15].g = 242; window->palette[15].b = 242;
}

void renderer_clear_256(renderer_term_window* window)
{
	// clear terminal in rgb mode
	printf(CSI "38;2;0;0;0m" CSI "48;2;0;0;0m");
	for (int y = 0; y < window->height; y += 2)
		for (int x = 0; x < window->width; x++)
			printf(RENDERER_PIXEL_CHAR);
}

void renderer_clear_palette(renderer_term_window* window)
{
	// clear terminal with terminal colors
	printf(CSI "30m" CSI "40m");
	for (int y = 0; y < window->height; y += 2)
		for (int x = 0; x < window->width; x++)
			printf(RENDERER_PIXEL_CHAR);
}

void renderer_set_scale(renderer_term_window* window, int width, int height, renderer_rgb* src, renderer_rgb* src_cp, renderer_rgb** dst, renderer_rgb** dst_cp)
{
	(*dst) = (renderer_rgb*)malloc(sizeof(renderer_rgb) * window->width * window->height);
	if (dst_cp != NULL)
		(*dst_cp) = (renderer_rgb*)malloc(sizeof(renderer_rgb) * window->width * window->height);

	memset((*dst), 0, sizeof(renderer_rgb) * window->width * window->height);
	if (dst_cp != NULL)
		memset((*dst_cp), 0, sizeof(renderer_rgb) * window->width * window->height);

	if (width == window->width)
	{
		int offset = (window->height - height) / 2;

		for (int y = offset; y < height + offset; y++)
		{
			for (int x = 0; x < window->width; x++)
			{
				(*dst)[x + window->width * y] = src[x + width * (y - offset)];
				if (dst_cp != NULL)
					(*dst_cp)[x + window->width * y] = src_cp[x + width * (y - offset)];
			}
		}
	}
	else if (height == window->height)
	{
		int offset = (window->width - width) / 2;

		for (int y = 0; y < window->height; y++)
		{
			for (int x = offset; x < width + offset; x++)
			{
				(*dst)[x + window->width * y] = src[(x - offset) + width * y];
				if (dst_cp != NULL)
					(*dst_cp)[x + window->width * y] = src_cp[(x - offset) + width * y];
			}
		}
	}
}

void renderer_draw_256(renderer_term_window* window, renderer_rgb* buffer, int width, int height)
{
	size_t max_str_size = window->width * window->height * 20;
	char* str_buf = (char*)malloc(max_str_size);
	renderer_rgb* fixed_buffer = NULL;

	renderer_set_scale(window, width, height, buffer, NULL, &fixed_buffer, NULL);

	size_t pos = snprintf(str_buf, max_str_size, CSI "0;0H" CSI "48;2;0;0;0m" CSI "38;2;0;0;0m");

	renderer_rgb og_top;
	renderer_rgb og_bottom;

	memset(&og_top, 0, sizeof(renderer_rgb));
	memset(&og_bottom, 0, sizeof(renderer_rgb));
	
	// Loop through the buffer and add correct ansi sequence
	for (int y = 0; y < window->height; y += 2)
	{
		for (int x = 0; x < window->width; x++)
		{
			renderer_rgb top = fixed_buffer[x + window->width * y];
			renderer_rgb bottom = fixed_buffer[x + window->width * (y + 1)];

			if (renderer_compare(top, og_top) && renderer_compare(bottom, og_bottom))
				pos += snprintf(str_buf + pos, max_str_size - pos, RENDERER_PIXEL_CHAR);
			else if (renderer_compare(top, og_top))
				pos += snprintf(str_buf + pos, max_str_size - pos,
					CSI "48;2;%d;%d;%dm" RENDERER_PIXEL_CHAR,
					bottom.r, bottom.g, bottom.b);
			else if (renderer_compare(bottom, og_bottom))
				pos += snprintf(str_buf + pos, max_str_size - pos,
					CSI "38;2;%d;%d;%dm" RENDERER_PIXEL_CHAR,
					top.r, top.g, top.b);
			else
				pos += snprintf(str_buf + pos, max_str_size - pos,
					CSI "38;2;%d;%d;%dm" CSI "48;2;%d;%d;%dm" RENDERER_PIXEL_CHAR,
					top.r, top.g, top.b, bottom.r, bottom.g, bottom.b);

			memcpy(&og_top, &top, sizeof(renderer_rgb));
			memcpy(&og_bottom, &bottom, sizeof(renderer_rgb));
		}
	}

	puts(str_buf);
	free(str_buf);
	free(fixed_buffer);
}

void renderer_draw_kitty(renderer_term_window* window, renderer_rgb* buffer, renderer_rgb* last_frame, int width, int height)
{
	size_t max_str_size = window->width * window->height * 20;
	char* str_buf = (char*)malloc(max_str_size);
	renderer_rgb* fixed_buffer = NULL;
	renderer_rgb* fixed_last_buffer = NULL;

	renderer_set_scale(window, width, height, buffer, last_frame, &fixed_buffer, &fixed_last_buffer);

	size_t pos = snprintf(str_buf, max_str_size, CSI "0;0H" CSI "48;2;0;0;0m" CSI "38;2;0;0;0m");

	renderer_rgb og_top;
	renderer_rgb og_bottom;

	memset(&og_top, 0, sizeof(renderer_rgb));
	memset(&og_bottom, 0, sizeof(renderer_rgb));

	// Loop through the buffer and add correct ansi sequence
	for (int y = 0; y < window->height; y += 2)
	{
		pos += snprintf(str_buf + pos, max_str_size - pos, CSI "%d;0H", y / 2);
		if (pos >= max_str_size)
		{
			puts(str_buf);
			pos = 0;
		}

		for (int x = 0; x < window->width; x++)
		{
			renderer_rgb top = fixed_buffer[x + window->width * y];
			renderer_rgb bottom = fixed_buffer[x + window->width * (y + 1)];

			// Skip unchanged pixels
            if (renderer_compare(top, fixed_last_buffer[x + window->width * y]) &&
                renderer_compare(bottom, fixed_last_buffer[x + window->width * (y + 1)]))
            {
                pos += snprintf(str_buf + pos, max_str_size - pos, ESC "[C");
            }
			else
			{
				// If top and bottom haven't changed, just print the pixel
				if (renderer_compare(top, og_top) && renderer_compare(bottom, og_bottom))
					pos += snprintf(str_buf + pos, max_str_size - pos, RENDERER_PIXEL_CHAR);
				else if (renderer_compare(top, og_top))
					pos += snprintf(str_buf + pos, max_str_size - pos,
									CSI "48;2;%d;%d;%dm" RENDERER_PIXEL_CHAR,
									bottom.r, bottom.g, bottom.b);
				else if (renderer_compare(bottom, og_bottom))
					pos += snprintf(str_buf + pos, max_str_size - pos,
									CSI "38;2;%d;%d;%dm" RENDERER_PIXEL_CHAR,
									top.r, top.g, top.b);
				else
					pos += snprintf(str_buf + pos, max_str_size - pos,
									CSI "38;2;%d;%d;%dm" CSI "48;2;%d;%d;%dm" RENDERER_PIXEL_CHAR,
									top.r, top.g, top.b, bottom.r, bottom.g, bottom.b);

				memcpy(&og_top, &top, sizeof(renderer_rgb));
				memcpy(&og_bottom, &bottom, sizeof(renderer_rgb));
			}

			// Check if buffer is full and reset
			if (pos >= max_str_size)
			{
				puts(str_buf);
				pos = 0;
			}
		}
	}

	memcpy(last_frame, buffer, window->width * window->height * sizeof(renderer_rgb));

    puts(str_buf);
    free(str_buf);
	free(fixed_buffer);
	free(fixed_last_buffer);
}


void renderer_draw_palette(renderer_term_window* window, renderer_rgb* buffer, int width, int height)
{
	size_t max_str_size = window->width * window->height * 25;
	char* str_buf = (char*)malloc(max_str_size);

	renderer_rgb* fixed_buffer = NULL;
	renderer_set_scale(window, width, height, buffer, NULL, &fixed_buffer, NULL);

	if (str_buf == NULL)
		return;

	size_t pos = snprintf(str_buf, max_str_size, CSI "0;0H");

	int top = 30;
	int bottom = 40;
	int og_top = -1;
	int og_bottom = -1;

	// Loop through the buffer and add correct ansi sequence
	for (int y = 0; y < window->height; y += 2)
	{
		pos += snprintf(str_buf + pos, max_str_size - pos, CSI "%d;0H", y / 2);
		if (pos >= max_str_size)
		{
			puts(str_buf);
			pos = 0;
		}

		for (int x = 0; x < window->width; x++)
		{
			int dist = 10000000;
			for (int i = 0; i < 16; i++)
			{
				int c = renderer_rgb_distance(fixed_buffer[x + window->width * y], window->palette[i]);

				if (dist > c)
				{
					dist = c;
					top = i <= 7 ? 30 + i : 90 + i % 8;
				}
			}

			dist = 10000000;
			for (int i = 0; i < 16; i++)
			{
				int c = renderer_rgb_distance(fixed_buffer[x + window->width * (y + 1)], window->palette[i]);

				if (dist > c)
				{
					dist = c;
					bottom = i <= 7 ? 40 + i : 100 + i % 8;
				}
			}

			if (top == og_top && bottom == og_bottom)
				pos += snprintf(str_buf + pos, max_str_size - pos, RENDERER_PIXEL_CHAR);
			else if (top == og_top)
				pos += snprintf(str_buf + pos, max_str_size - pos,
					CSI "%dm" RENDERER_PIXEL_CHAR, bottom);
			else if (bottom == og_bottom)
				pos += snprintf(str_buf + pos, max_str_size - pos,
					CSI "%dm" RENDERER_PIXEL_CHAR, top);
			else
				pos += snprintf(str_buf + pos, max_str_size - pos,
					CSI "%dm" CSI "%dm" RENDERER_PIXEL_CHAR,
					top, bottom);

			// Check if buffer is full and reset
			if (pos >= max_str_size)
			{
				puts(str_buf);
				pos = 0;
			}

			og_top = top;
			og_bottom = bottom;
		}
	}

	puts(str_buf);
	free(str_buf);
	free(fixed_buffer);
}

void renderer_draw_ascii(renderer_term_window* window, renderer_rgb* buffer, int width, int height)
{
	char* ascii = " `.-':_,^=;><+!rc*/z?sLTv)J7(|Fi{C}fI31tlu[neoZ5Yxjya]2ESwqkP6h9d4VpOGbUAKXHm8RD#$Bg0MNWQ%&@";
	

	size_t max_str_size = window->width * window->height * 25;
	char* str_buf = (char*)malloc(max_str_size);

	renderer_rgb* fixed_buffer = NULL;
	renderer_set_scale(window, width, height, buffer, NULL, &fixed_buffer, NULL);

	if (str_buf == NULL)
		return;

	size_t pos = snprintf(str_buf, max_str_size, CSI "0;0H");

	// Loop through the buffer and add correct ansi sequence
	for (int y = 0; y < window->height; y += 2)
	{
		pos += snprintf(str_buf + pos, max_str_size - pos, CSI "%d;0H", y / 2);
		if (pos >= max_str_size)
		{
			puts(str_buf);
			pos = 0;
		}

		for (int x = 0; x < window->width; x++)
		{
            float gs = (float)fixed_buffer[x + window->width * y].r;
            int index = (int)(gs / 255.0f * 92.0f);
            char c = ascii[index];
			pos += snprintf(str_buf + pos, max_str_size - pos, "%c", c);

			// Check if buffer is full and reset
			if (pos >= max_str_size)
			{
				puts(str_buf);
				pos = 0;
			}
		}
	}

	puts(str_buf);
	free(str_buf);
	free(fixed_buffer);
}

renderer_term_window* renderer_init(int mode)
{
	renderer_term_window* window = (renderer_term_window*)malloc(sizeof(renderer_term_window));

	if (window == NULL)
		return NULL;

	// we need to set the console modes and get the console dimensions
	// this is platform specific

#ifdef _WIN32
	// get handles
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD dwOgOut = 0;
	DWORD dwOgIn = 0;

	// get current console information
	GetConsoleMode(hout, &dwOgOut);
	GetConsoleMode(hin, &dwOgIn);
	GetConsoleScreenBufferInfo(hout, &csbi);

	window->og_in_mode = dwOgIn;
	window->og_out_mode = dwOgOut;

	// set console mode
	DWORD dwInMode = dwOgIn | ENABLE_VIRTUAL_TERMINAL_INPUT;
	DWORD dwOutMode = dwOgOut | (ENABLE_VIRTUAL_TERMINAL_PROCESSING);

	SetConsoleMode(hout, dwOutMode);
	SetConsoleMode(hin, dwInMode);

	// get console width and height
	window->width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	window->height = (csbi.srWindow.Bottom - csbi.srWindow.Top + 1) * 2;
#elif defined(__unix__) || defined(__APPLE__)
	struct winsize size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);

	window->width = size.ws_col;
	window->height = size.ws_row * 2;

#else
	return NULL;
#endif

	// setup terminal by switching to its alternate buffer and hiding the cursor
	uint8_t buffer[] = CSI "?1049h" CSI "?25l";
	fwrite(buffer, sizeof(uint8_t), sizeof(buffer), stdout);

	window->mode = mode;

	// clear terminal with black
	if (window->mode == RENDERER_FULL_COLOR)
	{
		renderer_clear_256(window);
	}
	else
	{
		renderer_palette_init(window);
		renderer_clear_palette(window);
	}

	return window;
}

void renderer_draw(renderer_term_window* window, renderer_rgb* buffer, renderer_rgb* last_frame, int width, int height)
{
	switch (window->mode)
	{
		case RENDERER_FULL_COLOR:
			renderer_draw_256(window, buffer, width, height);
			break;
		case RENDERER_KITTY:
			renderer_draw_kitty(window, buffer, last_frame, width, height);
			break;
		case RENDERER_ASCII:
			renderer_draw_ascii(window, buffer, width, height);
			break;
		case RENDERER_PALETTE:
		default:
			renderer_draw_palette(window, buffer, width, height);
			break;
	}
}

void renderer_destroy(renderer_term_window* window)
{
	uint8_t buffer[] = CSI "?1049l" CSI "?25h";
	fwrite(buffer, sizeof(uint8_t), sizeof(buffer), stdout);

#ifdef _WIN32
	HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);

	SetConsoleMode(hout, window->og_out_mode);
	SetConsoleMode(hin, window->og_in_mode);
#endif

	free(window);
}