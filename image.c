
#include "color.h"
#include "image.h"
#include "palette.h"
#include "perlin.h"
#include "util.h"

#include <errno.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include <wand/MagickWand.h>


#define OPACITY_MIN (0.5f)
#define OPACITY_MAX (1.0f)

#define OBJECT_RADIUS_MIN_MILLIS (1.0f)
#define OBJECT_RADIUS_MAX_MILLIS (3.5f)

#define PERLIN_INNER_LENGTH_MILLIS (1.5f)
#define PERLIN_OUTER_LENGTH_MILLIS (6.0f)

#define DOT_WIDTH_MILLIS (0.5f)

#define PERLIN_INNER_OPACITY (0.6f)
#define PERLIN_OUTER_OPACITY (0.8f)


void image_init(void) {
  MagickWandGenesis();
}


void image_close(void) {
  MagickWandTerminus();
}


typedef int (*draw_object_t)(DrawingWand *draw, PixelWand *pixel, float x, float y, float min_radius, float max_radius, size_t width, const palette_t *palette);


image_t *image_create(size_t width, size_t height) {
  image_t *image;

  if ((image = malloc(sizeof(*image))) == NULL) {
    PERROR("image allocation");
    return NULL;
  }

  image->width = width;
  image->height = height;

  if ((image->pixels = calloc(4 * width * height, sizeof(*image->pixels))) == NULL) {
    free(image);
    return NULL;
  }

  return image;
}


void image_destroy(image_t *image) {
  free(image->pixels);
  free(image);
}


static int magic_wand_to_allocated_image(MagickWand *wand, image_t *image) {
  return MagickExportImagePixels(wand, 0, 0, image->width, image->height, "RGBA", FloatPixel, image->pixels) == MagickFalse ? -1 : 0;
}


static image_t *magic_wand_to_new_image(MagickWand *wand) {
  image_t *image;
  size_t width = MagickGetImageWidth(wand);
  size_t height = MagickGetImageHeight(wand);

  if ((image = image_create(width, height)) == NULL) return NULL;

  if (magic_wand_to_allocated_image(wand, image) == -1) {
    image_destroy(image);
    return NULL;
  }

  return image;
}


image_t *image_read(const char *filename) {
  image_t *image = NULL;

  MagickWand *wand;

  if ((wand = NewMagickWand()) == NULL) {
    PERROR("MagickWand creation");
    return NULL;
  }

  if (MagickReadImage(wand, filename) == MagickFalse) {
    PERROR("reading image");
    goto cleanup;
  }
  if ((image = magic_wand_to_new_image(wand)) == NULL) goto cleanup;

 cleanup:
  DestroyMagickWand(wand);

  return image;
}


static MagickWand *image_to_wand(image_t *image) {
  MagickWand *wand;
  PixelWand *bgcolor;

  if ((wand = NewMagickWand()) == NULL) {
    return NULL;
  }
  if ((bgcolor = NewPixelWand()) == NULL) {
    DestroyMagickWand(wand);
    return NULL;
  }

  if (PixelSetColor(bgcolor, "black") == MagickFalse) {
    DestroyPixelWand(bgcolor);
    DestroyMagickWand(wand);
    return NULL;
  }

  if (MagickNewImage(wand, image->width, image->height, bgcolor) == MagickFalse) {
    DestroyPixelWand(bgcolor);
    DestroyMagickWand(wand);
    return NULL;
  }

  DestroyPixelWand(bgcolor);

  if (MagickImportImagePixels(wand, 0, 0, image->width, image->height, "RGBA", FloatPixel, image->pixels) == MagickFalse) {
    DestroyMagickWand(wand);
    return NULL;
  }

  return wand;
}


static MagickWand *drawing_wand_to_magic_wand(DrawingWand *draw, size_t width, size_t height) {
  MagickWand *wand = NULL;
  PixelWand *color = NULL;

  MagickWand *retval = NULL;

  if ((wand = NewMagickWand()) == NULL) goto bad;
  if ((color = NewPixelWand()) == NULL) goto bad;

  if (PixelSetColor(color, "gray") == MagickFalse) goto bad;

  if (MagickNewImage(wand, width, height, color) == MagickFalse) goto bad;
  if (!MagickSetSize(wand, width, height)) goto bad;

  if (MagickDrawImage(wand, draw) == MagickFalse) goto bad;

  retval = wand;

 cleanup:
  if (color) DestroyPixelWand(color);

  return retval;

 bad:
  retval = NULL;
  if (wand) DestroyMagickWand(wand);
  goto cleanup;
}


