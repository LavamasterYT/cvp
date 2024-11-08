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

enum AVDecoder_Error {
	AVDECODER_ERROR_FILE = -1,
	AVDECODER_ERROR_STREAM = -2,
	AVDECODER_ERROR_CODEC = -3,
	AVDECODER_ERROR_CODEC_CTX = -4,
	AVDECODER_ERROR_EOF = -5
};

enum AVDecoder_Stream {
	AVDECODER_STREAM_AUDIO,
	AVDECODER_STREAM_VIDEO
};

class AVDecoder {

private:
	int mVideoIndex;
	int mAudioIndex;

	AVFormatContext* mFormatCtx;
	AVCodecContext* mVideoCtx;
	AVCodecContext* mAudioCtx;
	AVPacket* mPacket;
	AVFrame* mRawFrame;
	AVFrame* mScaledFrame;
	const AVCodec* mAudioCodec;
	const AVCodec* mVideoCodec;

public:
	struct FrameData{
		double pts;
		int stream;
		AVFrame* frame;
		AVCodecContext* codec_ctx;
		AVFormatContext* format_ctx;
	};

	AVDecoder();
	~AVDecoder();

	int open(const char* file, bool openAudioStream);

	int read_frame(AVDecoder::FrameData& frame);

	void discard_frame(AVDecoder::FrameData& frame);

	void decode_video(std::vector<colors::rgb>& buffer, int width, int height);

	int64_t seek(int64_t ms);

	double fps();

	int duration();

	AVCodecContext* get_audio_context();
};
