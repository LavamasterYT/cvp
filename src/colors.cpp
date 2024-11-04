#include "colors.h"

#include <cmath>

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

void colors_rgb_to_xyz(colors_rgb rgb, colors_xyz* xyz)
{
    float r, g, b;
	r = (rgb.r / 255.0f) <= 0.04045f ? (rgb.r / 255.0f) / 12.92f : powf(((rgb.r / 255.0f) + 0.055f) / 1.055f, 2.4f);
	g = (rgb.g / 255.0f) <= 0.04045f ? (rgb.g / 255.0f) / 12.92f : powf(((rgb.g / 255.0f) + 0.055f) / 1.055f, 2.4f);
	b = (rgb.b / 255.0f) <= 0.04045f ? (rgb.b / 255.0f) / 12.92f : powf(((rgb.b / 255.0f) + 0.055f) / 1.055f, 2.4f);

	xyz->x = (0.4124564 * r) + (0.3575761 * g) + (0.1804375 * b);
    xyz->y = (0.2126729 * r) + (0.7151522 * g) + (0.0721750 * b);
    xyz->z = (0.0193339 * r) + (0.1191920 * g) + (0.9503041 * b);
}

void colors_xyz_to_lab(colors_xyz xyz, colors_lab* lab)
{
    colors_xyz n;
    n.x = 95.0489f;
	n.y = 100.0f;
	n.z = 108.8840f;

    float xx = renderer_lab_fun(xyz.x / n.x);
	float yy = renderer_lab_fun(xyz.y / n.y);
	float zz = renderer_lab_fun(xyz.z / n.z);

	lab->l = 116.0f * yy - 16.0f;
	lab->a = 500.0f * (xx - yy);
	lab->b = 200 * (yy - zz);
}

void colors_rgb_to_lab(colors_rgb rgb, colors_lab* lab)
{
    colors_xyz m;
    colors_rgb_to_xyz(rgb, &m);
    colors_xyz_to_lab(m, lab);
}
