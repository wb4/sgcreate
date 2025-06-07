
#ifndef COLOR_H
#define COLOR_H


#include <math.h>
#include <stddef.h>


typedef struct {
  float red;
  float green;
  float blue;
} color_t;

void color_from_rgb(color_t *c, float red, float green, float blue);
void color_from_hsv(color_t *c, float hue, float saturation, float value);
void color_random(color_t *c);

color_t color_copy(const color_t *src);

int color_equal(color_t a, color_t b);

#define color_red(c) (c.red)
#define color_green(c) (c.green)
#define color_blue(c) (c.blue)

unsigned color_r_byte(const color_t *c);
unsigned color_g_byte(const color_t *c);
unsigned color_b_byte(const color_t *c);

float color_h(const color_t *c);
float color_s(const color_t *c);
float color_v(const color_t *c);

void color_set_h(color_t *c, float hue);
void color_set_s(color_t *c, float saturation);
void color_set_v(color_t *c, float value);

void color_jitter_hsv(color_t *c, float max_jitter);
void color_from_jittered_hsv_color(color_t *dest, color_t source, float hue_radius, float saturation_radius, float value_radius);

void color_scale_value(color_t *c, float scale);


color_t color_lerp(color_t a, color_t b, float t);


void color_magick_string(const color_t *c, char *dest, size_t len);


void color_hsv_cone_coords(const color_t *c, float *x, float *y, float *z);
float color_hsv_cone_distance(const color_t *c1, const color_t *c2);


/* Returns the hue in the range (0..1] */
float rgb_to_hue(float red, float green, float blue);

/* rgb must be an array of at least 3 floats. */
void hsv_to_rgb(float *rgb, float hue, float saturation, float value);


#endif