static image_t *drawing_wand_to_image(DrawingWand *draw, size_t width, size_t height) {
  MagickWand *wand = NULL;
  image_t *image;

  if ((wand = drawing_wand_to_magic_wand(draw, width, height)) == NULL) return NULL;

  image = magic_wand_to_new_image(wand);

  DestroyMagickWand(wand);

  return image;
}


int image_write(image_t *image, const char *filename) {
  MagickWand *wand;

  if ((wand = image_to_wand(image)) == NULL) {
    return -1;
  }

  if (MagickWriteImage(wand, filename) == MagickFalse) {
    DestroyMagickWand(wand);
    return -1;
  } 

  DestroyMagickWand(wand);

  return 0;
}


void image_get_pixel(const image_t *image, float *pixel, size_t x, size_t y) {
  size_t base;

  /* Sanity check. */
  if (x >= image->width) {
    fprintf(stderr, "x = %lu is outside the image, width = %lu\n", x, image->width);
    exit(1);
  }
  if (y >= image->height) {
    fprintf(stderr, "y = %lu is outside the image, height = %lu\n", y, image->height);
    exit(1);
  }

  base = 4 * (y * image->width + x);

  pixel[0] = image->pixels[base];
  pixel[1] = image->pixels[base + 1];
  pixel[2] = image->pixels[base + 2];
  pixel[3] = image->pixels[base + 3];
}


void image_set_pixel(image_t *image, const float *pixel, size_t x, size_t y) {
  size_t base;

  base = 4 * (y * image->width + x);

  image->pixels[base] = pixel[0];
  image->pixels[base + 1] = pixel[1];
  image->pixels[base + 2] = pixel[2];
  image->pixels[base + 3] = pixel[3];
}


void image_set_pixel_color(image_t *image, color_t color, size_t x, size_t y) {
  float pixel[4] = { color.red, color.green, color.blue, 1 };
  image_set_pixel(image, pixel, x, y);
}


size_t image_get_width(const image_t *image) {
  return image->width;
}


size_t image_get_height(const image_t *image) {
  return image->height;
}


static int set_fill_color(DrawingWand *draw, PixelWand *pixel, const color_t *color) {
  char color_string[32];

  color_magick_string(color, color_string, sizeof(color_string));

  if (PixelSetColor(pixel, color_string) == MagickFalse) return -1;

  DrawSetFillColor(draw, pixel);

  return 0;
}


static int set_random_fill_color(DrawingWand *draw, PixelWand *pixel, const palette_t *palette) {
  color_t color;

  palette_random_jittered_color(palette, &color);

  return set_fill_color(draw, pixel, &color);
}


static void set_random_opacity(DrawingWand *draw, float min, float max) {
  float opacity;

  opacity = min + (max - min) * rand() / (RAND_MAX + 1.0f);
  DrawSetFillOpacity(draw, opacity);
}


int draw_random_ellipse(DrawingWand *draw, PixelWand *pixel, float x, float y, float min_radius, float max_radius, size_t width, const palette_t *palette) {
  float rx, ry;

  if (set_random_fill_color(draw, pixel, palette) == -1) return -1;
  set_random_opacity(draw, OPACITY_MIN, OPACITY_MAX);

  rx = rand_in_range(min_radius, max_radius);
  ry = rand_in_range(min_radius, max_radius);

  DrawEllipse(draw, x, y, rx, ry, 0.0, 360.0);

  if (x - rx < 0.0) {
    DrawEllipse(draw, x + (float) width, y, rx, ry, 0.0, 360.0);
  }
  if (x + rx >= width) {
    DrawEllipse(draw, x - (float) width, y, rx, ry, 0.0, 360.0);
  }

  return 0;
}


int polygon_falls_left(size_t count, const PointInfo *points) {
  size_t i;

  for (i = 0;  i < count;  ++i) {
    if (points[i].x < 0) {
      return 1;
    }
  }

  return 0;
}


int polygon_falls_right(size_t count, const PointInfo *points, size_t end) {
  size_t i;

  for (i = 0;  i < count;  ++i) {
    if (points[i].x >= end) {
      return 1;
    }
  }

  return 0;
}


