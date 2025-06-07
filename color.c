

#include "color.h"

#include "util.h"

#include <math.h>
#include <stdio.h>


static float get_cmin(const color_t *c) {
  return fminf(fminf(c->red, c->green), c->blue);
}

static float get_cmax(const color_t *c) {
  return fmaxf(fmaxf(c->red, c->green), c->blue);
}

static unsigned float_to_byte(float value) {
  return (unsigned) (value * 255);
}


void color_from_rgb(color_t *c, float red, float green, float blue) {
  c->red = red;
  c->green = green;
  c->blue = blue;
}


void color_from_hsv(color_t *c, float hue, float saturation, float value) {
  float rgb[3];

  hsv_to_rgb(rgb, hue, saturation, value);
  color_from_rgb(c, rgb[0], rgb[1], rgb[2]);
}


void color_from_random(color_t *c) {
  color_from_rgb(c, rand_normal(), rand_normal(), rand_normal());
}


color_t color_copy(const color_t *src) {
  color_t dest;

  color_from_rgb(&dest, src->red, src->green, src->blue);
  return dest;
}


int color_equal(color_t a, color_t b) {
  return memcmp(&a, &b, sizeof(a)) == 0;
}


unsigned color_r_byte(const color_t *c) {
  return float_to_byte(c->red);
}

unsigned color_g_byte(const color_t *c) {
  return float_to_byte(c->green);
}

unsigned color_b_byte(const color_t *c) {
  return float_to_byte(c->blue);
}


float color_h(const color_t *c) {
  return rgb_to_hue(c->red, c->green, c->blue);
}

float color_s(const color_t *c) {
  float cmin = get_cmin(c);
  float cmax = get_cmax(c);

  if (cmax == 0) {
    return 0;
  }

  return (cmax - cmin) / cmax;
}

float color_v(const color_t *c) {
  return get_cmax(c);
}


void color_set_h(color_t *c, float hue) {
  color_from_hsv(c, hue, color_s(c), color_v(c));
}

void color_set_s(color_t *c, float saturation) {
  color_from_hsv(c, color_h(c), saturation, color_v(c));
}

void color_set_v(color_t *c, float value) {
  color_from_hsv(c, color_h(c), color_s(c), value);
}


void color_jitter_hsv(color_t *c, float max_jitter) {
  float h = color_h(c);
  float s = color_s(c);
  float v = color_v(c);

  h = jitter_with_wrap(h, max_jitter, 0.0f, 1.0f);
  s = jitter_with_cap(s, max_jitter, 0.0f, 1.0f);
  v = jitter_with_cap(v, max_jitter, 0.0f, 1.0f);

  color_from_hsv(c, h, s, v);
}


void color_from_jittered_hsv_color(color_t *dest, color_t source, float hue_radius, float saturation_radius, float value_radius) {
  float h = color_h(&source);
  float s = color_s(&source);
  float v = color_v(&source);

  h = jitter_with_wrap(h, hue_radius, 0, 1);
  s = jitter_with_cap(s, saturation_radius, 0, 1);
  v = jitter_with_cap(s, value_radius, 0, 1);

  color_from_hsv(dest, h, s, v);
}


color_t color_lerp(color_t a, color_t b, float t) {
  color_t result = {
    lerp_float(a.red, b.red, t),
    lerp_float(a.green, b.green, t),
    lerp_float(a.blue, b.blue, t),
  };
  return result;
}


void color_magick_string(const color_t *c, char *dest, size_t len) {
  snprintf(dest, len, "rgba(%u,%u,%u,1)", color_r_byte(c), color_g_byte(c), color_b_byte(c));
}


void color_hsv_cone_coords(const color_t *c, float *x, float *y, float *z) {
  float hue = color_h(c);
  float sat = color_s(c);
  float val = color_v(c);

  float radius = 0.5f * sat * val;

  *x = radius * cosf(hue);
  *y = radius * sin(hue);
  *z = val;
}


float color_hsv_cone_distance(const color_t *c1, const color_t *c2) {
  float x1, y1, z1;
  float x2, y2, z2;
  float xd, yd, zd;

  color_hsv_cone_coords(c1, &x1, &y1, &z1);
  color_hsv_cone_coords(c2, &x2, &y2, &z2);

  xd = x1 - x2;
  yd = y1 - y2;
  zd = z1 - z2;

  return sqrtf(xd*xd + yd*yd + zd*zd);
}


float rgb_to_hue(float red, float green, float blue) { 
  float M;
  float m;

  float r;
  float g;
  float b;

  float hue;

  M = fmaxf(fmaxf(red, green), blue);
  m = fminf(fminf(red, green), blue);

  r = (M - red) / (M - m);
  g = (M - green) / (M - m);
  b = (M - blue) / (M - m);

  if (red == M) {
    hue = b - g;
  } else if (green == M) {
    hue = 2.0f + r - b;
  } else {
    hue = 4.0f + g - r;
  }
  hue /= 6.0f;

  if (hue < 0.0f) hue += 1.0f;
  if (hue >= 1.0f) hue -= 1.0f;

  return hue;
}


void hsv_to_rgb(float *rgb, float hue, float saturation, float value) {
  float chroma;
  float hue_prime;
  float x;
  float m;

  hue_prime = hue * 6.0f;
  if (hue_prime == 6.0f) {
    hue_prime = 0.0f;
  }

  chroma = value * saturation;

  x = chroma * (1.0f - fabsf(fmodf(hue_prime, 2.0f) - 1.0f));

  rgb[0] = rgb[1] = rgb[2] = 0.0f;

  if (hue_prime < 1.0f) {
    rgb[0] = chroma;
    rgb[1] = x;
  } else if (hue_prime < 2.0f) {
    rgb[0] = x;
    rgb[1] = chroma;
  } else if (hue_prime < 3.0f) {
    rgb[1] = chroma;
    rgb[2] = x;
  } else if (hue_prime < 4.0f) {
    rgb[1] = x;
    rgb[2] = chroma;
  } else if (hue_prime < 5.0f) {
    rgb[0] = x;
    rgb[2] = chroma;
  } else {
    rgb[0] = chroma;
    rgb[2] = x;
  }

  m = value - chroma;

  rgb[0] += m;
  rgb[1] += m;
  rgb[2] += m;
}
