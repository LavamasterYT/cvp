#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>

#define ESC "\x1B"
#define CSI "\x1B["

#define RENDERER_FULL_COLOR 0
#define RENDERER_PALETTE 1
#define RENDERER_KITTY 2

#define renderer_compare(x, y) ((x.r == y.r) && (x.g == y.g) && (x.b == y.b))
#define renderer_rgb_distance(x, y) ((x.r - y.r) * (x.r - y.r)) + ((x.g - y.g) * (x.g - y.g)) + ((x.b - y.b) * (x.b - y.b))

typedef struct renderer_rgb
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
} renderer_rgb;

typedef struct renderer_term_window
{
	int width;
	int height;
	int mode;
	renderer_rgb palette[16];

#ifdef _WIN32
	unsigned long og_in_mode;
	unsigned long og_out_mode;
#endif
} renderer_term_window;

/**
* Initialize the terminal and gets it ready for drawing.
* RENDERER_FULL_COLOR - Attempts to draw to the terminal using the entire RGB colorspace, more intensive and not as compatible but better picture.
* RENDERER_PALETTE - Attempts to draw to the terminal using the predetermined 16 color palette, more compatible and faster but lower quality picture.
* 
* @param mode	The mode that will be used to draw to the terminal.
*	
* @return Pointer to renderer_term_window
*/
renderer_term_window* renderer_init(int mode);

/**
* Initialize the terminal and gets it ready for drawing
*
* @param window	 Pointer to existing terminal window instance
* @param buffer	 Data to draw to the terminal
*/
void renderer_draw(renderer_term_window* window, renderer_rgb* buffer, renderer_rgb* last_frame);

/**
* Destroys and frees resources
*/
void renderer_destroy(renderer_term_window* window);


#endif // RENDERER_H