void shift_polygon(PointInfo *dest, size_t count, const PointInfo *src, int offset) {
  size_t i;

  for (i = 0;  i < count;  ++i) {
    dest[i].x = src[i].x + offset;
    dest[i].y = src[i].y;
  }
}


int draw_random_polygon(DrawingWand *draw, PixelWand *pixel, float x, float y, float min_radius, float max_radius, size_t width, const palette_t *palette) {
  size_t point_count;
  size_t i;
  PointInfo points[30];
  PointInfo temp_points[30];
  float angle;
  float radius;

  if (set_random_fill_color(draw, pixel, palette) == -1) return -1;
  set_random_opacity(draw, OPACITY_MIN, OPACITY_MAX);

  point_count = (size_t) rand_in_range_int(3, 8);

  for (i = 0;  i < point_count;  ++i) {
    angle = rand_normal() * 2.0f * M_PI;
    radius = rand_in_range(min_radius, max_radius);
    points[i].x = x + radius * cosf(angle);
    points[i].y = y + radius * sinf(angle);
  }

  DrawPolygon(draw, point_count, points);

  if (polygon_falls_left(point_count, points)) {
    shift_polygon(temp_points, point_count, points, (int) width);
    DrawPolygon(draw, point_count, temp_points);
  }
  if (polygon_falls_right(point_count, points, width)) {
    shift_polygon(temp_points, point_count, points, -((int) width));
    DrawPolygon(draw, point_count, temp_points);
  }

  return 0;
}


static image_t *image_create_random_objects(size_t width, size_t height, linear_density_t pixel_density, draw_object_t draw_object, const palette_t *palette) {
  DrawingWand *draw = NULL;
  PixelWand *pixel = NULL;
  size_t object_count;

  image_t *image = NULL;
  image_t *retval = NULL;

  if ((draw = NewDrawingWand()) == NULL) goto bad;
  if ((pixel = NewPixelWand()) == NULL) goto bad;

  object_count = width * height / 20;

  length_t object_radius_min = length_from_millimeters(OBJECT_RADIUS_MIN_MILLIS);
  length_t object_radius_max = length_from_millimeters(OBJECT_RADIUS_MAX_MILLIS);

  float object_radius_min_pixels = count_per_length(pixel_density, object_radius_min);
  float object_radius_max_pixels = count_per_length(pixel_density, object_radius_max);

  for (size_t i = 0;  i < object_count;  ++i) {
    float x = rand_normal() * width;
    float y = rand_normal() * height;

    if (draw_object(draw, pixel, x, y, object_radius_min_pixels, object_radius_max_pixels, width, palette) == -1) goto bad;
  }

  if ((image = drawing_wand_to_image(draw, width, height)) == NULL) goto bad;

  retval = image;

 cleanup:
  if (pixel) DestroyPixelWand(pixel);
  if (draw) DestroyDrawingWand(draw);

  return retval;

 bad:
  if (image) image_destroy(image);
  retval = NULL;
  goto cleanup;
}


// Returns NaN on error.
static float perlin_noise_for_pixel(perlin3d_t *perlin, unsigned texture_width, unsigned row, unsigned col) {
  // In order to make the texture tileable horizontally, we map the texture row
  // to a circle in the XZ plane in Perlin space.  (This is why we use 3D perlin
  // noise instead of 2D.)  We'll make the circumference the same length as the row
  // so that the noise's horizontal scale matches its vertical.
  float radius = texture_width / (2 * M_PI);
  float angle_radians = ((float) col / (float) texture_width) * 2 * M_PI;
  float x = radius * cos(angle_radians);
  float z = radius * sin(angle_radians);

  float point[3] = { x, row, z };
  return perlin3d_get(perlin, point);
}


static void inner_perlin_color_map(float color[4], float input) {
  const float threshold = 0.15;

  memset(color, 0, 4 * sizeof(color[0]));

  if (input > -threshold && input < threshold) {
    //color[3] = 1.0 - (input * input / threshold);
    color[3] = 1;
    if (input > 0) {
      color[0] = 1;
      color[1] = 1;
      color[2] = 1;
    }
  }
}


static void outer_perlin_color_map(float color[4], float input) {
  const float inner_threshold = 0.0;
  const float outer_threshold = 0.1;

  memset(color, 0, 4 * sizeof(color[0]));

  if (fabsf(input) > inner_threshold && fabsf(input) < outer_threshold) {
    color[3] = 1;
    if (input > 0) {
      color[0] = 1;
      color[1] = 1;
      color[2] = 1;
    }
  }
}


