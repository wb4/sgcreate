
#ifndef PALETTE_H
#define PALETTE_H


#include <stddef.h>

#include "color.h"


#define PALETTE_COLOR_MAX 256


typedef struct {
  color_t colors[PALETTE_COLOR_MAX];
  size_t color_count;
} palette_t;


int palette_new_around_color(palette_t *p, color_t mid_color);

void palette_random_jittered_color(const palette_t *p, color_t *dest);


#endif
