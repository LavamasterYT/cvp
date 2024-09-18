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

float renderer_lab_fun(float t)
{
	if (t > powf(6.0f / 29.0f, 3))
	{
		return cbrtf(t);
	}
	else
	{
		return ((1.0f / 3.0f) * t * powf((6.0f / 29.0f), -2.0f) + (4.0f / 29.0f));
	}
}

void renderer_rgb_to_lab(renderer_rgb rgb, renderer_lab* lab)
{
	renderer_xyz n;
	renderer_xyz c;

	float r, g, b;
	r = (rgb.r / 255.0f) <= 0.04045f ? (rgb.r / 255.0f) / 12.92f : powf(((rgb.r / 255.0f) + 0.055f) / 1.055f, 2.4f);
	g = (rgb.g / 255.0f) <= 0.04045f ? (rgb.g / 255.0f) / 12.92f : powf(((rgb.g / 255.0f) + 0.055f) / 1.055f, 2.4f);
	b = (rgb.b / 255.0f) <= 0.04045f ? (rgb.b / 255.0f) / 12.92f : powf(((rgb.b / 255.0f) + 0.055f) / 1.055f, 2.4f);

	c.x = (0.4124564 * r) + (0.3575761 * g) + (0.1804375 * b);
    c.y = (0.2126729 * r) + (0.7151522 * g) + (0.0721750 * b);
    c.z = (0.0193339 * r) + (0.1191920 * g) + (0.9503041 * b);

	n.x = 95.0489f;
	n.y = 100.0f;
	n.z = 108.8840f;

	float xx = renderer_lab_fun(c.x / n.x);
	float yy = renderer_lab_fun(c.y / n.y);
	float zz = renderer_lab_fun(c.z / n.z);

	lab->l = 116.0f * yy - 16.0f;
	lab->a = 500.0f * (xx - yy);
	lab->b = 200 * (yy - zz);
}

void renderer_palette_init(renderer_term_window* window)
{
	// this palette is based off of the windows terminal palette
	// order of colors: black, red, green, yellow, blue, magenta, cyan, and white
	// the rest of the 8 colors are the same but the bright versions of colors
	// TODO: custom palette?

	renderer_rgb_to_lab((renderer_rgb){12, 12, 12},    &window->palette[0]);
	renderer_rgb_to_lab((renderer_rgb){197, 15, 31},   &window->palette[1]);
	renderer_rgb_to_lab((renderer_rgb){19, 161, 14},   &window->palette[2]);
	renderer_rgb_to_lab((renderer_rgb){193, 161, 14},  &window->palette[3]);
	renderer_rgb_to_lab((renderer_rgb){0, 55, 218},    &window->palette[4]);
	renderer_rgb_to_lab((renderer_rgb){136, 23, 152},  &window->palette[5]);
	renderer_rgb_to_lab((renderer_rgb){58, 150, 221},  &window->palette[6]);
	renderer_rgb_to_lab((renderer_rgb){204, 204, 204}, &window->palette[7]);

	renderer_rgb_to_lab((renderer_rgb){118, 118, 118}, &window->palette[8]);
	renderer_rgb_to_lab((renderer_rgb){231, 72, 86},   &window->palette[9]);
	renderer_rgb_to_lab((renderer_rgb){22, 198, 12},   &window->palette[10]);
	renderer_rgb_to_lab((renderer_rgb){249, 241, 165}, &window->palette[11]);
	renderer_rgb_to_lab((renderer_rgb){59, 120, 255},  &window->palette[12]);
	renderer_rgb_to_lab((renderer_rgb){180, 0, 158},   &window->palette[13]);
	renderer_rgb_to_lab((renderer_rgb){97, 214, 214},  &window->palette[14]);
	renderer_rgb_to_lab((renderer_rgb){242, 242, 242}, &window->palette[15]);
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

void renderer_draw_exp(renderer_term_window* window, renderer_rgb* buffer, renderer_rgb* last_frame, int width, int height)
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

			renderer_lab current;

			renderer_rgb_to_lab(fixed_buffer[x + window->width * y], &current);

			for (int i = 0; i < 16; i++)
			{
				int c = (powf(current.l - window->palette[i].l, 2.0f) + powf(current.a - window->palette[i].a, 2.0f) + powf(current.b - window->palette[i].b, 2.0f));

				if (dist > c)
				{
					dist = c;
					top = i <= 7 ? 30 + i : 90 + i % 8;
				}
			}

			dist = 10000000;
			renderer_rgb_to_lab(fixed_buffer[x + window->width * (y + 1)], &current);

			for (int i = 0; i < 16; i++)
			{
				int c = (powf(current.l - window->palette[i].l, 2.0f) + powf(current.a - window->palette[i].a, 2.0f) + powf(current.b - window->palette[i].b, 2.0f));

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
	//char* ascii = " .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";

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
            int index = (int)(gs / 255.0f * strlen(ascii));
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
		case RENDERER_EXPERIMENTAL:
			renderer_draw_exp(window, buffer, last_frame, width, height);
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