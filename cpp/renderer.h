#pragma once

#include <vector>

#include "colors.h"

enum renderer_mode
{
	RENDERER_MODE_ASCII,
	RENDERER_MODE_PALETTE,
	RENDERER_MODE_FULL_COLOR,
};

class renderer
{
public:
	int width;
	int height;
	int mode;
	std::vector<colors_lab> palette;

	renderer();
	~renderer();
	void draw(std::vector<colors_rgb>& buffer, int crop_width, int crop_height);

private:
#ifdef _WIN32
	long og_in_mode;
	long og_out_mode;
#endif

	void draw_full(std::vector<colors_rgb>& buffer, int crop_width, int crop_height);
	void draw_ascii(std::vector<colors_rgb>& buffer, int crop_width, int crop_height);
	void draw_palette(std::vector<colors_rgb>& buffer, int crop_width, int crop_height);
};

