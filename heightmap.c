
#include "heightmap.h"

#include "color.h"
#include "util.h"

#include <stdio.h>


heightmap_t *heightmap_read(const char *filename) {
  heightmap_t *heightmap;
  float pixel[4];

  if ((heightmap = malloc(sizeof(*heightmap))) == NULL) {
    PERROR("struct allocation");
    return NULL;
  }

  if ((heightmap->image = image_read(filename)) == NULL) {
    free(heightmap);
    return NULL;
  }

  heightmap->reflected = 0;

  image_get_pixel(heightmap->image, pixel, 0, 0);

  heightmap->rainbow = ((pixel[0] != pixel[1]) || (pixel[0] != pixel[2]));

  return heightmap;
}

void heightmap_destroy(heightmap_t *heightmap) {
  image_destroy(heightmap->image);
  free(heightmap);
}

float heightmap_get(const heightmap_t *heightmap, float x, size_t y) {
  float pixel[4];

  if (heightmap->reflected) {
    x = image_get_width(heightmap->image) - x;
  }

  image_get_pixel(heightmap->image, pixel, (size_t) x, y);

  if (heightmap->rainbow) {
    return rgb_to_hue(pixel[0], pixel[1], pixel[2]);
  } else {
    return pixel[0];
  }
}

void heightmap_set_reflected(heightmap_t *heightmap, int reflected) {
  heightmap->reflected = reflected;
}

size_t heightmap_get_width(const heightmap_t *heightmap) {
  return image_get_width(heightmap->image);
}

size_t heightmap_get_height(const heightmap_t *heightmap) {
  return image_get_height(heightmap->image);
}