static int perlin_color_for_pixel(float color[4], perlin3d_t *perlin, unsigned texture_width, unsigned row, unsigned col, void (*color_map)(float color[4], float input)) {
  float noise = perlin_noise_for_pixel(perlin, texture_width, row, col);
  if (isnan(noise)) return -1;
  color_map(color, noise);
  return 0;
}


static int row_render_perlin_noise(image_t *image, unsigned row, perlin3d_t *perlin, void (*color_map)(float color[4], float input)) {
  for (unsigned col = 0;  col < image->width;  col++) {
    float color[4];
    if (perlin_color_for_pixel(color, perlin, image->width, row, col, color_map) == -1) return -1;
    image_set_pixel(image, color, col, row);
  }

  return 0;
}


static int render_perlin_noise(image_t *image, float perlin_scale, void (*color_map)(float color[4], float input)) {
  int retval = 0;
  perlin3d_t perlin;

  if (perlin3d_init(&perlin, perlin_scale, rand()) == -1) return -1;

  for (unsigned row = 0;  row < image->height;  row++) {
    if (row_render_perlin_noise(image, row, &perlin, color_map) == -1) goto bad;
  }

 cleanup:
  perlin3d_destroy(&perlin);

  return retval;

 bad:
  retval = -1;
  goto cleanup;
}


void image_fill_with_color(image_t *image, color_t color) {
  for (unsigned row = 0;  row < image->height;  row++) {
    for (unsigned col = 0;  col < image->width;  col++) {
      image_set_pixel_color(image, color, col, row);
    }
  }
}


static image_t *image_create_perlin(size_t width, size_t height, linear_density_t pixel_density, color_t color) {
  image_t *result = NULL;
  image_t *overlay = NULL;

  if ((result = image_create(width, height)) == NULL) goto bad;
  image_fill_with_color(result, color);

  if ((overlay = image_create(width, height)) == NULL) goto bad;

  length_t inner_length = length_from_millimeters(PERLIN_INNER_LENGTH_MILLIS);
  length_t outer_length = length_from_millimeters(PERLIN_OUTER_LENGTH_MILLIS);
  float inner_scale = count_per_length(pixel_density, inner_length);
  float outer_scale = count_per_length(pixel_density, outer_length);

  if (render_perlin_noise(overlay, inner_scale, inner_perlin_color_map) == -1) goto bad;
  image_blend_overlay(result, overlay, PERLIN_INNER_OPACITY);

  if (render_perlin_noise(overlay, outer_scale, outer_perlin_color_map) == -1) goto bad;
  image_blend_overlay(result, overlay, PERLIN_OUTER_OPACITY);
  
 cleanup:
  if (overlay) image_destroy(overlay);

  return result;

 bad:
  if (result) image_destroy(result);
  result = NULL;
  goto cleanup;
}


static image_t *image_create_random_dots(size_t width, size_t height, linear_density_t pixel_density, palette_t *palette) {
  DrawingWand *draw = NULL;
  PixelWand *color_wand = NULL;

  image_t *image = NULL;
  image_t *retval = NULL;

  if ((draw = NewDrawingWand()) == NULL) goto bad;
  if ((color_wand = NewPixelWand()) == NULL) goto bad;

  length_t dot_width = length_from_millimeters(DOT_WIDTH_MILLIS);
  float dot_width_pixels = count_per_length(pixel_density, dot_width);

  for (float x = 0;  x < width;  x += dot_width_pixels) {
    for (float y = 0;  y < height;  y += dot_width_pixels) {
      color_t color;
      palette_random_jittered_color(palette, &color);
      char color_str[30];
      color_magick_string(&color, color_str, sizeof(color_str));

      if (PixelSetColor(color_wand, color_str) == MagickFalse) goto bad;
      DrawSetFillColor(draw, color_wand);
      DrawRectangle(draw, x, y, x + dot_width_pixels, y + dot_width_pixels);
    }
  }

  if ((image = drawing_wand_to_image(draw, width, height)) == NULL) goto bad;

  retval = image;

 cleanup:
  if (color_wand) DestroyPixelWand(color_wand);
  if (draw) DestroyDrawingWand(draw);

  return retval;

 bad:
  if (image) image_destroy(image);
  retval = NULL;
  goto cleanup;
}


