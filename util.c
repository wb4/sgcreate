
#include "util.h"


#include <math.h>
#include <stdlib.h>


float rand_in_range(float min, float max) { 
  return min + rand() / (RAND_MAX + 1.0f) * (max - min);
}


float rand_r_in_range(unsigned int *seed, float min, float max) {
    return min + rand_r(seed) / (RAND_MAX + 1.0f) * (max - min);
}


float rand_normal(void) {
  return rand() / (RAND_MAX + 1.0f);
}


int rand_in_range_int(int min, int max) { 
  return min + (int) (floorf(rand_normal() * (max - min + 1)));
}


size_t rand_index(size_t length) {
  return (size_t) (rand_normal() * length);
}


int rand_bool(void) {
  return rand() <= RAND_MAX / 2;
}


float cap_float(float value, float min, float max) {
  if (value < min) {
    return min;
  }
  if (value > max) {
    return max;
  }

  return value;
}


float wrap_float(float value, float min, float max) {
  float range = max - min;

  while (value < min) {
    value += range;
  }
  while (value >= max) {
    value -= range;
  }

  return value;
}


float jitter(float value, float max_jitter) {
  float jitter_fraction = powf(rand_normal(), 2);
  if (rand_bool()) {
    jitter_fraction = -jitter_fraction;
  }

  return value + jitter_fraction * max_jitter;
}


float jitter_with_cap(float value, float max_jitter, float min, float max) {
  return cap_float(jitter(value, max_jitter), min, max);
}

float jitter_with_wrap(float value, float max_jitter, float min, float max) {
  return wrap_float(jitter(value, max_jitter), min, max);
}
