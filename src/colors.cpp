#include "colors.h"

#include <cmath>

inline float lab_function(float t)
{
	if (t > 0.00885645167904f)
		return cbrtf(t);
	else
		return (7.78703703704f * t + 0.137931034483f);
}

colors::xyz colors::rgb_to_xyz(const colors::rgb& c_rgb) {
	colors::xyz c_xyz;
	float r, g, b;

	r = (c_rgb.r / 255.0f) <= 0.04045f ? (c_rgb.r / 255.0f) / 12.92f : powf(((c_rgb.r / 255.0f) + 0.055f) / 1.055f, 2.4f);
	g = (c_rgb.g / 255.0f) <= 0.04045f ? (c_rgb.g / 255.0f) / 12.92f : powf(((c_rgb.g / 255.0f) + 0.055f) / 1.055f, 2.4f);
	b = (c_rgb.b / 255.0f) <= 0.04045f ? (c_rgb.b / 255.0f) / 12.92f : powf(((c_rgb.b / 255.0f) + 0.055f) / 1.055f, 2.4f);

	c_xyz.x = (0.4124564 * r) + (0.3575761 * g) + (0.1804375 * b);
    c_xyz.y = (0.2126729 * r) + (0.7151522 * g) + (0.0721750 * b);
    c_xyz.z = (0.0193339 * r) + (0.1191920 * g) + (0.9503041 * b);

	return c_xyz;
}

colors::lab colors::xyz_to_lab(const colors::xyz& c_xyz) {
	colors::lab c_lab;
	const colors::xyz n = { 95.0489f, 100.0f, 108.8840f };

    float xx = lab_function(c_xyz.x / n.x);
	float yy = lab_function(c_xyz.y / n.y);
	float zz = lab_function(c_xyz.z / n.z);

	c_lab.l = 116.0f * yy - 16.0f;
	c_lab.a = 500.0f * (xx - yy);
	c_lab.b = 200 * (yy - zz);

	return c_lab;
}

colors::lab colors::rgb_to_lab(const colors::rgb& r) {
	colors::xyz m = colors::rgb_to_xyz(r);
    return colors::xyz_to_lab(m);
}
