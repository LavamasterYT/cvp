#pragma once

#include <cstdint>

typedef struct colors_rgb {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} colors_rgb;

typedef struct colors_xyz {
	float x;
	float y;
	float z;
} colors_xyz;

typedef struct colors_lab {
	float l;
	float a;
	float b;
} colors_lab;
