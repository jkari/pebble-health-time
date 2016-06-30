#include <pebble.h>
#include <math.h>
#include "draw.h"

#ifdef PBL_RECT
static int _deg_to_rad(float deg_angle) {
  float rad_angle = 2.f * PI * deg_angle / 360.f;

  while (rad_angle < -PI) {
    rad_angle += 2.f * PI;
  }

  while (rad_angle > PI) {
    rad_angle -= 2.f * PI;
  }
  
  return rad_angle;
}

static int _get_region_from_angle(GSize size, float rad_angle) {
  float rect_atan = atan2f(size.h, size.w);
  
  if ((rad_angle > -rect_atan) &&  (rad_angle <= rect_atan)){
    return 1;
  } else if ((rad_angle > rect_atan) && (rad_angle <= (PI - rect_atan))) {
    return 2;
  } else if ((rad_angle > (PI - rect_atan)) || (rad_angle <= -(PI - rect_atan))) {
    return 3;
  } else {
    return 4;
  }

  return -1;
}

static GPoint _angle_to_point_in_rect(GRect rect, float deg_angle) {
    float rad_angle = _deg_to_rad(deg_angle);

    // find edge ofview
    // Ref: http://stackoverflow.com/questions/4061576/finding-points-on-a-rectangle-at-a-given-angle
    float aa = rect.size.w;
    float bb = rect.size.h;

    int region = _get_region_from_angle(rect.size, rad_angle);
    float tan_angle = tan(rad_angle);

    GPoint edge_point = GPoint(rect.origin.x + aa/2.f, rect.origin.y + bb/2.f);
    float x_factor = 1;
    float y_factor = 1;

    switch (region) {
        case 1: y_factor = -1; break;
        case 2: y_factor = -1; break;
        case 3: x_factor = -1; break;
        case 4: x_factor = -1; break;
    }

    if ((region == 1) || (region == 3)) {
        edge_point.x += x_factor * (aa / 2.f);
        edge_point.y += y_factor * (aa / 2.f) * tan_angle;
    } else {
        edge_point.x += x_factor * (bb / (2.f * tan_angle));
        edge_point.y += y_factor * (bb / 2.f);
    }

    return edge_point;
}
#endif

void draw_arc(GContext* ctx, GPoint center, int radius, int width, float angle_from, float angle_to, GColor color) {
  graphics_context_set_fill_color(ctx, color);

#ifdef PBL_ROUND  
  graphics_fill_radial(
    ctx,
    GRect(
      center.x - radius,
      center.y - radius,
      2 * radius,
      2 * radius
    ),
    GOvalScaleModeFitCircle,
    width,
    DEG_TO_TRIGANGLE(angle_from),
    DEG_TO_TRIGANGLE(angle_to)
  );
#else
  GRect rect = GRect(center.x - radius, center.y - radius, 2.f * radius, 2.f * radius);
  GPoint from = _angle_to_point_in_rect(rect, angle_from);
  GPoint to = _angle_to_point_in_rect(rect, angle_to);
  
  int iterations = _get_region_from_angle(rect.size, _deg_to_rad(angle_to)) - _get_region_from_angle(rect.size, _deg_to_rad(angle_from));
  if (iterations < 0) {
    iterations += 4;
  }
  
  if (iterations == 0 && angle_to < angle_from) {
    iterations += 4;
  }
  
  GPoint current = from;
  GPoint next = from;
  int region = _get_region_from_angle(GSize(2.f * radius, 2.f * radius), angle_from);
  
  while (iterations >= 0) {
    if (iterations == 0) {
      next = to;
    } else {
      switch (region) {
        case 1: next = GPoint(center.x + radius, center.y - radius); break;
        case 2: next = GPoint(center.x - radius, center.y - radius); break;
        case 3: next = GPoint(center.x - radius, center.y + radius); break;
        case 4: next = GPoint(center.x + radius, center.y + radius); break;
      }
    }
    
    graphics_fill_rect(ctx, GRect(current.x, current.y, next.x - current.x + 1, next.y - current.y + 1), 0, GCornerNone);
    
    iterations--;
    region = (region % 4) + 1;
    current = next;
  }
#endif
}