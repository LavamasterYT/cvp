#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ui_context* ui_init(int duration, int width, int height)
{
    ui_context* ctx = (ui_context*)malloc(sizeof(ui_context));
    if (ctx == NULL)
        return NULL;

    if (duration == 0)
        return NULL;

    // calculate hours, minutes, and seconds value
    ctx->d_hours = duration / 3600000;
    ctx->d_minutes = (duration % 3600000) / 60000;
    ctx->d_seconds = ((duration % 3600000) % 60000) / 1000;
    ctx->duration = duration;

    ctx->y = height;

    // set linesize accordingly
    // if no hours, no need to render hour time
    if (ctx->d_hours == 0)
        ctx->linesize = width - 14;
    else
        ctx->linesize = width - 20;

    ctx->buffer = malloc(ctx->linesize + 1);
    if (ctx->buffer == NULL)
        return NULL;

    memset(ctx->buffer, 0, ctx->linesize + 1);

    return ctx;
}

void ui_draw(ui_context* ctx, int ms)
{
    if (ms == 0)
        return; // avoid dividing by 0;

    if (ctx->linesize < 2)
        return; // no point in drawing ui ngl

    printf("\x1B[37m\x1b[40m" // change colors
          "\x1B[%d;0H", ctx->y); // move cursor

    // calculate hours, minutes, and seconds for current time
    int c_hours = ms / 3600000;
    int c_minutes = (ms % 3600000) / 60000;
    int c_seconds = ((ms % 3600000) % 60000) / 1000;

    // calculate how much of progress bar to fill
    double progress = (double)ms / (double)ctx->duration;
    int progress_width = (int)(ctx->linesize * progress);
    int remaining_width = ctx->linesize - progress_width;

    // if no hours, no need to render hour time, else we do
    if (ctx->d_hours == 0)
    {
        memset(ctx->buffer, '=', progress_width);
        memset(ctx->buffer + progress_width, ' ', remaining_width);

        printf("%02d:%02d [%s] %02d:%02d", c_minutes, c_seconds, ctx->buffer, ctx->d_minutes, ctx->d_seconds);
    }
    else
    {
        memset(ctx->buffer, '=', progress_width);
        memset(ctx->buffer + progress_width, ' ', remaining_width);

        printf("%02d:%02d:%02d [%s] %02d:%02d:%02d", c_hours, c_minutes, c_seconds, ctx->buffer, ctx->d_hours, ctx->d_minutes, ctx->d_seconds);
    }
}

void ui_destroy(ui_context* ctx)
{
    free(ctx->buffer);
    free(ctx);
}
