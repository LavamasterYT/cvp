#pragma once

#include <string>
#include <vector>

#include "colors.h"

#define ESC "\x1B"
#define CSI "\x1B["

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

	renderer();
	~renderer();

	void draw(std::vector<colors_rgb>& buffer, int crop_width, int crop_height);
	void clear();
	void set_dimensions();

private:
#ifdef _WIN32
	long og_in_mode;
	long og_out_mode;
#endif
	std::string ascii;
	std::vector<colors_lab> palette;

	void intialize_palette();
	void scale_buffer(std::vector<colors_rgb>& src, std::vector<colors_rgb>& dst, int slc_width, int slc_height);
};

