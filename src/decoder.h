#ifndef DECODER_H
#define DECODER_H

/*

	SOME QUICK DOCUMENTATION ABOUT DECODER.H

	What is it?
	decoder.h is a C library that utilizes the FFmpeg libraries to decode a video file, and read each frame to a simple RGB buffer
	for the user to do whatever they wish to do. They have the option to scale it down to a set dimension.

	How do I use it?

	Initialize the decoder:
	decoder_context* ctx = decoder_init();

	This initialize backend stuff to prepare reading files.

	Open a file:
	decoder_open_input(ctx, "sample file.mp4");

	Opens a file to be ready to read from.

	Read streams from file:
	int len;
	decoder_stream* streams = decoder_get_streams(ctx, &len);

	Reads the streams from the video file and sets the len variable to the count of streams in said video file.

	// CONTINUE LATER

	
	*/

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

typedef struct
{
	AVFormatContext* format_ctx;
	AVCodecContext* codec_ctx;
	struct SwsContext* sws_ctx;
	AVCodec* codec;
	AVFrame* frame;
	AVFrame* rgb_frame;
	AVPacket* packet;
	int index;
	int width;
	int height;
	double fps;
} decoder_context;

typedef struct
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
} decoder_rgb;

/**
* Initializes the decoder for reading video files.
* 
* @return Pointer to a decoder_context if successful, NULL if failed to initialize internal resources.
*/
decoder_context* decoder_init();

/**
* Allocates appropriate memory for an RGB buffer.
* 
* @return Pointer to a decoder_rgb buffer if successfull, NULL otherwise if failed allocating memory.
*/
decoder_rgb* decoder_alloc_rgb(decoder_context* ctx);

/**
* Opens a file, and sets up scalers to read frames at a specified width and height
* 
* @param ctx				Decoder context
* @param file				Path to video file to open
* @param width				Target width to scale frames to
* @param height				Target height to scale frames to
* @param use_multithreading	Use multithreading, use for some videos that are slow to decode.
* 
* @return >=0 if successful opening the file, otherwise failed opening file.
*/
int decoder_open_input(decoder_context* ctx, const char* file, int width, int height, int use_multithreading);

/**
* Reads the next available frame and writes it to buffer/
*
* @param ctx	Decoder context
* @param buffer	Pointer to buffer to write frame to
*
* @return 0 if successful reading the frame, otherwise encountered EOF or other internal error.
*/
int decoder_read_frame(decoder_context* ctx, decoder_rgb* buffer);

/**
* Destroys and frees any allocated resources
*
* @param ctx	Decoder context
*/
void decoder_ctx_destroy(decoder_context* ctx);

#endif // DECODER_H