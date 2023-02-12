#include <iostream>

#ifndef _WIN32
#include <sys/ioctl.h>
#endif
#ifdef _WIN32
#include <Windows.h>
#endif

#define ESC "\x1b"
#define CSI "\x1b["

typedef struct
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
} color;

typedef struct
{
	color bg;
	color fg;
} conchar;

class Renderer
{
private:
	conchar* framebuffer;
	conchar* prev_frame;

	bool redraw;

public:
	uint16_t Width;
	uint16_t Height;

	Renderer();

	bool SetDimensions();
	void SetPixel(int x, int y, color col);
	void Clear(color col);
	void Render();
	void Clean();
};
