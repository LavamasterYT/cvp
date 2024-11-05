#pragma once

#include <cstdint>
#include <cmath>

namespace colors {

	/*
	* A struct representing three 8-bit RGB values.
	*/
	struct rgb {
		uint8_t r;
		uint8_t g;
		uint8_t b;
	};

	/*
	* A struct representing color in the XYZ color space.
	*/
	struct xyz {
		float x;
		float y;
		float z;
	};

	/*
	* A struct representing color in the CIELAB color space.
	*/
	struct lab {
		float l;
		float a;
		float b;
	};

	/*
	* Compares two RGB structs.
	* @param x RGB struct
	* @param y Another RGB to compare to
	* @returns Result of the comparison.
	*/
	inline bool compare_rgb(const rgb& x, const rgb& y) {
		return x.r == y.r && x.g == y.g && x.b == y.b;
	}

	/*
	* Performs the euclidean algorithm on two CIELAB colors.
	* @param x First CIELAB color
	* @param y Second CIELAB color
	* @returns Distance of the two colors, not square-rooted.
	*/
	inline float euclidean_lab(const lab& x, const lab& y) {
   		return std::pow(x.l - y.l, 2.0f) + std::pow(x.a - y.a, 2.0f) + std::pow(x.b - y.b, 2.0f);
	}

	/*
	* Converts an RGB color to a value between 0-255 representing its grayscale
	* value.
	* @param c_rgb Color to convert.
	* @returns An 8-bit number representing the grayscale value.
	*/
	inline uint8_t rgb_to_grayscale(const rgb& c_rgb);

	/*
	* Converts an RGB color to the XYZ color space.
	* @param c_rgb RGB color to convert.
	* @returns The color in XYZ color space.
	*/
	xyz rgb_to_xyz(const rgb& c_rgb);

	/*
	* Converts an XYZ color to the CIELAB color space.
	* @param c_xyz XYZ color to convert.
	* @returns The color in CIELAB color space.
	*/
	lab xyz_to_lab(const xyz& c_xyz);

	/*
	* Converts an RGB color to the CIELAB color space.
	* @param c_rgb RGB color to convert.
	* @returns The color in CIELAB color space.
	*/
	lab rgb_to_lab(const rgb& c_rgb);
}