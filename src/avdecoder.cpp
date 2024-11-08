#include "avdecoder.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

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

    mFormatCtx = avformat_alloc_context();
    mPacket = av_packet_alloc();
    mRawFrame = av_frame_alloc();
    mScaledFrame = av_frame_alloc();
    mVideoCtx = nullptr;
    mAudioCtx = nullptr;
    mVideoCodec = nullptr;
    mAudioCodec = nullptr;

    av_log_set_level(AV_LOG_QUIET);
}

AVDecoder::~AVDecoder() {
    if (mVideoCtx) avcodec_free_context(&mVideoCtx);
    if (mAudioCtx) avcodec_free_context(&mVideoCtx);
    if (mPacket) av_packet_free(&mPacket);
    if (mRawFrame) av_frame_free(&mRawFrame);
    if (mScaledFrame) av_frame_free(&mScaledFrame);
    if (mFormatCtx) avformat_close_input(&mFormatCtx);
}

int AVDecoder::open(const char* file, bool openAudioStream) {
    int ret = 0;

    // Open file
    ret = avformat_open_input(&mFormatCtx, file, nullptr, nullptr);
    if (ret != 0)
        return AVDECODER_ERROR_FILE;

    // Find best streams
    mVideoIndex = av_find_best_stream(mFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &mVideoCodec, 0);
    if (openAudioStream)
        mAudioIndex = av_find_best_stream(mFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, &mAudioCodec, 0);
    if (mVideoIndex < 0 || (mAudioIndex < 0 && openAudioStream))
        return AVDECODER_ERROR_STREAM;

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
    if (mVideoCtx->codec_id == AV_CODEC_ID_VP9)
        mVideoCtx->thread_count = 1;
    else
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
        
        // Is audio or video stream
        if (!(mPacket->stream_index == mVideoIndex || mPacket->stream_index == mAudioIndex)) {
            av_packet_unref(mPacket);
            continue; // If not, next frame please
        }

        // Retrieve frame data
        if (mPacket->stream_index == mVideoIndex) {
            if (avcodec_send_packet(mVideoCtx, mPacket) || avcodec_receive_frame(mVideoCtx, mRawFrame)) {
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

        if (mPacket->dts != AV_NOPTS_VALUE) {
            frame.pts = mRawFrame->pts * av_q2d(mFormatCtx->streams[mPacket->stream_index]->time_base);
        }
        else {
            frame.pts = 0;
        }

        av_packet_unref(mPacket);

        return 0;
    }
}

void AVDecoder::discard_frame(AVDecoder::FrameData& frame) {
    av_frame_unref(frame.frame);
}

void AVDecoder::decode_video(std::vector<colors::rgb>& buffer, int width, int height) {
    int scaledWidth, scaledHeight;
    float videoAspectRatio, targetAspectRatio, scale;
    std::vector<colors::rgb> scaledBuffer;
    struct SwsContext* sws;

    // Determine aspect ratio and dimensions
    videoAspectRatio = static_cast<float>(mVideoCtx->width) / static_cast<float>(mVideoCtx->height);
    targetAspectRatio = static_cast<float>(width) / static_cast<float>(height);

    if (targetAspectRatio > videoAspectRatio) {
        scale = static_cast<float>(mVideoCtx->height) / static_cast<float>(height);
        scaledHeight = height;
        scaledWidth = static_cast<float>(mVideoCtx->width) / scale;
    }
    else {
        scale = static_cast<float>(mVideoCtx->width) / static_cast<float>(width);
        scaledWidth = width;
        scaledHeight = static_cast<float>(mVideoCtx->height) / scale;
    }

    // Initialize scaler
    sws = sws_getContext(mVideoCtx->width, mVideoCtx->height, mVideoCtx->pix_fmt, scaledWidth, scaledHeight, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
    if (!sws)
        return;

    // Set buffer sizes
    buffer.resize(width * height);
    scaledBuffer.resize(scaledWidth * scaledHeight);

    // Initialize frame
    if (av_image_fill_arrays(mScaledFrame->data, mScaledFrame->linesize, (uint8_t*)scaledBuffer.data(), AV_PIX_FMT_RGB24, scaledWidth, scaledHeight, 1) < 0)
        return;

    // Scale
    sws_scale(sws, (const uint8_t* const*)mRawFrame->data, mRawFrame->linesize, 0, mVideoCtx->height, mScaledFrame->data, mScaledFrame->linesize);

    // Copy pixels to scaled buffer
    for (int y = 0; y < scaledHeight; y++) {
        int index = y * mScaledFrame->linesize[0];
        memcpy(&scaledBuffer.at(scaledWidth * y), &mScaledFrame->data[0][index], scaledWidth * sizeof(colors::rgb));
    }

    if (scaledWidth == width) {
        int offset = (height - scaledHeight) / 2;

        for (int y = offset; y < scaledHeight + offset; y++)
            for (int x = 0; x < width; x++)
                buffer[x + width * y] = scaledBuffer[x + scaledWidth * (y - offset)];
    }
    else if (scaledHeight == height) {
        int offset = (width - scaledWidth) / 2;

        for (int y = 0; y < height; y++)
            for (int x = offset; x < scaledWidth + offset; x++)
                buffer[x + width * y] = scaledBuffer[(x - offset) + scaledWidth * y];
    }

    av_frame_unref(mRawFrame);
    av_frame_unref(mScaledFrame);
    sws_freeContext(sws);
}

int64_t AVDecoder::seek(int64_t ms) {
	// I have no clue how it works and i forgot how it works sorry
    int64_t frame = (int64_t)(ms * fps() / 1000.0);

	double sec = frame / fps();
	int64_t ts = mFormatCtx->streams[mVideoIndex]->start_time;
	double tb = av_q2d(mFormatCtx->streams[mVideoIndex]->time_base);
	ts += (int64_t)(sec / tb + 0.5);

	av_seek_frame(mFormatCtx, mVideoIndex, ts, AVSEEK_FLAG_BACKWARD);
	avcodec_flush_buffers(mVideoCtx);

	return frame;
}

double AVDecoder::fps() {
    if (mVideoIndex < 0 || mFormatCtx == nullptr) {
        return 0;
    }

    AVRational framerate = av_guess_frame_rate(mFormatCtx, mFormatCtx->streams[mVideoIndex], nullptr);
    return av_q2d(framerate);
}

int AVDecoder::duration() {
    return mFormatCtx->duration / 1000;
}

AVCodecContext* AVDecoder::get_audio_context() {
    return mAudioCtx;
}
