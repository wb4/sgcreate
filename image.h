
#ifndef IMAGE_H
#define IMAGE_H

#include "color.h"
#include "metrics.h"

#include <stdlib.h>


typedef struct image_tag {
  size_t width;
  size_t height;

  float *pixels;
} image_t;


typedef enum {
  PATTERN_TYPE_PERLIN,
  PATTERN_TYPE_POLYGONS,
  PATTERN_TYPE_ELLIPSES,
  PATTERN_TYPE_DOTS,
  PATTERN_TYPE_COUNT,  /* Don't use this.  This just indicates how many patterns there are. */
  PATTERN_TYPE_RANDOM
} pattern_t;


void image_init(void);  /* initialize the image library */
void image_close(void);  /* close the image library */

image_t *image_create(size_t width, size_t height);

void image_destroy(image_t *image);

image_t *image_read(const char *filename);

int image_write(image_t *image, const char *filename);

void image_get_pixel(const image_t *image, float *pixel, size_t x, size_t y);

void image_set_pixel(image_t *image, const float *pixel, size_t x, size_t y);
void image_set_pixel_color(image_t *image, color_t color, size_t x, size_t y);

size_t image_get_width(const image_t *image);
size_t image_get_height(const image_t *image);

image_t *image_create_random(size_t width, size_t height, linear_density_t pixel_density, pattern_t type, color_t color);

void image_fill_with_color(image_t *image, color_t color);

int image_add_noise(image_t *image);

int image_scale(image_t *image, size_t width, size_t height);

void image_blend_overlay(image_t *dest, image_t *overlay, float overlay_opacity);

int image_color_from_string(color_t *dest, const char *str);

pattern_t image_pattern_type_from_name(const char *name);

#endif
