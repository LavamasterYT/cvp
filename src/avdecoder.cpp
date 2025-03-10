#include "avdecoder.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <fmt/core.h>

#include "timer.h"

extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
}

#include "colors.h"

AVDecoder::AVDecoder() {
    mVideoIndex = -1;
    mAudioIndex = -1;
    mTargetWidth = 0;
    mTargetHeight = 0;
    mScaledWidth = 0;
    mScaledHeight = 0;

    mFormatCtx = avformat_alloc_context();
    mPacket = av_packet_alloc();
    mRawFrame = av_frame_alloc();
    mScaledFrame = av_frame_alloc();
    mSws = nullptr;
    mVideoCtx = nullptr;
    mAudioCtx = nullptr;
    mVideoCodec = nullptr;
    mAudioCodec = nullptr;

    av_log_set_level(AV_LOG_VERBOSE);
}

AVDecoder::~AVDecoder() {
    if (mVideoCtx) avcodec_free_context(&mVideoCtx);
    if (mAudioCtx) avcodec_free_context(&mAudioCtx);
    if (mPacket) av_packet_free(&mPacket);
    if (mRawFrame) av_frame_free(&mRawFrame);
    if (mScaledFrame) av_frame_free(&mScaledFrame);
    if (mFormatCtx) avformat_close_input(&mFormatCtx);
    if (mSws) sws_freeContext(mSws);
}

int AVDecoder::open(const char* file, bool openAudioStream) {
    int ret = 0;

    // Open file
    ret = avformat_open_input(&mFormatCtx, file, nullptr, nullptr);
    if (ret != 0)
        return AVDECODER_ERROR_FILE;

    avformat_find_stream_info(mFormatCtx, NULL);

    // Find best streams
    mVideoIndex = av_find_best_stream(mFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &mVideoCodec, 0);
    if (openAudioStream)
        mAudioIndex = av_find_best_stream(mFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, &mAudioCodec, 0);
    if (mVideoIndex < 0)
        return AVDECODER_ERROR_STREAM;

    if (mAudioIndex < 0 && openAudioStream)
        openAudioStream = false;

    // Allocate codec context
    mVideoCtx = avcodec_alloc_context3(mVideoCodec);
    if (openAudioStream)
        mAudioCtx = avcodec_alloc_context3(mAudioCodec);
    if (!mVideoCtx || (!mAudioCtx && openAudioStream))
        return AVDECODER_ERROR_CODEC;

    // Fill parameters
    ret = avcodec_parameters_to_context(mVideoCtx, mFormatCtx->streams[mVideoIndex]->codecpar);
    if (ret < 0)
        return AVDECODER_ERROR_CODEC_CTX;
    if (openAudioStream)
        if (avcodec_parameters_to_context(mAudioCtx, mFormatCtx->streams[mAudioIndex]->codecpar))
            return AVDECODER_ERROR_CODEC_CTX;
    
    // Setup multithreading
    mVideoCtx->thread_count = 0;
    if (mVideoCodec->capabilities & AV_CODEC_CAP_FRAME_THREADS)
        mVideoCtx->thread_type = FF_THREAD_FRAME;
    else if (mVideoCodec->capabilities & AV_CODEC_CAP_SLICE_THREADS)
        mVideoCtx->thread_type = FF_THREAD_SLICE;
    
    // Open codecs
    if (avcodec_open2(mVideoCtx, mVideoCodec, NULL))
        return AVDECODER_ERROR_CODEC;
    if (openAudioStream)
        if (avcodec_open2(mAudioCtx, mAudioCodec, NULL))
            return AVDECODER_ERROR_CODEC;
    
    return 0;
}

int AVDecoder::read_frame(AVDecoder::FrameData& frame) {
    while (1) {
        // Read next frame
        if (av_read_frame(mFormatCtx, mPacket) != 0)
            return AVDECODER_ERROR_EOF;
        
        // Is audio or video stream?
        if (!(mPacket->stream_index == mVideoIndex || mPacket->stream_index == mAudioIndex)) {
            av_packet_unref(mPacket);
            continue; // If not, next frame please
        }

        // Retrieve frame data
        if (mPacket->stream_index == mVideoIndex) {
            int ret = avcodec_send_packet(mVideoCtx, mPacket);
            if (ret < 0 && ret != AVERROR(EAGAIN)) {
                av_packet_unref(mPacket);
                continue;
            }
            ret = avcodec_receive_frame(mVideoCtx, mRawFrame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                av_packet_unref(mPacket);
                continue;
            } else if (ret < 0) {
                av_packet_unref(mPacket);
                continue;
            }
            frame.codec_ctx = mVideoCtx;
        }
        else if (mPacket->stream_index == mAudioIndex) {
            if (avcodec_send_packet(mAudioCtx, mPacket)) {
                av_packet_unref(mPacket);
                continue;
            }
            if (avcodec_receive_frame(mAudioCtx, mRawFrame)) {
                av_packet_unref(mPacket);
                continue;
            }
            frame.codec_ctx = mAudioCtx;
        }

        // Set approriate fields and return
        frame.format_ctx = mFormatCtx;
        frame.stream = mPacket->stream_index == mVideoIndex ? AVDECODER_STREAM_VIDEO : AVDECODER_STREAM_AUDIO;
        frame.frame = mRawFrame;

        if (mPacket->dts != AV_NOPTS_VALUE)
            frame.pts = mRawFrame->pts * av_q2d(mFormatCtx->streams[mPacket->stream_index]->time_base);
        else
            frame.pts = 0;

        av_packet_unref(mPacket);

        return 0;
    }
}

