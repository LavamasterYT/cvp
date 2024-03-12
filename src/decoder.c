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
	ctx->codec = NULL;
	ctx->codec_ctx = NULL;
	ctx->sws_ctx = NULL;
	ctx->index = -1;
	ctx->width = -1;
	ctx->height = -1;
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

int decoder_open_input(decoder_context* ctx, const char* file, int width, int height)
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

	ctx->index = av_find_best_stream(ctx->format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &ctx->codec, 0); // get video stream

	if (ctx->index < 0)
		return -1; // no video stream with decoder found

	ctx->width = width;
	ctx->height = height;
	ctx->fps = ctx->format_ctx->streams[ctx->index]->r_frame_rate.num;

	ctx->codec_ctx = avcodec_alloc_context3(ctx->codec);
	if (ctx->codec_ctx == NULL)
		return -1; // failed allocating codec context

	ret = avcodec_parameters_to_context(ctx->codec_ctx, ctx->format_ctx->streams[ctx->index]->codecpar);
	if (ret < 0)
		return -1; // failed filling codec context

	if (avcodec_open2(ctx->codec_ctx, ctx->codec, NULL))
		return -1; // failed opening codec

	ctx->sws_ctx = sws_getContext(
		ctx->codec_ctx->width, ctx->codec_ctx->height, ctx->codec_ctx->pix_fmt,
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

int decoder_read_frame(decoder_context* ctx, decoder_rgb* buffer)
{
	av_packet_unref(ctx->packet);

	while (1)
	{
		if (av_read_frame(ctx->format_ctx, ctx->packet) < 0)
		{
			return -1; // eof or error reading frame
		}

		if (ctx->packet->stream_index != ctx->index)
		{
			continue;
		}

		if (avcodec_send_packet(ctx->codec_ctx, ctx->packet) || avcodec_receive_frame(ctx->codec_ctx, ctx->frame))
		{
			continue;
		}

		av_packet_unref(ctx->packet);

		if (av_image_fill_arrays(ctx->rgb_frame->data, ctx->rgb_frame->linesize, (uint8_t*)buffer, AV_PIX_FMT_RGB24, ctx->width, ctx->height, 1) < 0)
		{
			return -1; // error on image fill arrays
		}

		// scale image down
		sws_scale(ctx->sws_ctx, (const uint8_t* const*)ctx->frame->data, ctx->frame->linesize, 0, ctx->codec_ctx->height, ctx->rgb_frame->data, ctx->rgb_frame->linesize);

		// copy pixels to buffer
		for (int y = 0; y < ctx->height; y++)
		{
			int index = y * ctx->rgb_frame->linesize[0];
			memcpy(&buffer[ctx->width * y], &ctx->rgb_frame->data[0][index], ctx->width * sizeof(decoder_rgb));
		}

		return 0;
	}
}

void decoder_ctx_destroy(decoder_context* ctx)
{
	sws_freeContext(ctx->sws_ctx);
	avcodec_free_context(&ctx->codec_ctx);
	avformat_close_input(&ctx->format_ctx);

	free(ctx);
}