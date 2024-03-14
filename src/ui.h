#ifndef UI_H
#define UI_H

typedef struct ui_context
{
	int d_hours; // duration of video in hours
	int d_minutes; // duration of video in minutes
	int d_seconds; // duration of video in seconds
	int duration; // duration in ms
	int y; // y value to set cursor to
	int linesize; // size of video progress bar
	char* buffer; // buffer of ui text
} ui_context;

ui_context* ui_init(int duration, int width, int height);
void ui_draw(ui_context* ctx, int ms);
void ui_destroy(ui_context* ctx);

#endif // UI_H