#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
}

#include "colors.h"

enum decoder_error {
	DECODER_ERROR_FILE = -1,
	DECODER_ERROR_STREAM = -2,
	DECODER_ERROR_CODEC = -3,
	DECODER_ERROR_CODEC_CTX = -4,
	DECODER_ERROR_EOF = -5
};

class decoder
{
public:
	int duration;
	int audio_index;
	int video_index;
	double fps;

	/**
	* Intializes the docoder class
	*/
	decoder();

	/**
	* Opens and reads the file
	* 
	* @param file The file to open
	* @param get_audio If true, opens the audio stream as well
	* @return 0 on success, an error code on failure.
	*/
	int open(std::string file, bool get_audio);

	/**
	* Reads the next frame from the video.
	* 
	* @return The index of the frame (aka type of frame)
	* @note Frame doesn't necessarily mean video frame, it could also be audio.
	*/
	int read_frame(double* pts);

	/**
	* Discards the frame that was read.
	*/
	void discard_frame();

	/**
	* Seeks to the specified point of the video.
	* 
	* @return The frame number of the seeked point.
	*/
	int64_t seek(int64_t ms);

	/**
	* Decodes the video frame, scales it while conforming to aspect ratio,
	* and writes the frame to the buffer.
	* 
	* @param buffer The buffer to write the final converted frame to.
	* @param width The requested width to scale to
	* @param height The requested height to scale to
	* @param output_width Pointer to store the scaled width to.
	* @param output_height Pointer to store the scaled height to.
	*/
	void decode(std::vector<colors_rgb>& buffer, int width, int height, int* output_width, int* output_height, bool grayscale);

	/**
	* Frees any resources allocated in this class.
	*/
	~decoder();

private:
	AVFormatContext* fmtctx;
	AVCodecContext* video_ctx;
	AVCodecContext* audio_ctx;
	AVPacket* packet;
	AVFrame* raw_frame;
	AVFrame* scaled_frame;
	const AVCodec* audio_codec;
	const AVCodec* video_codec;
};

