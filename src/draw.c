#include <pebble.h>
#include "draw.h"

#ifdef PBL_RECT

static int _round(float value) {
  if (value < 0.f) {
    return value -0.5f;
  }
  return value + 0.5f;
}

static int _get_region_from_angle(float angle, GSize size) {
  GPoint point = draw_get_arc_point(angle, size, GPointZero);
  
  if (point.y == -size.h/2) {
    return 0;
  } else if (point.x == size.w/2) {
    return 1;
  } else if (point.y == size.h/2) {
    return 2;
  } else if (point.x == -size.w/2) {
    return 3;
  }
  
  return -1;
}

static int _get_amount_corners(float angle_from, float angle_to, GSize size) {
  int corners = _get_region_from_angle(angle_to, size) - _get_region_from_angle(angle_from, size);
  while (corners < 0) {
    corners += 4;
  }
  if (corners == 0 && (angle_from > angle_to || angle_to - angle_from > 90)) {
    corners += 4;
  }
  
  return corners;
}

static GPoint _get_inner_point(GPoint point, int distance, int region) {
  switch(region) {
    case 0: point.y += distance; break;
    case 1: point.x -= distance; break;
    case 2: point.y -= distance; break;
    case 3: point.x += distance; break;
  }
  
  return point;
}
#endif

void draw_arc(GContext* ctx, GPoint center, GSize size, int width, float angle_from, float angle_to, GColor color) {
  graphics_context_set_fill_color(ctx, color);
  
#ifdef PBL_ROUND  
  graphics_fill_radial(
    ctx,
    GRect(
      center.x - size.w / 2,
      center.y - size.h / 2,
      size.w,
      size.h
    ),
    GOvalScaleModeFitCircle,
    width,
    DEG_TO_TRIGANGLE(angle_from),
    DEG_TO_TRIGANGLE(angle_to)
  );
#else
  GPoint from = draw_get_arc_point(angle_from, size, GPointZero);
  GPoint to = draw_get_arc_point(angle_to, size, GPointZero);
  
  GPoint current = from;
  GPoint next = from;
  int iterations = _get_amount_corners(angle_from, angle_to, size);
  int region = _get_region_from_angle(angle_from, size);
  
  while (iterations >= 0) {
    if (iterations == 0) {
      next = to;
    } else {
      switch (region) {
        case 0: next = GPoint(size.w / 2, -size.h / 2); break;
        case 1: next = GPoint(size.w / 2, size.h / 2); break;
        case 2: next = GPoint(-size.w / 2, size.h / 2); break;
        case 3: next = GPoint(-size.w / 2, -size.h / 2); break;
      }
    }
    
    GPoint next_inner = _get_inner_point(next, width, region);
    
    graphics_fill_rect(
      ctx,
      GRect(
        center.x + current.x,
        center.y + current.y,
        next_inner.x - current.x,
        next_inner.y - current.y
      ),
      0,
      GCornerNone
    );
    
    iterations--;
    region = (region + 1) % 4;
    current = next;
  }
#endif
}
  
GPoint draw_get_arc_point(float angle, GSize size, GPoint offset) {
  int trig_angle = TRIG_MAX_ANGLE * (angle / 360.f);
#ifdef PBL_ROUND
  return GPoint(
    (sin_lookup(trig_angle) * (size.w / 2.f) / TRIG_MAX_RATIO) + offset.x,
    (-cos_lookup(trig_angle) * (size.h / 2.f) / TRIG_MAX_RATIO) + offset.y
  );
#else
  const float FACTOR = 100;
  float max_ratio = (float)size.h / (float)size.w;
  float px = FACTOR * sin_lookup(trig_angle) / TRIG_MAX_RATIO;
  float py = FACTOR * -cos_lookup(trig_angle) / TRIG_MAX_RATIO;
  float absx = px > 0 ? px : -px;
  float absy = py > 0 ? py : -py;
  float ratio = absx == 0 ? 1000 : (float)absy / absx;
  
  float factor = 0;
  if (ratio > max_ratio) {
    factor = (size.h / 2.f) / absy;
  } else {
    factor = (size.w / 2.f) / absx;
  }
    
  return GPoint(_round(factor * px + offset.x), _round(factor * py + offset.y));
#endif
}