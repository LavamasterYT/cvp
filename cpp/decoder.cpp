#include "decoder.h"

#include <cstdint>
#include <memory>
#include <vector>
#include <string>

extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
	#include <libavutil/imgutils.h>
}

decoder::decoder()
{
	duration = 0;
	audio_index = -1;
	video_index = -1;
	fps = 0;

	fmtctx = avformat_alloc_context();
	packet = av_packet_alloc();
	raw_frame = av_frame_alloc();
	scaled_frame = av_frame_alloc();
	video_ctx = nullptr;
	audio_ctx = nullptr;
	video_codec = nullptr;
	audio_codec = nullptr;

	av_log_set_level(AV_LOG_QUIET); // Prevent FFmpeg to print to the console.
}

decoder::~decoder()
{
	if (video_ctx) avcodec_free_context(&video_ctx);
	if (audio_ctx) avcodec_free_context(&audio_ctx);
	if (packet) av_packet_free(&packet);
	if (raw_frame) av_frame_free(&raw_frame);
	if (scaled_frame) av_frame_free(&scaled_frame);
	if (fmtctx) avformat_close_input(&fmtctx);
}

int decoder::open(std::string file, bool get_audio)
{
	int ret = 0;

	// Open file
	ret = avformat_open_input(&fmtctx, file.c_str(), NULL, NULL);
	if (ret != 0) return DECODER_ERROR_FILE;
	avformat_find_stream_info(fmtctx, nullptr);

	// Find the best stream for video and audio
	video_index = av_find_best_stream(fmtctx, AVMEDIA_TYPE_VIDEO, -1, -1, &video_codec, 0);
	if (get_audio)
		audio_index = av_find_best_stream(fmtctx, AVMEDIA_TYPE_AUDIO, -1, -1, &audio_codec, 0);
	if (video_index < 0 || (audio_index < 0 && get_audio)) return DECODER_ERROR_STREAM;

	// Allocate codec context
	video_ctx = avcodec_alloc_context3(video_codec);
	if (get_audio)
		audio_ctx = avcodec_alloc_context3(audio_codec);
	if (!video_ctx || (!audio_ctx && get_audio)) return DECODER_ERROR_CODEC;

	// Fill codec parameters?
	ret = avcodec_parameters_to_context(video_ctx, fmtctx->streams[video_index]->codecpar);
	if (ret < 0) return DECODER_ERROR_CODEC_CTX;
	if (get_audio)
	{
		ret = avcodec_parameters_to_context(audio_ctx, fmtctx->streams[audio_index]->codecpar);
		if (ret < 0) return DECODER_ERROR_CODEC_CTX;
	}

	if (video_ctx->codec_id == AV_CODEC_ID_VP9)
		video_ctx->thread_count = 1; // VP9 is sensitive to this for some reason
	else
		video_ctx->thread_count = 0; // Just let FFmpeg handle thread count

	//  Multithreading?
	if (video_codec->capabilities & AV_CODEC_CAP_FRAME_THREADS)
		video_ctx->thread_type = FF_THREAD_FRAME;
	else if (video_codec->capabilities & AV_CODEC_CAP_SLICE_THREADS)
		video_ctx->thread_type = FF_THREAD_SLICE;

	// Open codecs
	if (avcodec_open2(video_ctx, video_codec, NULL)) return DECODER_ERROR_CODEC;
	if (get_audio)
		if (avcodec_open2(audio_ctx, audio_codec, NULL)) return DECODER_ERROR_CODEC;

	AVRational fr = av_guess_frame_rate(fmtctx, fmtctx->streams[video_index], nullptr);
	fps = av_q2d(fr);

	duration = fmtctx->duration / 1000;

	return 0;
}

