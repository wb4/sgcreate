
#include "palette.h"

#include "util.h"

#include <errno.h>


#define PALETTE_MAX_JITTER (0.02f)
#define PALETTE_MIN_COLOR_DISTANCE (0.15f)
#define PALETTE_HUE_SPAN (0.17f)
#define PALETTE_MIN_SATURATION (0.2f)
#define PALETTE_MAX_COLOR_TRY_COUNT (50)


static void random_color_in_wedge(color_t *dest, float min_hue) {
  float hue = wrap_float(min_hue + rand_float(PALETTE_HUE_SPAN), 0.0f, 1.0f);
  float sat = PALETTE_MIN_SATURATION + (1.0f - PALETTE_MIN_SATURATION) * rand_normal();
  float val = rand_normal();

  color_init_hsv(dest, hue, sat, val);
}


static int color_too_close(const palette_t *p, size_t color_count, const color_t *color) {
  size_t i;

  for (i = 0;  i < color_count;  ++i) {
    if (color_hsv_cone_distance(color, &p->colors[i]) < PALETTE_MIN_COLOR_DISTANCE) {
      return 1;
    }
  }

  return 0;
}


int palette_random(palette_t *p) {
  size_t i;
  size_t try_count;
  float min_hue;

  min_hue = rand_normal();

  for (i = 0;  i < PALETTE_COLOR_MAX;  ++i) {
    try_count = 0;
    do {
      ++try_count;
      if (try_count > PALETTE_MAX_COLOR_TRY_COUNT) {
        break;
      }
      random_color_in_wedge(&p->colors[i], min_hue);
    } while (color_too_close(p, i, &p->colors[i]));
  }

  p->color_count = i;

  return 0;
}


void palette_random_color(const palette_t *p, color_t *dest) {
  size_t color_index;

  color_index = rand_index(p->color_count);
  *dest = color_copy(&p->colors[color_index]);

  color_jitter_hsv(dest, PALETTE_MAX_JITTER);
}
