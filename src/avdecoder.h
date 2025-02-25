#pragma once

#include <chrono>
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

/**
 * @brief A simple audio/video decoder
 * Decodes video and audio files into packets that can be used for simple playback
 */
class AVDecoder {

private:
	int mVideoIndex;
	int mAudioIndex;
	int mTargetWidth;
	int mTargetHeight;
	int mScaledWidth;
	int mScaledHeight;

	AVFormatContext* mFormatCtx;
	AVCodecContext* mVideoCtx;
	AVCodecContext* mAudioCtx;
	AVPacket* mPacket;
	AVFrame* mRawFrame;
	AVFrame* mScaledFrame;
	struct SwsContext* mSws;
	std::vector<colors::rgb> mScaledBuffer;
	const AVCodec* mAudioCodec;
	const AVCodec* mVideoCodec;

public:
	struct FrameData {
		double pts; // The pts of the frame that was read.
		int stream; // The type of stream that is stored.
		AVFrame* frame; // The raw frame.
		AVCodecContext* codec_ctx; // The respective codec context of the frame.
		AVFormatContext* format_ctx; // The format context of the file.
	};

	/**
	 * @brief Construct a new AVDecoder object
	 * Initializes any variables that need to be initalized.
	 */
	AVDecoder();
	~AVDecoder();

	/**
	 * @brief Opens a file to be decoded.
	 * 
	 * Open and reads the default (best) streams of the input file and initializes
	 * the necessary decoders needed for decoding the video and audio if needed.
	 * 
	 * @param file The path to the file to open.
	 * @param openAudioStream If set true, initialize an audio decoder too if audio stream exists.
	 * @return 0 on success, else AVDecoder_Error on error.
	 */
	int open(const char* file, bool openAudioStream);

	/**
	 * @brief Reads the next valid audio or video frame from the file. 
	 * Continues reading the next frame, skipping any errornous, invalid, or non-selected frames.
	 * Sets the passed in FrameData struct data to appropriate values:
	 * - Sets codec_ctx to the audio or video context, depending on the frame read.
	 * - Sets frame to the raw frame read from the decoder.
	 * - Sets stream to the stream index of the frame read.
	 * - Sets pts to the pts of the frame read *times* the time base of the video.
	 * @param frame A FrameData struct to store frame information.
	 * @return int AVDECODER_ERROR_EOF on error or end of file.
	 */
	int read_frame(AVDecoder::FrameData& frame);

	/**
	 * @brief Discards the frame freeing any allocated buffers if any in the underlying decoder.
	 */
	void discard_frame();

	/**
	 * @brief Reinitializes scaling structures
	 * Whenever the target width and height for scaling down (or up) video changes, make sure that the
	 * proper structs are reinitialized.
	 * 
	 * @param width 
	 * @param height 
	 */
	void rescale_decoder(int width, int height);

	/**
	 * @brief Reads the raw video frame to a buffer.
	 * Decodes and scales up or down the video frame to an RGB buffer. It also discards the frame after its done decoding.
	 * @param buffer The buffer to write the decoded video to.
	 * @param width The width to scale the video to.
	 * @param height The height to scale the video to.
	 */
	void decode_video(std::vector<colors::rgb>& buffer);

	/**
	 * @brief Seeks the video forward or backward.
	 * 
	 * @param ms The amount of time in milliseconds to seek by. Can be negative or positive.
	 * @return int64_t The frame number it seeked to.
	 */
	int64_t seek(int64_t ms);

	/**
	 * @brief Gets the framerate of the video by using av_guess_frame_rate.
	 * 
	 * Uses av_q2d to convert the rational to a double.
	 * 
	 * @return double The framerate of the video.
	 */
	double fps();

	/**
	 * @brief Self-explanitory
	 * 
	 * @return std::chrono::seconds Time in chrono duration.
	 */
	std::chrono::seconds duration();

	/**
	 * @brief Get the audio context object
	 * 
	 * @return AVCodecContext* 
	 */
	AVCodecContext* get_audio_context();
};
