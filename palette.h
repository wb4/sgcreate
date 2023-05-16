
#ifndef PALETTE_H
#define PALETTE_H


#include <stddef.h>

#include "color.h"


#define PALETTE_COLOR_MAX 256


typedef struct {
  color_t colors[PALETTE_COLOR_MAX];
  size_t color_count;
} palette_t;


int palette_random(palette_t *p);

void palette_random_color(const palette_t *p, color_t *dest);


#endif
