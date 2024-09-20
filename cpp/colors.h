#pragma once

#include <cstdint>

#define colors_compare_rgb(x, y) (x.r == y.r && x.g == y.g && x.b == y.b)
#define colors_euclidean_lab(x, y) (powf(x.l - y.l, 2.0f) + powf(x.a - y.a, 2.0f) + powf(x.b - y.b, 2.0f))

struct colors_rgb {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct colors_xyz {
	float x;
	float y;
	float z;
};

struct colors_lab {
	float l;
	float a;
	float b;
};

void colors_rgb_to_xyz(colors_rgb rgb, colors_xyz* xyz);
void colors_xyz_to_lab(colors_xyz xyz, colors_lab* lab);
void colors_rgb_to_lab(colors_rgb rgb, colors_lab* lab);