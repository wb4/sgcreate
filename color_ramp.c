
#include "color_ramp.h"
#include "image.h"
#include "util.h"

#include <errno.h>
#include <string.h>

#define RAMP_STR_PARTS_BUFFER_SIZE (32)

void color_ramp_init(color_ramp_t *ramp) {
  ramp->point_count = 0;
}

static int color_ramp_point_from_string(color_ramp_point_t *point, const char *s) {
  char x_str[RAMP_STR_PARTS_BUFFER_SIZE];
  const char *color_str;

  if ((color_str = next_token(x_str, sizeof(x_str), s, ':')) == NULL) return -1;

  char *endptr;
  point->x = strtof(x_str, &endptr);
  if (errno == ERANGE || endptr == x_str) return -1;

  if (image_color_from_string(&point->color, color_str) == -1) return -1;

  return 0;
}

int color_ramp_from_string(color_ramp_t *ramp, const char *s) {
  color_ramp_init(ramp);

  if (!strchr(s, ':') && !strchr(s, ',')) {
    // The string specifies a single color, not a ramp.
    color_t color;
    if (image_color_from_string(&color, s) == -1) return -1;
    color_ramp_point_t point = { 0.0, color };
    if (color_ramp_add_point(ramp, point) == -1) return -1;
    return 0;
  }

  while (*s) {
    char point_str[RAMP_STR_PARTS_BUFFER_SIZE];
    if ((s = next_token(point_str, sizeof(point_str), s, ',')) == NULL) return -1;
    color_ramp_point_t point;
    if (color_ramp_point_from_string(&point, point_str) == -1) return -1;
    if (color_ramp_add_point(ramp, point) == -1) return -1;
  }

  return 0;
}

/*
 * Returns the index of the largest control point which is <= x.
 * If no such point exists, then returns -1.
 */
static int index_for_x(const color_ramp_t *ramp, float x) {
  if (ramp->point_count == 0) {
    return -1;
  }
  if (x < ramp->points[0].x) {
    return -1;
  }
  if (x > ramp->points[ramp->point_count - 1].x) {
    return ramp->point_count - 1;
  }

  int low = 0;
  int high = ramp->point_count;

  while (high > low + 1) {
    int mid = (low + high) / 2;
    if (x == ramp->points[mid].x) {
      return mid;
    }
    if (x < ramp->points[mid].x) {
      high = mid;
    } else {
      low = mid;
    }
  }

  return low;
}

int color_ramp_add_point(color_ramp_t *ramp, color_ramp_point_t point) {
  if (ramp->point_count >= COLOR_RAMP_MAX_POINTS) {
    errno = ENOSPC;
    return -1;
  }

  int index = index_for_x(ramp, point.x);

  if (index >= 0 && ramp->points[index].x == point.x) {
    if (color_equal(ramp->points[index].color, point.color)) {
      return 0;
    } else {
      errno = EEXIST;
      return -1;
    }
  }

  index += 1;

  memmove(&ramp->points[index + 1], &ramp->points[index], (ramp->point_count - index) * sizeof(ramp->points[0]));
  ramp->point_count += 1;

  ramp->points[index] = point;

  return 0;
}

color_t color_ramp_get(const color_ramp_t *ramp, float x) {
  if (ramp->point_count == 0) {
    color_t color;
    color_from_rgb(&color, 0.0, 0.0, 0.0);
    return color;
  }

  int index = index_for_x(ramp, x);

  if (index < 0) {
    return ramp->points[0].color;
  }

  if (index == ramp->point_count - 1) {
    return ramp->points[ramp->point_count - 1].color;
  }

  if (x == ramp->points[index].x) {
    // exact match
    return ramp->points[index].color;
  }

  // linearly interpolate between control points
  float t = (x - ramp->points[index].x) / (ramp->points[index + 1].x - ramp->points[index].x);
  return color_lerp(ramp->points[index].color, ramp->points[index + 1].color, t);
}
