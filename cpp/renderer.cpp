#include "renderer.h"

renderer::renderer()
{
	width = 0;
	height = 0;
	mode = RENDERER_MODE_ASCII;

#ifdef _WIN32
	og_in_mode = 0;
	og_out_mode = 0;
#endif
}

renderer::~renderer()
{

}

void renderer::draw(std::vector<colors_rgb>& buffer, int crop_width, int crop_height)
{

}

void renderer::draw_ascii(std::vector<colors_rgb>& buffer, int crop_width, int crop_height)
{

}

void renderer::draw_palette(std::vector<colors_rgb>& buffer, int crop_width, int crop_height)
{

}

void renderer::draw_full(std::vector<colors_rgb>& buffer, int crop_width, int crop_height)
{

}