int decoder::read_frame(double* pts)
{
	while (1)
	{
		// Read the next frame
		if (av_read_frame(fmtctx, packet) != 0) return DECODER_ERROR_EOF;

		// Continue to the next frame if it isnt audio or video
		if (!(packet->stream_index == audio_index || packet->stream_index == video_index))
		{
			av_packet_unref(packet);
			continue;
		}
		
		// If its a video or audio frame, try to retrieve the data
		if (packet->stream_index == video_index)
		{
			if (avcodec_send_packet(video_ctx, packet) || avcodec_receive_frame(video_ctx, raw_frame))
			{
				av_packet_unref(packet);
				continue;
			}
		}
		else if (packet->stream_index == audio_index)
		{
			if (avcodec_send_packet(audio_ctx, packet) || avcodec_receive_frame(audio_ctx, raw_frame))
			{
				av_packet_unref(packet);
				continue;
			}
		}

		// Return the index of the type of data we recieved
		int index = packet->stream_index;
		av_packet_unref(packet);

		*pts = raw_frame->best_effort_timestamp * av_q2d(fmtctx->streams[video_index]->time_base);

		return index;
	}
}

AVFrame* decoder::get_raw_frame()
{
	return raw_frame;
}

AVCodecContext* decoder::get_audio_ctx()
{
	return audio_ctx;
}

AVFormatContext* decoder::get_format_ctx()
{
	return fmtctx;
}

void decoder::discard_frame()
{
	av_frame_unref(raw_frame);
}

int64_t decoder::seek(int64_t ms)
{
	// I have no clue how it works and i forgot how it works sorry
	int64_t frame = (int64_t)(ms * fps / 1000.0);

	double sec = frame / fps;
	int64_t ts = fmtctx->streams[video_index]->start_time;
	double tb = av_q2d(fmtctx->streams[video_index]->time_base);
	ts += (int64_t)(sec / tb + 0.5);

	av_seek_frame(fmtctx, video_index, ts, AVSEEK_FLAG_BACKWARD);
	avcodec_flush_buffers(video_ctx);

	return frame;
}

void decoder::decode(std::vector<colors_rgb>& buffer, int width, int height, int* output_width, int* output_height, bool grayscale)
{
	// Setting aspect ratio data
	float video_aspect_ratio = (float)video_ctx->width / (float)video_ctx->height;
	float target_aspect_ratio = (float)width / (float)height;

	if (target_aspect_ratio > video_aspect_ratio)
	{
		float scale = (float)video_ctx->height / (float)height;
		*output_height = height;
		*output_width = (float)video_ctx->width / scale;
	}
	else
	{
		float scale = (float)video_ctx->width / (float)width;
		*output_width = width;
		*output_height = (float)video_ctx->height / scale;
	}

	// Intialize scaler
	struct SwsContext* sws = sws_getContext(video_ctx->width, video_ctx->height, video_ctx->pix_fmt, *output_width, *output_height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
	if (!sws)
		return;

	// Set buffer to appropriate size and fill with zeroes
	buffer.resize(*output_width * *output_height);

	// Intialize scaled_frame
	if (av_image_fill_arrays(scaled_frame->data, scaled_frame->linesize, (uint8_t*)buffer.data(), AV_PIX_FMT_RGB24, *output_width, *output_height, 1) < 0)
		return;

	// Scale frame
	sws_scale(sws, (const uint8_t* const*)raw_frame->data, raw_frame->linesize, 0, video_ctx->height, scaled_frame->data, scaled_frame->linesize);

	// Copy pixels to buffer
	for (int y = 0; y < *output_height; y++)
	{
		int index = y * scaled_frame->linesize[0];
		memcpy(&buffer.at(*output_width * y), &scaled_frame->data[0][index], *output_width * sizeof(colors_rgb));
	}

	if (grayscale)
	{
		// Convert to grayscale
		for (int y = 0; y < *output_height; y++)
		{
			for (int x = 0; x < *output_width; x++)
			{
				uint8_t gs = (buffer[x + *output_width * y].r * 0.2126) + (buffer[x + *output_width * y].g * 0.7152) + (buffer[x + *output_width * y].b * 0.0722);
				buffer[x + *output_width * y] = { gs, gs, gs };
			}
		}
	}

	av_frame_unref(raw_frame);
	av_frame_unref(scaled_frame);
	sws_freeContext(sws);
}