
#ifndef HEIGHTMAP_H
#define HEIGHTMAP_H

#include "image.h"

typedef struct heightmap_tag {
  image_t *image;
  int reflected;
  int rainbow;
} heightmap_t;

heightmap_t *heightmap_read(const char *filename);

void heightmap_destroy(heightmap_t *heightmap);

float heightmap_get(const heightmap_t *heightmap, float x, size_t y);

void heightmap_set_reflected(heightmap_t *heightmap, int reflected);

size_t heightmap_get_width(const heightmap_t *heightmap);
size_t heightmap_get_height(const heightmap_t *heightmap);

#endif
