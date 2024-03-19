#include "decoder.h"

#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <stdlib.h>
#include <time.h>

decoder_context* decoder_init()
{
	decoder_context* ctx = (decoder_context*)malloc(sizeof(decoder_context));

	if (ctx == NULL)
		return NULL;

	ctx->format_ctx = avformat_alloc_context();
	ctx->rgb_frame = av_frame_alloc();
	ctx->frame = av_frame_alloc();
	ctx->packet = av_packet_alloc();
	ctx->video_codec = NULL;
	ctx->video_ctx = NULL;
	ctx->sws_ctx = NULL;
	ctx->video_index = -1;
	ctx->width = -1;
	ctx->height = -1;
	ctx->duration = 0;
	ctx->fps = -1;

	av_log_set_level(AV_LOG_QUIET); // disable ffmpeg output

	if (ctx->format_ctx == NULL ||
		ctx->rgb_frame == NULL ||
		ctx->frame == NULL ||
		ctx->packet == NULL)
	{
		// TODO: free ctx
		return NULL;
	}

	return ctx;
}

int decoder_open_input(decoder_context* ctx, const char* file, int width, int height, int use_multithreading)
{
	if (ctx == NULL)
		return -1; // no ctx

	int ret = 0;

	ret = avformat_open_input(&ctx->format_ctx, file, NULL, NULL);
	if (ret != 0) // error opening the file
		return ret;

	ret = avformat_find_stream_info(ctx->format_ctx, NULL);
	if (ret < 0)
		return ret;

	ctx->video_index = av_find_best_stream(ctx->format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &ctx->video_codec, 0); // get video stream
	ctx->audio_index = av_find_best_stream(ctx->format_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &ctx->audio_codec, 0); // get audio stream

	if (ctx->video_index < 0)
		return -1; // no video stream with decoder found

	ctx->width = width;
	ctx->height = height;
	ctx->fps = (double)ctx->format_ctx->streams[ctx->video_index]->r_frame_rate.num / (double)ctx->format_ctx->streams[ctx->video_index]->r_frame_rate.den;
	ctx->duration = ctx->format_ctx->duration / 1000;

	ctx->video_ctx = avcodec_alloc_context3(ctx->video_codec);
	if (ctx->video_ctx == NULL)
		return -1; // failed allocating codec context

	if (ctx->audio_index >= 0)
	{
		ctx->audio_ctx = avcodec_alloc_context3(ctx->audio_codec);
		if (ctx->audio_ctx == NULL)
			ctx->audio_index = -1; // failed allocating codec context for audio
	}

	ret = avcodec_parameters_to_context(ctx->video_ctx, ctx->format_ctx->streams[ctx->video_index]->codecpar);
	if (ret < 0)
		return -1; // failed filling codec context

	if (ctx->audio_index >= 0)
	{
		ret = avcodec_parameters_to_context(ctx->audio_ctx, ctx->format_ctx->streams[ctx->audio_index]->codecpar);
		if (ret < 0)
			ctx->audio_index = -1; // failed filling codec context
	}

	if (use_multithreading)
	{
		// attempt to use multithreading
		ctx->video_ctx->thread_count = 0;
		if (ctx->video_codec->capabilities & AV_CODEC_CAP_FRAME_THREADS)
			ctx->video_ctx->thread_type = FF_THREAD_FRAME;
		else if (ctx->video_codec->capabilities & AV_CODEC_CAP_SLICE_THREADS)
			ctx->video_ctx->thread_type = FF_THREAD_SLICE;
		else
			ctx->video_ctx->thread_count = 1;
	}

	if (avcodec_open2(ctx->video_ctx, ctx->video_codec, NULL))
		return -1; // failed opening codec

	if (ctx->audio_index >= 0)
		if (avcodec_open2(ctx->audio_ctx, ctx->audio_codec, NULL))
			ctx->audio_index = -1; // failed opening codec

	ctx->sws_ctx = sws_getContext(
		ctx->video_ctx->width, ctx->video_ctx->height, ctx->video_ctx->pix_fmt,
		width, height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
	if (ctx->sws_ctx == NULL)
		return -1; // failed allocating scaling context

	return 0;
}

decoder_rgb* decoder_alloc_rgb(decoder_context* ctx)
{
	if (ctx == NULL)
		return NULL;

	if (ctx->width < 1 ||
		ctx->height < 1)
		return NULL;

	return (decoder_rgb*)malloc(sizeof(decoder_rgb) * 3 * (ctx->width * ctx->height));
}

int decoder_read_frame(decoder_context* ctx)
{
	while (1)
	{
		int ret = av_read_frame(ctx->format_ctx, ctx->packet);

		if (ret < 0)
			return -1; // eof or error reading frame

		if (ctx->packet->stream_index != ctx->video_index && ctx->packet->stream_index != ctx->audio_index)
		{
			av_packet_unref(ctx->packet);
			continue;
		}

		if (ctx->packet->stream_index == ctx->video_index)
		{
			if (avcodec_send_packet(ctx->video_ctx, ctx->packet) || avcodec_receive_frame(ctx->video_ctx, ctx->frame))
			{
				av_packet_unref(ctx->packet);
				continue;
			}
		}
		else if (ctx->packet->stream_index == ctx->audio_index)
		{
			if (avcodec_send_packet(ctx->audio_ctx, ctx->packet) || avcodec_receive_frame(ctx->audio_ctx, ctx->frame))
			{
				av_packet_unref(ctx->packet);
				continue;
			}
		}

		ret = ctx->packet->stream_index;

		av_packet_unref(ctx->packet);
		return ret;
	}
}

void decoder_discard_frame(decoder_context* ctx)
{
	av_frame_unref(ctx->frame);
}

void decoder_decode_video(decoder_context* ctx, decoder_rgb* buffer)
{
	if (av_image_fill_arrays(ctx->rgb_frame->data, ctx->rgb_frame->linesize, (uint8_t*)buffer, AV_PIX_FMT_RGB24, ctx->width, ctx->height, 1) < 0)
		return; // error on image fill arrays

	// scale image down
	sws_scale(ctx->sws_ctx, (const uint8_t* const*)ctx->frame->data, ctx->frame->linesize, 0, ctx->video_ctx->height, ctx->rgb_frame->data, ctx->rgb_frame->linesize);

	// copy pixels to buffer
	for (int y = 0; y < ctx->height; y++)
	{
		int index = y * ctx->rgb_frame->linesize[0];
		memcpy(&buffer[ctx->width * y], &ctx->rgb_frame->data[0][index], ctx->width * sizeof(decoder_rgb));
	}

	av_frame_unref(ctx->frame);
	av_frame_unref(ctx->rgb_frame);
}

int64_t decoder_seek(decoder_context *ctx, int64_t ms)
{
	double fps = av_q2d(ctx->format_ctx->streams[ctx->video_index]->r_frame_rate);
	int64_t frame = (int64_t)(ms * fps / 1000.0);

	double sec = frame / fps;
	int64_t ts = ctx->format_ctx->streams[ctx->video_index]->start_time;
	double tb = av_q2d(ctx->format_ctx->streams[ctx->video_index]->time_base);
	ts += (int64_t)(sec / tb + 0.5);

	av_seek_frame(ctx->format_ctx, ctx->video_index, ts, AVSEEK_FLAG_BACKWARD);
	avcodec_flush_buffers(ctx->video_ctx);

	return frame;
}

void decoder_ctx_destroy(decoder_context* ctx)
{
	sws_freeContext(ctx->sws_ctx);
	avcodec_free_context(&ctx->video_ctx);
	if (ctx->audio_index > -1)
		avcodec_free_context(&ctx->audio_ctx);
	avformat_close_input(&ctx->format_ctx);

	free(ctx);
}