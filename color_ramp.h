#ifndef COLOR_RAMP_H
#define COLOR_RAMP_H

#include "color.h"

#define COLOR_RAMP_MAX_POINTS (256)

typedef struct {
  float x;
  color_t color;
} color_ramp_point_t;

typedef struct {
  color_ramp_point_t points[COLOR_RAMP_MAX_POINTS];
  size_t point_count;
} color_ramp_t;

void color_ramp_init(color_ramp_t *ramp);
int color_ramp_from_string(color_ramp_t *ramp, const char *s);
int color_ramp_add_point(color_ramp_t *ramp, color_ramp_point_t point);
color_t color_ramp_get(const color_ramp_t *ramp, float x);

#endif