image_t *image_create_random(size_t width, size_t height, linear_density_t pixel_density, pattern_t type, color_t color) {
  if (type == PATTERN_TYPE_RANDOM) {
    type = (pattern_t) ((rand() / (RAND_MAX + 1.0f)) * PATTERN_TYPE_COUNT);
  }

  if (type == PATTERN_TYPE_PERLIN) {
    return image_create_perlin(width, height, pixel_density, color);
  }

  palette_t palette;
  if (palette_new_around_color(&palette, color) == -1) {
    return NULL;
  }

  if (type == PATTERN_TYPE_DOTS) {
    return image_create_random_dots(width, height, pixel_density, &palette);
  }

  draw_object_t draw = NULL;
  switch (type) {
    case PATTERN_TYPE_POLYGONS:
      draw = draw_random_polygon;
      break;
    case PATTERN_TYPE_ELLIPSES:
      draw = draw_random_ellipse;
      break;
    default:
      errno = EINVAL;
      return NULL;
  }

  return image_create_random_objects(width, height, pixel_density, draw, &palette);
}


int image_add_noise(image_t *image) {
  MagickWand *wand = NULL;
  int retval = 0;

  if ((wand = image_to_wand(image)) == NULL) goto bad;
  if (MagickAddNoiseImage(wand, PoissonNoise) == MagickFalse) goto bad;
  if (magic_wand_to_allocated_image(wand, image) == -1) goto bad;

 cleanup:
  if (wand) DestroyMagickWand(wand);

 return retval;

 bad:
  retval = -1;
  goto cleanup;
}


int image_scale(image_t *image, size_t width, size_t height) {
  MagickWand *wand;
  float *pixels;

  if (image->width == width && image->height == height) {
    /* The image is already at the target dimensions. */
    return 0;
  }

  if ((wand = image_to_wand(image)) == NULL) {
    return -1;
  }

  if (MagickScaleImage(wand, width, height) == MagickFalse) {
    DestroyMagickWand(wand);
    return -1;
  }

  if ((pixels = realloc(image->pixels, 4 * width * height * sizeof(*image->pixels))) == NULL) {
    DestroyMagickWand(wand);
    return -1;
  }

  image->width = width;
  image->height = height;
  image->pixels = pixels;

  if (magic_wand_to_allocated_image(wand, image) == -1) {
    wand = DestroyMagickWand(wand);
    return -1;
  }

  DestroyMagickWand(wand);

  return 0;
}


void image_blend_overlay(image_t *dest, image_t *overlay, float overlay_opacity) {
  size_t row_max = dest->height < overlay->height ? dest->height : overlay->height;
  size_t col_max = dest->width < overlay->width ? dest->width : overlay->width;
  for (size_t row = 0;  row < row_max;  row++) {
    for (size_t col = 0;  col < col_max;  col++) {
      float dest_pixel[4];
      float overlay_pixel[4];
      image_get_pixel(dest, dest_pixel, col, row);
      image_get_pixel(overlay, overlay_pixel, col, row);
      float overlay_alpha = overlay_pixel[3] * overlay_opacity;
      float dest_alpha = 1.0 - overlay_alpha;
      for (int i = 0;  i < 3;  i++) {
	dest_pixel[i] = (dest_alpha * dest_pixel[i]) + (overlay_alpha * overlay_pixel[i]);
      }
      image_set_pixel(dest, dest_pixel, col, row);
    }
  }
}


int image_color_from_string(color_t *dest, const char *str) {
  int retval = 0;

  PixelWand *color;
  if ((color = NewPixelWand()) == NULL) goto bad;
  if (PixelSetColor(color, str) == MagickFalse) goto bad;

  color_from_rgb(dest, PixelGetRed(color), PixelGetGreen(color), PixelGetBlue(color));

 cleanup:
  if (color) DestroyPixelWand(color);
  return retval;

 bad:
  retval = -1;
  goto cleanup;
}


pattern_t image_pattern_type_from_name(const char *name) {
  if (!strcmp(name, "perlin")) {
    return PATTERN_TYPE_PERLIN;
  } else if (!strcmp(name, "polygons")) {
    return PATTERN_TYPE_POLYGONS;
  } else if (!strcmp(name, "ellipses")) {
    return PATTERN_TYPE_ELLIPSES;
  } else if (!strcmp(name, "dots")) {
    return PATTERN_TYPE_DOTS;
  } else {
    return -1;
  }
}
