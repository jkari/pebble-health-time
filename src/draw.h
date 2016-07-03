#pragma once

#define PI 3.1415926535f

void draw_arc(GContext* ctx, GPoint center, GSize size, int width, float angle_from, float angle_to, GColor color);
GPoint draw_get_arc_point(float angle, GSize size, GPoint offset);