void AVDecoder::discard_frame() {
    av_frame_unref(mRawFrame);
}

void AVDecoder::rescale_decoder(int width, int height) {
    mTargetWidth = width;
    mTargetHeight = height;

    float videoAspectRatio, targetAspectRatio, scale;

    // Determine aspect ratio and dimensions
    videoAspectRatio = static_cast<float>(mVideoCtx->width) / static_cast<float>(mVideoCtx->height);
    targetAspectRatio = static_cast<float>(width) / static_cast<float>(height);

    if (targetAspectRatio > videoAspectRatio) {
        scale = static_cast<float>(mVideoCtx->height) / static_cast<float>(height);
        mScaledHeight = height;
        mScaledWidth = static_cast<float>(mVideoCtx->width) / scale;
    }
    else {
        scale = static_cast<float>(mVideoCtx->width) / static_cast<float>(width);
        mScaledWidth = width;
        mScaledHeight = static_cast<float>(mVideoCtx->height) / scale;
    }

    mScaledBuffer.resize(mScaledWidth * mScaledHeight);

    mSws = sws_getContext(mVideoCtx->width, mVideoCtx->height, mVideoCtx->pix_fmt, mScaledWidth, mScaledHeight, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
    if (!mSws)
        return;
}

void AVDecoder::decode_video(std::vector<colors::rgb>& buffer) {

    // Set buffer sizes
    if (buffer.size() != mTargetHeight * mTargetWidth)
        buffer.resize(mTargetWidth * mTargetHeight);

    // Initialize frame
    if (av_image_fill_arrays(mScaledFrame->data, mScaledFrame->linesize, (uint8_t*)mScaledBuffer.data(), AV_PIX_FMT_RGB24, mScaledWidth, mScaledHeight, 1) < 0)
        return;

    // Scale
    sws_scale(mSws, (const uint8_t* const*)mRawFrame->data, mRawFrame->linesize, 0, mVideoCtx->height, mScaledFrame->data, mScaledFrame->linesize);

    // Copy pixels to scaled buffer
    for (int y = 0; y < mScaledHeight; y++) {
        int index = y * mScaledFrame->linesize[0];
        memcpy(&mScaledBuffer.at(mScaledWidth * y), &mScaledFrame->data[0][index], mScaledWidth * sizeof(colors::rgb));
    }

    if (mScaledWidth == mTargetWidth) {
        int offset = (mTargetHeight - mScaledHeight) / 2;

        for (int y = offset; y < mScaledHeight + offset; y++)
            for (int x = 0; x < mTargetWidth; x++)
                buffer[x + mTargetWidth * y] = mScaledBuffer[x + mScaledWidth * (y - offset)];
    }
    else if (mScaledHeight == mTargetHeight) {
        int offset = (mTargetWidth - mScaledWidth) / 2;

        for (int y = 0; y < mTargetHeight; y++)
            for (int x = offset; x < mScaledWidth + offset; x++)
                buffer[x + mTargetWidth * y] = mScaledBuffer[(x - offset) + mScaledWidth * y];
    }

    av_frame_unref(mRawFrame);
    av_frame_unref(mScaledFrame);
}

int64_t AVDecoder::seek(int64_t ms) {
    int64_t frame = (int64_t)(ms * fps() / 1000.0); // Convert the milliseconds to the number of frames to seek
	double sec = frame / fps(); // Convert the frames to seconds (I think I can just convert from ms to sec
                                // instead of converting to frames first?)
	int64_t ts = mFormatCtx->streams[mVideoIndex]->start_time;
	double tb = av_q2d(mFormatCtx->streams[mVideoIndex]->time_base);
	ts += (int64_t)(sec / tb + 0.5); // Convert all that to a timestamp to sync to

	av_seek_frame(mFormatCtx, mVideoIndex, ts, AVSEEK_FLAG_BACKWARD);
	avcodec_flush_buffers(mVideoCtx);

	return frame;
}

double AVDecoder::fps() {
    if (mVideoIndex < 0 || mFormatCtx == nullptr)
        return 0;

    AVRational framerate = av_guess_frame_rate(mFormatCtx, mFormatCtx->streams[mVideoIndex], nullptr);
    return av_q2d(framerate);
}

std::chrono::seconds AVDecoder::duration() {
    // Duration is returned in AV_TIME_BASE fractional seconds.
    // Therefore, to get the duration in seconds, we must
    // multiply the duration by 1/AV_TIME_BASE.
    // We could also multiply duration by AV_TIME_BASE_Q

    int64_t d = mFormatCtx->duration * av_q2d(AV_TIME_BASE_Q);
    if (d < 0)
        return std::chrono::seconds(1);
    return std::chrono::seconds(d);
}

AVCodecContext* AVDecoder::get_audio_context() {
    return mAudioCtx;
}
