
#include "palette.h"

#include "util.h"

#include <errno.h>


#define PALETTE_MAX_JITTER (0.02f)
#define PALETTE_MIN_COLOR_DISTANCE (0.01f)

#define PALETTE_HUE_JITTER_RADIUS (0.085f)
#define PALETTE_SATURATION_JITTER_RADIUS (1.0f)
#define PALETTE_VALUE_JITTER_RADIUS (1.0f)

#define PALETTE_MIN_SATURATION (0.2f)
#define PALETTE_MAX_COLOR_TRY_COUNT (50)


static int color_too_close(const palette_t *p, size_t color_count, const color_t *color) {
  size_t i;

  for (i = 0;  i < color_count;  ++i) {
    if (color_hsv_cone_distance(color, &p->colors[i]) < PALETTE_MIN_COLOR_DISTANCE) {
      return 1;
    }
  }

  return 0;
}


int palette_new_around_color(palette_t *p, color_t mid_color) {
  size_t i;
  size_t try_count;

  for (i = 0;  i < PALETTE_COLOR_MAX;  ++i) {
    try_count = 0;
    do {
      ++try_count;
      if (try_count > PALETTE_MAX_COLOR_TRY_COUNT) {
        break;
      }
      color_from_jittered_hsv_color(&p->colors[i], mid_color, PALETTE_HUE_JITTER_RADIUS, PALETTE_SATURATION_JITTER_RADIUS, PALETTE_VALUE_JITTER_RADIUS);
    } while (color_too_close(p, i, &p->colors[i]));
  }

  p->color_count = i;

  return 0;
}


void palette_random_jittered_color(const palette_t *p, color_t *dest) {
  size_t color_index;

  color_index = rand_index(p->color_count);
  *dest = color_copy(&p->colors[color_index]);

  color_jitter_hsv(dest, PALETTE_MAX_JITTER);
}
