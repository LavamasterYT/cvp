#ifndef DECODER_H
#define DECODER_H

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

typedef struct decoder_context
{
	AVFormatContext* format_ctx; // format context
	AVCodecContext* video_ctx; // codec context
	AVCodecContext* audio_ctx; // codec context
	struct SwsContext* sws_ctx; // scaling context
	const AVCodec* video_codec; // video codec
	const AVCodec* audio_codec; // video codec
	AVFrame* frame; // original video frame
	AVFrame* rgb_frame; // downscaled video frame
	AVPacket* packet; // video packet
	int video_index; // video stream index
	int audio_index; // audio stream index
	int width; // output width to scaled down/up to
	int height; // output height to scaled down/up to
	int duration; // length of video (in ms)
	double fps; // self-explanitory
	int grayscale;
} decoder_context;

typedef struct decoder_rgb
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
int decoder_read_frame(decoder_context* ctx);

int64_t decoder_seek(decoder_context* ctx, int64_t ms);

void decoder_discard_frame(decoder_context* ctx);

void decoder_decode_video(decoder_context* ctx, decoder_rgb* buffer);

/**
* Destroys and frees any allocated resources
*
* @param ctx	Decoder context
*/
void decoder_ctx_destroy(decoder_context* ctx);

#endif // DECODER_H